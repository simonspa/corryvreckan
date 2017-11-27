#include "ATLASpixEventLoader.h"
#include <regex>

using namespace corryvreckan;
using namespace std;

ATLASpixEventLoader::ATLASpixEventLoader(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {

    m_timewalkCorrectionFactors = m_config.getArray<double>("timewalkCorrectionFactors", std::vector<double>());
    m_timestampPeriod = m_config.get<double>("timestampPeriod", Units::convert(25, "ns"));

    m_inputDirectory = m_config.get<std::string>("inputDirectory");
    m_calibrationFile = m_config.get<std::string>("calibrationFile");
    m_eventLength = m_config.get<double>("eventLength", 0.000010);

    m_startTime - m_config.get<double>("startTime", 0.);
    m_toaMode = m_config.get<bool>("toaMode", false);
}

void ATLASpixEventLoader::initialise() {

    // File structure is RunX/ATLASpix/data.dat

    // Assume that the ATLASpix is the DUT (if running this algorithm
    string detectorID = m_config.get<std::string>("DUT");

    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == NULL) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    dirent* entry;
    dirent* file;

    // Read the entries in the folder
    while(entry = readdir(directory)) {
        // Check for the data file
        string filename = m_inputDirectory + "/" + entry->d_name;
        if(filename.find(".dat") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0)
        LOG(WARNING) << "No data file was found for ATLASpix in " << m_inputDirectory;

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 128, 0, 128, 400, 0, 400);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 200, 0, 200);

    // Read calibration:
    m_calibrationFactors.resize(25 * 400, 1.0);
    std::ifstream calibration(m_calibrationFile);
    std::string line;
    std::getline(calibration, line);

    int col, row;
    double calibfactor;
    while(getline(calibration, line)) {
        std::istringstream(line) >> col >> row >> calibfactor;
        m_calibrationFactors.at(row * 25 + col) = calibfactor;
    }
    calibration.close();

    LOG(INFO) << "Timewalk corrtion factors: ";
    for(auto& ts : m_timewalkCorrectionFactors) {
        LOG(INFO) << ts;
    }

    m_clockFactor = m_timestampPeriod / 25;
    LOG(INFO) << "Applying clock scaling factor: " << m_clockFactor << std::endl;

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ATLASpixEventLoader::run(Clipboard* clipboard) {

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

            auto detector = get_detector(detectorID);
            // If this pixel is masked, do not save it
            if(detector->masked(col, row)) {
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
