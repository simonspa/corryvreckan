#include "EventLoaderCLICpix2.h"

#include "CLICpix2/clicpix2_pixels.hpp"
#include "CLICpix2/clicpix2_utilities.hpp"

using namespace corryvreckan;
using namespace std;
using namespace caribou;
using namespace clicpix2_utils;

EventLoaderCLICpix2::EventLoaderCLICpix2(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    discardZeroToT = m_config.get<bool>("discard_zero_tot", false);
}

void EventLoaderCLICpix2::initialise() {

    // Take input directory from global parameters
    string inputDirectory = m_config.getPath("input_directory");

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
    std::string title = m_detector->name() + " Hit map;x [px];y [px];pixels";
    hHitMap = new TH2F("hitMap",
                       title.c_str(),
                       m_detector->nPixels().X(),
                       0,
                       m_detector->nPixels().X(),
                       m_detector->nPixels().Y(),
                       0,
                       m_detector->nPixels().Y());
    title = m_detector->name() + " Map of discarded hits;x [px];y [px];pixels";
    hHitMapDiscarded = new TH2F("hitMapDiscarded",
                                title.c_str(),
                                m_detector->nPixels().X(),
                                0,
                                m_detector->nPixels().X(),
                                m_detector->nPixels().Y(),
                                0,
                                m_detector->nPixels().Y());
    title = m_detector->name() + " TOT spectrum;TOT;pixels";
    hPixelToT = new TH1F("pixelToT", title.c_str(), 32, 0, 31);
    title = m_detector->name() + " TOT map;x [px];y [px];TOT";
    hPixelToTMap = new TProfile2D("pixelToTMap",
                                  title.c_str(),
                                  m_detector->nPixels().X(),
                                  0,
                                  m_detector->nPixels().X(),
                                  m_detector->nPixels().Y(),
                                  0,
                                  m_detector->nPixels().Y(),
                                  0,
                                  maxcounter - 1);
    title = m_detector->name() + " TOA spectrum;TOA;pixels";
    hPixelToA = new TH1F("pixelToA", title.c_str(), maxcounter, 0, maxcounter - 1);
    title = m_detector->name() + " CNT spectrum;CNT;pixels";
    hPixelCnt = new TH1F("pixelCnt", title.c_str(), maxcounter, 0, maxcounter - 1);
    title = m_detector->name() + " Pixel multiplicity;pixels;frames";
    hPixelsPerFrame = new TH1F("pixelsPerFrame", title.c_str(), 1000, 0, 1000);

    title = m_detector->name() + " Timewalk;TOA;TOT;pixels";
    hTimeWalk = new TH2F("timewalk", title.c_str(), maxcounter, 0, maxcounter - 1, 32, 0, 31);

    title = m_detector->name() + " Map of masked pixels;x [px];y [px];mask code";
    hMaskMap = new TH2F("maskMap",
                        title.c_str(),
                        m_detector->nPixels().X(),
                        0,
                        m_detector->nPixels().X(),
                        m_detector->nPixels().Y(),
                        0,
                        m_detector->nPixels().Y());
    for(int column = 0; column < m_detector->nPixels().X(); column++) {
        for(int row = 0; row < m_detector->nPixels().Y(); row++) {
            if(m_detector->masked(column, row)) {
                hMaskMap->Fill(column, row, 2);
            }
            if(matrix_config[std::make_pair(row, column)].GetMask()) {
                hMaskMap->Fill(column, row, 1);
            }
        }
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EventLoaderCLICpix2::run(std::shared_ptr<Clipboard> clipboard) {

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return StatusCode::Failure;
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
            int tot = -1, toa = -1, cnt = -1;

            // ToT will throw if longcounter is enabled:
            try {
                tot = cp2_pixel->GetTOT();
                if(!discardZeroToT || tot > 0) {
                    hPixelToT->Fill(tot);
                    hPixelToTMap->Fill(col, row, tot);
                }
            } catch(caribou::DataException&) {
                // Set ToT to one if not defined.
                tot = 1;
            }

            // Time defaults ot rising shutter edge:
            double timestamp = shutterStartTime;

            // Decide whether information is counter of ToA
            if(matrix_config[std::make_pair(row, col)].GetCountingMode()) {
                cnt = cp2_pixel->GetCounter();
                if(!discardZeroToT || tot > 0) {
                    hPixelCnt->Fill(cnt);
                }
            } else {
                toa = cp2_pixel->GetTOA();
                // Convert ToA form 100MHz clk into ns and sutract from shutterStopTime. Then add configured detector time
                // offset
                timestamp = shutterStopTime - static_cast<double>(toa) / 0.1 + m_detector->timingOffset();
                if(!discardZeroToT || tot > 0) {
                    hPixelToA->Fill(toa);
                    hTimeWalk->Fill(toa, tot);
                }
            }

            Pixel* pixel = new Pixel(m_detector->name(), row, col, tot, timestamp);

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
        return StatusCode::NoData;
    }

    // Fill histograms
    hPixelsPerFrame->Fill(npixels);

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderCLICpix2::finalise() {
    delete decoder;
}
