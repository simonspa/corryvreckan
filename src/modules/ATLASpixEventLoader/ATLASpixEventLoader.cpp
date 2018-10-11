#include "ATLASpixEventLoader.h"
#include <regex>

using namespace corryvreckan;
using namespace std;

ATLASpixEventLoader::ATLASpixEventLoader(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_timewalkCorrectionFactors = m_config.getArray<double>("timewalkCorrectionFactors", std::vector<double>());

    m_inputDirectory = m_config.get<std::string>("inputDirectory");
    m_calibrationFile = m_config.get<std::string>("calibrationFile", std::string());

    m_clockCycle = m_config.get<double>("clockCycle", Units::convert(25, "ns"));

    // Allow reading of legacy data format using the Karlsruhe readout system:
    m_legacyFormat = m_config.get<bool>("legacyFormat", false);

    m_startTime = m_config.get<double>("startTime", 0.);
    m_toaMode = m_config.get<bool>("toaMode", false);
}

void ATLASpixEventLoader::initialise() {

    // File structure is RunX/ATLASpix/data.dat
    // Assume that the ATLASpix is the DUT (if running this algorithm
    m_detectorID = m_config.get<std::string>("DUT");
    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == NULL) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    dirent* entry;
    dirent* file;

    // Read the entries in the folder
    while((entry = readdir(directory))) {
        // Check for the data file
        string filename = m_inputDirectory + "/" + entry->d_name;
        if(filename.find("data.txt") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0) {
        LOG(WARNING) << "No data file was found for ATLASpix in " << m_inputDirectory;
    } else {
        LOG(STATUS) << "Opened data file for ATLASpix: " << m_filename;
    }

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Skip the first line as it only contains headers:
    std::string headerline;
    std::getline(m_file, headerline);

    // Make histograms for debugging
    auto det = get_detector(m_detectorID);
    hHitMap = new TH2F("hitMap", "hitMap", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelToTCal = new TH1F("pixelToTCal", "pixelToT", 100, 0, 100);
    hPixelToA = new TH1F("pixelToA", "pixelToA", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 200, 0, 200);
    hPixelsOverTime = new TH1F("pixelsOverTime", "pixelsOverTime", 2e6, 0, 2e6);

    // Read calibration:
    m_calibrationFactors.resize(det->nPixelsX() * det->nPixelsY(), 1.0);
    if(!m_calibrationFile.empty()) {
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
    }

    LOG(INFO) << "Timewalk correction factors: ";
    for(auto& ts : m_timewalkCorrectionFactors) {
        LOG(INFO) << ts;
    }

    LOG(INFO) << "Using clock cycle length of " << m_clockCycle << std::endl;

    // Initialise member variables
    m_eventNumber = 0;
    m_oldtoa = 0;
    m_overflowcounter = 0;
}

StatusCode ATLASpixEventLoader::run(Clipboard* clipboard) {

    // Check if event frame is defined:
    if(!clipboard->has_persistent("eventStart") || !clipboard->has_persistent("eventEnd")) {
        throw ModuleError("Event not defined. Add Metronome module or Event reader defining the event.");
    }

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    double start_time = clipboard->get_persistent("eventStart");
    double end_time = clipboard->get_persistent("eventEnd");

    // Read pixel data
    Pixels* pixels = (m_legacyFormat ? read_legacy_data(start_time, end_time) : read_caribou_data(start_time, end_time));

    for(auto px : (*pixels)) {
        hHitMap->Fill(px->column(), px->row());
        hPixelToT->Fill(px->tot());
        hPixelToTCal->Fill(px->charge());
        hPixelToA->Fill(px->timestamp());

        // Pixels per 100us:
        hPixelsOverTime->Fill(Units::convert(px->timestamp(), "ms"));
    }

    // Fill histograms
    hPixelsPerFrame->Fill(pixels->size());

    // Put the data on the clipboard
    if(!pixels->empty()) {
        clipboard->put(m_detectorID, "pixels", (Objects*)pixels);
    } else {
        return NoData;
    }

    // Return value telling analysis to keep running
    return Success;
}

Pixels* ATLASpixEventLoader::read_caribou_data(double start_time, double end_time) {

    // Pixel container
    Pixels* pixels = new Pixels();

    // Detector we're looking at:
    auto detector = get_detector(m_detectorID);

    // Read file and load data
    std::string line_str;
    while(getline(m_file, line_str)) {
        double timestamp;
        std::istringstream line(line_str);

        std::string identifier;
        line >> identifier;
        m_identifiers[identifier]++;

        if(identifier == "WEIRD_DATA") {
            continue;
        } else if(identifier == "TRIGGER") {
            int id, cnt;
            line >> id >> cnt;
            LOG(DEBUG) << "Trigger at " << cnt;
        } else if(identifier == "HIT") {
            // Read columns form file
            std::string scol, srow, sts1, sts2, stot, sfpga_ts, str_cnt, sbin_cnt;
            line >> scol >> srow >> sts1 >> sts2 >> stot >> sfpga_ts >> str_cnt >> sbin_cnt;

            // Only convert used numbers:
            long long col = std::stoll(scol);
            long long row = std::stoll(srow);
            long long fpga_ts = std::stoll(sfpga_ts);
            long long tot = std::stoll(stot);

            // Convert the timestamp to nanoseconds:
            timestamp = fpga_ts * m_clockCycle;

            // Stop looking at data if the pixel is after the current event window
            // (and rewind the file reader so that we start with this pixel next event)
            if(timestamp > end_time) {
                LOG(DEBUG) << "Stopping processing event, pixel is after event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " > "
                           << Units::display(end_time, {"s", "us", "ns"}) << ")";
                // Rewind to previous position:
                LOG(TRACE) << "Rewinding to file pointer : " << oldpos;
                m_file.seekg(oldpos);
                break;
            }

            if(timestamp < start_time) {
                LOG(DEBUG) << "Skipping pixel hit, pixel is before event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " < "
                           << Units::display(start_time, {"s", "us", "ns"}) << ")";
                continue;
            }

            // If this pixel is masked, do not save it
            if(detector->masked(col, row)) {
                continue;
            }

            Pixel* pixel = new Pixel(m_detectorID, row, col, tot, timestamp);
            LOG(DEBUG) << *pixel;
            pixels->push_back(pixel);
        } else {
            LOG(DEBUG) << "Unknown identifier \"" << identifier << "\"";
        }

        // Store this position in the file in case we need to rewind:
        LOG(TRACE) << "Storing file pointer position: " << m_file.tellg();
        oldpos = m_file.tellg();
    }

    return pixels;
}

Pixels* ATLASpixEventLoader::read_legacy_data(double, double) {

    // Pixel container
    Pixels* pixels = new Pixels();

    // Read file and load data
    while(!m_file.eof()) {

        unsigned int col, row, tot, ts;
        unsigned long long int toa, TriggerDebugTS, dummy, bincounter;

        m_file >> col >> row >> ts >> tot >> dummy >> dummy >> bincounter >> TriggerDebugTS;

        auto detector = get_detector(m_detectorID);
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
        toa /= (4096. * 0.04);

        Pixel* pixel = new Pixel(m_detectorID, row, col, cal_tot, toa);
        pixel->setCharge(cal_tot);
        pixels->push_back(pixel);
    }

    return pixels;
}

void ATLASpixEventLoader::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
    LOG(INFO) << "Identifier distribution:";
    for(auto id : m_identifiers) {
        LOG(INFO) << "\t" << id.first << ": " << id.second;
    }
}
