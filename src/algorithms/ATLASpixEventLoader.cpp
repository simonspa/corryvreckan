#include "ATLASpixEventLoader.h"
#include <regex>

using namespace corryvreckan;
using namespace std;

ATLASpixEventLoader::ATLASpixEventLoader(Configuration config, Clipboard* clipboard)
    : Algorithm(std::move(config), clipboard) {}

void ATLASpixEventLoader::initialise(Parameters* par) {

    parameters = par;

    // File structure is RunX/ATLASpix/data.dat

    // Take input directory from global parameters
    string inputDirectory = m_config.get<std::string>("inputDirectory") + "/ATLASpix";

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
        if(filename.find(".dat") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0)
        LOG(WARNING) << "No data file was found for ATLASpix in " << inputDirectory;

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 128, 0, 128, 128, 0, 128);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 1000, 0, 1000);

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ATLASpixEventLoader::run(Clipboard* clipboard) {

    // Assume that the CLICpix is the DUT (if running this algorithm
    string detectorID = parameters->DUT;

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    // Regular expression to find the hit information:
    std::regex regex_hit("^Col\\(([0-9]+)\\); "
                         "Row\\(([0-9]+)\\); "
                         "TS\\(([0-9]+)\\); "
                         "TOT\\(([0-9]+)\\); "
                         "TriggerTSCoarse\\(([0-9]+)\\); "
                         "TriggerTSFine\\(([0-9]+)\\); "
                         "TriggerIndex\\(([0-9]+)\\)");
    std::smatch sm;

    // Pixel container, shutter information
    Pixels* pixels = new Pixels();
    string data;

    int old_trg_index = 0;
    std::streampos oldpos;

    // Read file and load data
    while(getline(m_file, data)) {

        LOG(TRACE) << "Raw data: " << data;

        if(std::regex_search(data, sm, regex_hit)) {
            int col = std::stoi(sm[1]);
            int row = std::stoi(sm[2]);
            int tot = std::stoi(sm[4]);
            int trg_index = std::stoi(sm[7]);

            // If this pixel is masked, do not save it
            if(parameters->detector[detectorID]->masked(col, row)) {
                continue;
            }

            // Treat first event correctly:
            if(old_trg_index == 0) {
                old_trg_index = trg_index;
            }

            // Break at new event:
            if(trg_index != old_trg_index) {
                LOG(TRACE) << "Peek   (" << col << "," << row << ") TOT: " << tot << " TRG: " << trg_index;
                m_file.seekg(oldpos);
                old_trg_index = trg_index;
                m_eventNumber++;
                break;
            }

            LOG(DEBUG) << "Hit at (" << col << "," << row << ") TOT: " << tot << " TRG: " << trg_index;
            LOG_PROGRESS(INFO, "atlaspix_reader") << "Current time: " << trg_index << ", " << m_eventNumber << " events";

            Pixel* pixel = new Pixel(detectorID, row, col, tot, 0);
            pixels->push_back(pixel);
            hHitMap->Fill(col, row);
            hPixelToT->Fill(tot);
        } else {
            LOG(WARNING) << "No hit data: " << data;
        }

        // Update position of last hit read:
        oldpos = m_file.tellg();
    }

    // Put the data on the clipboard
    if(pixels->size() > 0)
        clipboard->put(detectorID, "pixels", (TestBeamObjects*)pixels);

    // Fill histograms
    hPixelsPerFrame->Fill(pixels->size());

    // Return value telling analysis to keep running
    return Success;
}

void ATLASpixEventLoader::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
