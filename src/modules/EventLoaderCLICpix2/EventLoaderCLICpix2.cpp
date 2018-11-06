#include "EventLoaderCLICpix2.h"

#include "CLICpix2/clicpix2_pixels.hpp"
#include "CLICpix2/clicpix2_utilities.hpp"
#include "datatypes.hpp"

using namespace corryvreckan;
using namespace std;
using namespace caribou;
using namespace clicpix2_utils;

EventLoaderCLICpix2::EventLoaderCLICpix2(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    discardZeroToT = m_config.get<bool>("discardZeroToT", false);
}

void EventLoaderCLICpix2::initialise() {

    // Take input directory from global parameters
    string inputDirectory = m_config.get<std::string>("inputDirectory");

    // Open the root directory
    DIR* directory = opendir(inputDirectory.c_str());
    if(directory == nullptr) {
        throw ModuleError("Directory " + inputDirectory + " does not exist");
    }

    dirent* entry;

    // Read the entries in the folder
    while((entry = readdir(directory))) {
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
        throw ModuleError("No matrix configuration file found in " + inputDirectory);
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
    hHitMap = new TH2F("hitMap",
                       "hitMap",
                       m_detector->nPixelsX(),
                       0,
                       m_detector->nPixelsX(),
                       m_detector->nPixelsY(),
                       0,
                       m_detector->nPixelsY());
    hHitMapDiscarded = new TH2F("hitMapDiscarded",
                                "hitMapDiscarded",
                                m_detector->nPixelsX(),
                                0,
                                m_detector->nPixelsX(),
                                m_detector->nPixelsY(),
                                0,
                                m_detector->nPixelsY());

    hPixelToT = new TH1F("pixelToT", "pixelToT", 32, 0, 31);
    hPixelToTMap = new TProfile2D("pixelToTMap",
                                  "pixelToTMap",
                                  m_detector->nPixelsX(),
                                  0,
                                  m_detector->nPixelsX(),
                                  m_detector->nPixelsY(),
                                  0,
                                  m_detector->nPixelsY(),
                                  0,
                                  maxcounter - 1);
    hPixelToA = new TH1F("pixelToA", "pixelToA", maxcounter, 0, maxcounter - 1);
    hPixelCnt = new TH1F("pixelCnt", "pixelCnt", maxcounter, 0, maxcounter - 1);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 1000, 0, 1000);

    hMaskMap = new TH2F("maskMap",
                        "maskMap",
                        m_detector->nPixelsX(),
                        0,
                        m_detector->nPixelsX(),
                        m_detector->nPixelsY(),
                        0,
                        m_detector->nPixelsY());
    for(int column = 0; column < m_detector->nPixelsX(); column++) {
        for(int row = 0; row < m_detector->nPixelsY(); row++) {
            if(m_detector->masked(column, row)) {
                hMaskMap->Fill(column, row, 2);
            } else if(matrix_config[std::make_pair(row, column)].GetMask()) {
                hMaskMap->Fill(column, row, 1);
            }
        }
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EventLoaderCLICpix2::run(Clipboard* clipboard) {

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    // Pixel container, shutter information
    Pixels* pixels = new Pixels();
    long long int shutterStartTimeInt = 0, shutterStopTimeInt = 0;
    double shutterStartTime, shutterStopTime;
    string datastring;
    int npixels = 0;
    bool shutterOpen = false;
    std::vector<uint32_t> rawData;

    // Read file and load data
    while(getline(m_file, datastring)) {

        // Check if this is a header
        if(datastring.find("=====") != string::npos) {
            std::istringstream header(datastring);
            std::string guff;
            int frameNumber;
            header >> guff >> frameNumber >> guff;
            LOG(DEBUG) << "Found next header, frame number = " << frameNumber;
            break;
        }
        // If there is a colon, then this is a timestamp
        else if(datastring.find(":") != string::npos) {
            istringstream timestamp(datastring);
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
            rawData.push_back(static_cast<unsigned int>(std::atoi(datastring.c_str())));
        }
    }

    // Now set the event time so that the Timepix3 data is loaded correctly, unit is nanoseconds
    // NOTE FPGA clock is always on 100MHz from CaR oscillator, same as chip
    shutterStartTime = static_cast<double>(shutterStartTimeInt) / 0.1;
    shutterStopTime = static_cast<double>(shutterStopTimeInt) / 0.1;

    try {
        LOG(DEBUG) << "Decoding data frame...";
        decoder->decode(rawData);
        pearydata data = decoder->getZerosuppressedFrame();

        for(const auto& px : data) {

            auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
            int col = px.first.first;
            int row = px.first.second;

            // If this pixel is masked, do not save it
            if(m_detector->masked(col, row)) {
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

            Pixel* pixel = new Pixel(m_detector->name(), row, col, tot, shutterStartTime);

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
    clipboard->put_persistent("eventStart", shutterStartTime);
    clipboard->put_persistent("eventEnd", shutterStopTime);
    clipboard->put_persistent("eventLength", (shutterStopTime - shutterStartTime));

    // Put the data on the clipboard
    if(!pixels->empty()) {
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
    } else {
        delete pixels;
    }

    // Fill histograms
    hPixelsPerFrame->Fill(npixels);

    // Return value telling analysis to keep running
    return Success;
}

void EventLoaderCLICpix2::finalise() {
    delete decoder;
}
