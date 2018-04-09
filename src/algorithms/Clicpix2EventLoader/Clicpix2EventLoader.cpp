#include "Clicpix2EventLoader.h"

#include "clicpix2_pixels.hpp"
#include "clicpix2_utilities.hpp"
#include "datatypes.hpp"

using namespace corryvreckan;
using namespace std;
using namespace caribou;
using namespace clicpix2_utils;

Clicpix2EventLoader::Clicpix2EventLoader(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {}

void Clicpix2EventLoader::initialise() {

    // File structure is RunX/CLICpix2/data.csv

    // Take input directory from global parameters
    string inputDirectory = m_config.get<std::string>("inputDirectory") + "/CLICpix2";

    // Open the root directory
    DIR* directory = opendir(inputDirectory.c_str());
    if(directory == NULL) {
        LOG(ERROR) << "Directory " << inputDirectory << " does not exist";
        return;
    }
    dirent* entry;
    dirent* file;

    // Read the entries in the folder
    while(entry = readdir(directory)) {
        // Check for the data file
        string filename = inputDirectory + "/" + entry->d_name;
        if(filename.find(".csv") != string::npos) {
            m_filename = filename;
            LOG(INFO) << "Found data file: " << m_filename;
        }
        if(filename.find(".cfg") != string::npos) {
            if(filename.find("matrix") != string::npos) {
                m_matrix = filename;
                LOG(INFO) << "Found matrix file: " << m_matrix;
            }
        }
    }

    if(m_matrix.empty()) {
        LOG(ERROR) << "No matrix configuration file found in " << inputDirectory;
        return;
    }

    // Read the matrix configuration:
    matrix_config = clicpix2_utils::readMatrix(m_matrix);
    // Make sure we initializefd all pixels:
    for(size_t column = 0; column < 128; column++) {
        for(size_t row = 0; row < 128; row++) {
            pixelConfig px = matrix_config[std::make_pair(row, column)];
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.empty()) {
        LOG(WARNING) << "No data file was found for CLICpix2 in " << inputDirectory;
    }

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Compression flags
    comp = true;
    sp_comp = true;

    std::streampos oldpos;
    std::string line;

    // Parse the header:
    while(getline(m_file, line)) {

        if(!line.length()) {
            continue;
        }
        if('#' != line.at(0)) {
            break;
        }

        // Replicate header to new file:
        LOG(DEBUG) << "Detected file header: " << line;
        oldpos = m_file.tellg();

        // Search for compression settings:
        std::string::size_type n = line.find(" sp_comp:");
        if(n != std::string::npos) {
            LOG(DEBUG) << "Value read for sp_comp: " << line.substr(n + 9, 1);
            sp_comp = static_cast<bool>(std::stoi(line.substr(n + 9, 1)));
            LOG(INFO) << "Superpixel Compression: " << (sp_comp ? "ON" : "OFF");
        }
        n = line.find(" comp:");
        if(n != std::string::npos) {
            LOG(DEBUG) << "Value read for comp: " << line.substr(n + 6, 1);
            comp = static_cast<bool>(std::stoi(line.substr(n + 6, 1)));
            LOG(INFO) << "     Pixel Compression: " << (comp ? "ON" : "OFF");
        }
    }

    decoder = new clicpix2_frameDecoder(comp, sp_comp, matrix_config);
    LOG(INFO) << "Prepared CLICpix2 frame decoder.";

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 128, 0, 128, 128, 0, 128);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 1000, 0, 1000);

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode Clicpix2EventLoader::run(Clipboard* clipboard) {

    // Assume that the CLICpix is the DUT (if running this algorithm
    string detectorID = m_config.get<std::string>("DUT");

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    // Otherwise load a new frame

    // Pixel container, shutter information
    Pixels* pixels = new Pixels();
    long long int shutterStartTimeInt, shutterStopTimeInt;
    long double shutterStartTime, shutterStopTime;
    string data;
    int npixels = 0;
    bool shutterOpen = false;
    std::vector<uint32_t> rawData;

    // Read file and load data
    while(getline(m_file, data)) {

        // Check if this is a header
        if(data.find("=====") != string::npos) {
            istringstream header(data);
            char guff;
            int frameNumber;
            header >> guff >> frameNumber >> guff;
            continue;
        }

        // If we are at the end of a frame then the next character will be a "="
        char c = m_file.peek();
        if(strcmp(&c, "=") == 0) {
            LOG(DEBUG) << "End of frame found";
            break;
        }

        // Otherwise load data

        // If there is a colon, then this is a timestamp
        if(data.find(":") != string::npos) {
            istringstream timestamp(data);
            char colon;
            int value;
            long long int time;
            timestamp >> value >> colon >> time;
            if(value == 3) {
                shutterOpen = true;
                shutterStartTimeInt = time;
            } else if(value == 1 && shutterOpen) {
                shutterOpen = false;
                shutterStopTimeInt = time;
            }
            continue;
        }

        // Otherwise pixel data
        rawData.push_back(atoi(data.c_str()));
    }

    try {
        decoder->decode(rawData);
        pearydata data = decoder->getZerosuppressedFrame();

        for(const auto& px : data) {

            auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
            int col = px.first.first;
            int row = px.first.second;
            int tot = cp2_pixel->GetTOT();

            // If this pixel is masked, do not save it
            if(get_detector(detectorID)->masked(col, row)) {
                continue;
            }

            // FIXME TOA is missing...
            Pixel* pixel = new Pixel(detectorID, row, col, tot, 0);
            pixels->push_back(pixel);
            npixels++;
            hHitMap->Fill(col, row);
            hPixelToT->Fill(tot);
        }
    } catch(caribou::DataException& e) {
        LOG(ERROR) << "Caugth DataException: " << e.what() << ", clearing event data.";
    }

    // Now set the event time so that the Timepix3 data is loaded correctly, unit is nanoseconds
    shutterStartTime = shutterStartTimeInt / 0.04;
    shutterStopTime = shutterStopTimeInt / 0.04;

    clipboard->put_persistent("currentTime", shutterStartTime);
    m_config.set<double>("eventLength", (shutterStopTime - shutterStartTime));

    // Put the data on the clipboard
    if(pixels->size() > 0)
        clipboard->put(detectorID, "pixels", (TestBeamObjects*)pixels);

    // Fill histograms
    hPixelsPerFrame->Fill(npixels);

    // Return value telling analysis to keep running
    return Success;
}

void Clicpix2EventLoader::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
