#include "Clicpix2EventLoader.h"

#include "clicpix2_pixels.hpp"
#include "clicpix2_utilities.hpp"
#include "datatypes.hpp"

using namespace corryvreckan;
using namespace std;
using namespace caribou;
using namespace clicpix2_utils;

Clicpix2EventLoader::Clicpix2EventLoader(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    discardZeroToT = m_config.get<bool>("discardZeroToT", false);
}

void Clicpix2EventLoader::initialise() {

    auto det = get_detector(m_config.get<std::string>("DUT"));
    if(det->type() != "CLICpix2") {
        LOG(ERROR) << "DUT not of type CLICpix2";
    }

    // Take input directory from global parameters
    string inputDirectory = m_config.get<std::string>("inputDirectory");

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
        if(filename.find(".raw") != string::npos) {
            LOG(INFO) << "Found file " << filename;
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

    // Check if longcounting mode is on, defined by pixel 0,0:
    int maxcounter = 256;
    if(matrix_config[std::make_pair(0, 0)].GetLongCounter()) {
        maxcounter = 8192;
    }

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    hHitMapDiscarded = new TH2F(
        "hitMapDiscarded", "hitMapDiscarded", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());

    hPixelToT = new TH1F("pixelToT", "pixelToT", 32, 0, 31);
    hPixelToTMap = new TProfile2D("pixelToTMap",
                                  "pixelToTMap",
                                  det->nPixelsX(),
                                  0,
                                  det->nPixelsX(),
                                  det->nPixelsY(),
                                  0,
                                  det->nPixelsY(),
                                  0,
                                  maxcounter - 1);
    hPixelToA = new TH1F("pixelToA", "pixelToA", maxcounter, 0, maxcounter - 1);
    hPixelCnt = new TH1F("pixelCnt", "pixelCnt", maxcounter, 0, maxcounter - 1);
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
            std::istringstream header(data);
            std::string guff;
            int frameNumber;
            header >> guff >> frameNumber >> guff;
            LOG(DEBUG) << "Found next header, frame number = " << frameNumber;
            break;
        }
        // If there is a colon, then this is a timestamp
        else if(data.find(":") != string::npos) {
            istringstream timestamp(data);
            char colon;
            int value;
            long long int time;
            timestamp >> value >> colon >> time;
            LOG(DEBUG) << "Found timestamp: " << time;
            if(value == 3) {
                shutterOpen = true;
                shutterStartTimeInt = time;
            } else if(value == 1 && shutterOpen) {
                shutterOpen = false;
                shutterStopTimeInt = time;
            }
        } else {
            // Otherwise pixel data
            rawData.push_back(atoi(data.c_str()));
        }
    }

    // Now set the event time so that the Timepix3 data is loaded correctly, unit is nanoseconds
    // NOTE FPGA clock is always on 100MHz from CaR oscillator, same as chip
    shutterStartTime = shutterStartTimeInt / 0.1;
    shutterStopTime = shutterStopTimeInt / 0.1;

    try {
        LOG(DEBUG) << "Decoding data frame...";
        decoder->decode(rawData);
        pearydata data = decoder->getZerosuppressedFrame();

        for(const auto& px : data) {

            auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
            int col = px.first.first;
            int row = px.first.second;

            // If this pixel is masked, do not save it
            if(get_detector(detectorID)->masked(col, row)) {
                continue;
            }

            // Disentangle data types from pixel:
            int tot, toa = -1, cnt = -1;

            // ToT will throw if longcounter is enabled:
            try {
                tot = cp2_pixel->GetTOT();
                hPixelToT->Fill(tot);
                hPixelToTMap->Fill(col, row, tot);
            } catch(caribou::WrongDataFormat&) {
                // Set ToT to one of not defined.
                tot = 1;
            }

            // Decide whether information is counter of ToA
            if(matrix_config[std::make_pair(row, col)].GetCountingMode()) {
                cnt = cp2_pixel->GetCounter();
                hPixelCnt->Fill(cnt);
            } else {
                toa = cp2_pixel->GetTOA();
                hPixelToA->Fill(toa);
            }

            Pixel* pixel = new Pixel(detectorID, row, col, tot, shutterStartTime);

            if(tot == 0 && discardZeroToT) {
                hHitMapDiscarded->Fill(col, row);
            } else {
                pixels->push_back(pixel);
                npixels++;
                hHitMap->Fill(col, row);
            }
        }
    } catch(caribou::DataException& e) {
        LOG(ERROR) << "Caugth DataException: " << e.what() << ", clearing event data.";
    }
    LOG(DEBUG) << "Finished decoding, storing " << pixels->size() << " pixels";

    // Store current frame time and the length of the event:
    LOG(DEBUG) << "Event time: " << Units::display(shutterStartTime, {"ns", "us", "s"})
               << ", length: " << Units::display((shutterStopTime - shutterStartTime), {"ns", "us", "s"});
    clipboard->put_persistent("currentTime", shutterStartTime);
    clipboard->put_persistent("eventLength", (shutterStopTime - shutterStartTime));

    // Put the data on the clipboard
    if(!pixels->empty()) {
        clipboard->put(detectorID, "pixels", (Objects*)pixels);
    }

    // Fill histograms
    hPixelsPerFrame->Fill(npixels);

    // Return value telling analysis to keep running
    return Success;
}

void Clicpix2EventLoader::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
