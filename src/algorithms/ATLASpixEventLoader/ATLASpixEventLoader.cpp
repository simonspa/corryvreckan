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

    // Skip the first line as it only contains headers:
    std::string headerline;
    std::getline(m_file, headerline);

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 128, 0, 128, 400, 0, 400);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelToTCal = new TH1F("pixelToTCal", "pixelToT", 100, 0, 100);
    hPixelToA = new TH1F("pixelToA", "pixelToA", 100, 0, 100);
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
    m_oldtoa = 0;
    m_overflowcounter = 0;
}

StatusCode ATLASpixEventLoader::run(Clipboard* clipboard) {

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    // Pixel container
    Pixels* pixels = new Pixels();

    // Read file and load data
    while(!m_file.eof()) {

        unsigned int col, row, tot, ts;
        unsigned long long int toa, TriggerDebugTS, dummy, bincounter;

        m_file >> col >> row >> ts >> tot >> dummy >> dummy >> bincounter >> TriggerDebugTS;

        auto detector = get_detector(detectorID);
        // If this pixel is masked, do not save it
        if(detector->masked(col, row)) {
            continue;
        }

        // TOT
        if(tot <= (ts * 2 & 0x3F)) {
            tot = 64 + tot - (ts * 2 & 0x3F);
        } else {
            tot = tot - (ts * 2 & 0x3F);
        }

        // Apply calibration:
        unsigned int cal_tot = tot * m_calibrationFactors.at(row * 25 + col);
        LOG(TRACE) << "Hit " << row << "\t" << col << ": " << m_calibrationFactors.at(row * 25 + col) << " * " << tot
                   << " = " << cal_tot;

        ts &= 0xFF;
        ts *= 2; // atlaspix timestamp runs at 10MHz, multiply by to to get 20.

        if((bincounter & 0x1FF) < ts) {
            toa = ((bincounter & 0xFFFFFFFFFFFFFE00) - (1 << 9)) | (ts & 0x1FF);
        } else {
            toa = (bincounter & 0xFFFFFFFFFFFFFE00) | (ts & 0x1FF);
        }

        if(((toa + 10000) & 0xFFFFF000) < (m_oldtoa & 0xFFFFF000)) {
            m_overflowcounter++;
            LOG(DEBUG) << "Overflow detected " << m_overflowcounter << " " << (toa & 0xFFFFF000) << " "
                       << (m_oldtoa & 0xFFFFF000);
        } // Atlaspix only! Toa has overflow at 32 bits.

        toa += (0x100000000 * m_overflowcounter);
        m_oldtoa = toa & 0xFFFFFFFF;
        LOG(DEBUG) << "    " << row << "\t" << col << ": " << tot << " " << ts << " " << bincounter << " " << toa << " "
                   << (TriggerDebugTS - toa);

        TriggerDebugTS *= 4096. / 5;              // runs with 200MHz, divide by 5 to scale counter value to 40MHz
        toa *= 4096. * (unsigned long long int)2; // runs with 20MHz, multiply by 2 to scale counter value to 40MHz

        // Timewalk correction:
        if(m_timewalkCorrectionFactors.size() == 5) {
            double corr = m_timewalkCorrectionFactors.at(0) + m_timewalkCorrectionFactors.at(1) * tot +
                          m_timewalkCorrectionFactors.at(2) * tot * tot +
                          m_timewalkCorrectionFactors.at(3) * tot * tot * tot +
                          m_timewalkCorrectionFactors.at(4) * tot * tot * tot * tot;
            toa -= corr * (4096. * 40000000.);
        }

        // Convert TOA to nanoseconds:
        toa /= (4096. * 0.04.);

        Pixel* pixel = new Pixel(detectorID, row, col, cal_tot, toa);
        pixels->push_back(pixel);
        hHitMap->Fill(col, row);
        hPixelToT->Fill(tot);
        hPixelToTCal->Fill(cal_tot);
        hPixelToA->Fill(toa);
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
