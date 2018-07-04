#include "Timepix3EventLoader.h"

#include <bitset>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace corryvreckan;
using namespace std;

Timepix3EventLoader::Timepix3EventLoader(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)), temporalSplit(false), m_currentEvent(0), m_prevTime(0),
      m_shutterOpen(false) {

    // Take input directory from global parameters
    m_inputDirectory = m_config.get<std::string>("inputDirectory");
    m_triggerLatency = m_config.get<double>("triggerLatency", 0.0);
    m_minNumberOfPlanes = m_config.get<int>("minNumerOfPlanes", 1);

    // Check whether event length or pixel count should be used to separate events:
    m_numberPixelHits = m_config.get<int>("number_of_pixelhits", 2000);

    // Calibration parameters
    calibrationPath = m_config.get<std::string>("calibrationPath", "");
    threshold = m_config.get<std::string>("threshold", "");
}

void Timepix3EventLoader::initialise() {

    if(m_config.has("number_of_pixelhits")) {
        LOG(INFO) << "Splitting events by number of pixel hits on detector plane.";
        temporalSplit = false;
    } else {
        LOG(INFO) << "Event length set, splitting events by time.";
        temporalSplit = true;
    }

    // File structure is RunX/ChipID/files.dat

    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == NULL) {
        throw ModuleError("Directory " + m_inputDirectory + " does not exist");
    } else {
        LOG(TRACE) << "Found directory " << m_inputDirectory;
    }
    dirent* entry;
    dirent* file;

    // Buffer for file names:
    std::map<std::string, std::vector<std::string>> detector_files;

    // Read the entries in the folder
    while((entry = readdir(directory))) {

        // Ignore UNIX functional directories:
        if(std::string(entry->d_name).at(0) == '.') {
            continue;
        }

        // If these are folders then the name is the chip ID
        // For some file systems, dirent only returns DT_UNKNOWN - in this case, check the dir. entry starts with "W"
        if(entry->d_type == DT_DIR || (entry->d_type == DT_UNKNOWN && std::string(entry->d_name).at(0) == 'W')) {

            LOG(DEBUG) << "Found directory for detector " << entry->d_name;

            // Open the folder for this device
            string detectorID = entry->d_name;
            string dataDirName = m_inputDirectory + "/" + entry->d_name;
            DIR* dataDir = opendir(dataDirName.c_str());

            // Check if this device has conditions loaded and is a Timepix3
            Detector* detector;
            try {
                LOG(DEBUG) << "Fetching detector with ID \"" << detectorID << "\"";
                detector = get_detector(detectorID);
            } catch(ModuleError& e) {
                LOG(WARNING) << e.what();
                continue;
            }

            LOG(DEBUG) << "Detector is of type \"" << detector->type() << "\"";
            if(detector->type() != "Timepix3") {
                LOG(WARNING) << "Device with detector ID " << entry->d_name << " is not of type Timepix3.";
                continue;
            }

            // Get all of the files for this chip
            while((file = readdir(dataDir))) {
                string filename = dataDirName + "/" + file->d_name;

                // Check if file has extension .dat
                if(string(file->d_name).find(".dat") != string::npos) {
                    LOG(INFO) << "Enqueuing data file for " << detectorID << ": " << filename;
                    detector_files[detectorID].push_back(filename);
                }

                // If not a data file, it might be a trimdac file, with the list of masked pixels etc.
                if(string(file->d_name).find("trimdac") != string::npos) {
                    // Now that we have all of the data files and mask files for this detector, pass the mask file to
                    // parameters
                    LOG(INFO) << "Set mask file " << filename;
                    detector->setMaskFile(filename);

                    // Apply the pixel masking
                    maskPixels(detector, filename);
                }
            }
        }
    }

    // Check that we have files for every detector in the configuration file and sort them correctly:
    for(auto& detector : get_detectors()) {
        std::string detectorID = detector->name();

        if(detector->type() != "Timepix3") {
            continue;
        }

        if(detector_files.count(detector->name()) == 0) {
            LOG(ERROR) << "No data file found for detector " << detector->name();
            continue;
        }

        // Initialise null values for later
        m_syncTime[detectorID] = 0;
        m_clearedHeader[detectorID] = false;
        m_syncTimeTDC[detectorID] = 0;
        m_TDCoverflowCounter[detectorID] = 0;

        // Sort all files by extracting the "serial number" from the file name while ignoring the timestamp:
        std::sort(detector_files[detector->name()].begin(),
                  detector_files[detector->name()].end(),
                  [](std::string a, std::string b) {
                      auto get_serial = [](std::string name) {
                          const auto pos1 = name.find_last_of('-');
                          const auto pos2 = name.find_last_of('.');
                          return name.substr(pos1 + 1, pos2 - pos1 - 1);
                      };
                      return std::stoi(get_serial(a)) < std::stoi(get_serial(b));
                  });

        // Open them:
        for(auto& filename : detector_files[detectorID]) {

            auto new_file = std::make_unique<std::ifstream>(filename);
            if(new_file->is_open()) {
                LOG(DEBUG) << "Opened data file for " << detectorID << ": " << filename;

                // The header is repeated in every new data file, thus skip it for all.
                uint32_t headerID;
                if(!new_file->read(reinterpret_cast<char*>(&headerID), sizeof headerID)) {
                    throw ModuleError("Cannot read header ID for " + detectorID + " in file " + filename);
                }
                if(headerID != 1380208723) {
                    throw ModuleError("Incorrect header ID for " + detectorID + " in file " + filename + ": " +
                                      std::to_string(headerID));
                }
                LOG(TRACE) << "Header ID: \"" << headerID << "\"";

                // Skip the rest of the file header
                uint32_t headerSize;
                if(!new_file->read(reinterpret_cast<char*>(&headerSize), sizeof headerSize)) {
                    throw ModuleError("Cannot read header size for " + detectorID + " in file " + filename);
                }

                // Skip the full header:
                new_file->seekg(headerSize);
                LOG(TRACE) << "Skipped header (" << headerSize << "b)";

                // Store the file in the data vector:
                m_files[detectorID].push_back(std::move(new_file));
            } else {
                throw ModuleError("Could not open data file " + filename);
            }
        }
    }

    // Set the file iterator to the first file for every detector:
    for(auto& detector : m_files) {
        m_file_iterator[detector.first] = detector.second.begin();
    }

    // Calibration
    pixelToT_beforecalibration = new TH1F("pixelToT_beforecalibration", "pixelToT_beforecalibration", 100, 0, 200);

    if(m_config.has("calibrationPath") && m_config.has("threshold")) {
        LOG(INFO) << "Applying calibration from " << calibrationPath;
        applyCalibration = true;

        // get DUT plane name
        std::string DUT = m_config.get<std::string>("DUT");

        // make paths to calibration files and read
        int ret = 0;
        std::string tmp;
        tmp = calibrationPath + "/" + DUT + "/cal_thr_" + threshold + "_ik_10/" + DUT + "_cal_tot.txt";
        loadCalibration(tmp, ' ', vtot);
        tmp = calibrationPath + "/" + DUT + "/cal_thr_" + threshold + "_ik_10/" + DUT + "_cal_toa.txt";
        loadCalibration(tmp, ' ', vtoa);

        // make graphs of calibration parameters
        LOG(DEBUG) << "Creating calibration graphs";
        pixelTOTParameterA = new TH2F("hist_par_a_tot", "hist_par_a_tot", 256, 0, 256, 256, 0, 256);
        pixelTOTParameterB = new TH2F("hist_par_b_tot", "hist_par_b_tot", 256, 0, 256, 256, 0, 256);
        pixelTOTParameterC = new TH2F("hist_par_c_tot", "hist_par_c_tot", 256, 0, 256, 256, 0, 256);
        pixelTOTParameterT = new TH2F("hist_par_t_tot", "hist_par_t_tot", 256, 0, 256, 256, 0, 256);
        pixelTOAParameterC = new TH2F("hist_par_c_toa", "hist_par_c_toa", 256, 0, 256, 256, 0, 256);
        pixelTOAParameterD = new TH2F("hist_par_d_toa", "hist_par_d_toa", 256, 0, 256, 256, 0, 256);
        pixelTOAParameterT = new TH2F("hist_par_t_toa", "hist_par_t_toa", 256, 0, 256, 256, 0, 256);
        timeshiftPlot = new TH1F("timeshift", "timeshift (/ns)", 1000, -10, 80);
        pixelToT_aftercalibration = new TH1F("pixelToT_aftercalibration", "pixelToT_aftercalibration", 2000, 0, 20000);

        for(int row = 0; row < 256; row++) {
            for(int col = 0; col < 256; col++) {
                float a = vtot.at(256 * row + col).at(2);
                float b = vtot.at(256 * row + col).at(3);
                float c = vtot.at(256 * row + col).at(4);
                float t = vtot.at(256 * row + col).at(5);
                float toa_c = vtoa.at(256 * row + col).at(2);
                float toa_t = vtoa.at(256 * row + col).at(3);
                float toa_d = vtoa.at(256 * row + col).at(4);

                pixelTOTParameterA->Fill(col, row, a);
                pixelTOTParameterB->Fill(col, row, b);
                pixelTOTParameterC->Fill(col, row, c);
                pixelTOTParameterT->Fill(col, row, t);
                pixelTOAParameterC->Fill(col, row, toa_c);
                pixelTOAParameterD->Fill(col, row, toa_d);
                pixelTOAParameterT->Fill(col, row, toa_t);
            }
        }
    } else {
        LOG(INFO) << "No calibration file path given, data will be uncalibrated.";
        applyCalibration = false;
    }
}

StatusCode Timepix3EventLoader::run(Clipboard* clipboard) {

    // This will loop through each timepix3 registered, and load data from each of them. This can
    // be done in one of two ways: by taking all data in the time interval (t,t+delta), or by
    // loading a fixed number of pixels (ie. 2000 at a time)

    // Check if event frame is defined:
    if(!clipboard->has_persistent("eventStart") || !clipboard->has_persistent("eventEnd")) {
        throw ModuleError("Event not defined. Add Metronome module or Event reader defining the event.");
    }

    LOG(TRACE) << "== New event";
    int loadedData = 0;

    // End the loop as soon as all detector files are finished:
    StatusCode returnvalue = Failure;

    // Loop through all registered detectors
    for(auto& detector : get_detectors()) {

        // Check if they are a Timepix3
        if(detector->type() != "Timepix3") {
            continue;
        }

        string detectorID = detector->name();
        // If all files for this detector have been read, ignore it:
        if(m_file_iterator[detectorID] == m_files[detectorID].end()) {
            continue;
        }

        // This detector has data left:
        returnvalue = Success;

        // Make a new container for the data
        Pixels* deviceData = new Pixels();
        SpidrSignals* spidrData = new SpidrSignals();

        // Load the next chunk of data
        bool data = loadData(clipboard, detector, deviceData, spidrData);

        // If data was loaded then put it on the clipboard
        if(data) {
            loadedData++;
            LOG(DEBUG) << "Loaded " << deviceData->size() << " pixels for device " << detectorID;
            clipboard->put(detectorID, "pixels", (Objects*)deviceData);
        }
        clipboard->put(detectorID, "SpidrSignals", (Objects*)spidrData);
    }

    // Otherwise tell event loop to keep running
    IFLOG(INFO) {
        if(temporalSplit) {
            LOG_PROGRESS(INFO, "tpx3_loader")
                << "Current time: " << Units::display(clipboard->get_persistent("eventStart"), {"s", "ms", "us", "ns"});
        } else {
            LOG_PROGRESS(INFO, "tpx3_loader") << "Current event: " << m_currentEvent;
        }
    }

    // If no/not enough data in this event then tell the event loop to directly skip to the next event
    if(returnvalue == Success && loadedData < m_minNumberOfPlanes) {
        return NoData;
    }

    return returnvalue;
}

// Function to load the pixel mask file
void Timepix3EventLoader::maskPixels(Detector* detector, string trimdacfile) {

    // Open the mask file
    ifstream trimdacs;
    trimdacs.open(trimdacfile.c_str());

    // Ignore the file header
    string line;
    getline(trimdacs, line);
    int t_col, t_row, t_trim, t_mask, t_tpen;

    // Loop through the pixels in the file and apply the mask
    for(int col = 0; col < 256; col++) {
        for(int row = 0; row < 256; row++) {
            trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
            if(t_mask)
                detector->maskChannel(t_col, t_row);
        }
    }

    // Close the files when finished
    trimdacs.close();
}

// Function to load calibration data
void Timepix3EventLoader::loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat) {
    std::ifstream f;
    f.open(path);
    dat.clear();

    // check if file is open
    if(!f.is_open()) {
        LOG(ERROR) << "Cannot open input file:\n\t" << path;
        throw InvalidValueError(m_config, "calibrationPath", "Parsing error in calibration file.");
    }

    // read file line by line
    int i = 0;
    std::string line;
    while(!f.eof()) {
        std::getline(f, line);

        // check if line is empty or a comment
        // if not write to output vector
        if(line.size() > 0 && isdigit(line.at(0))) {
            std::stringstream ss(line);
            std::string word;
            std::vector<float> row;
            while(std::getline(ss, word, delim)) {
                i += 1;
                row.push_back(stof(word));
            }
            dat.push_back(row);
        }
    }

    // warn if too few entries
    if(dat.size() != 256 * 256) {
        LOG(ERROR) << "Something went wrong. Found only " << i << " entries. Not enough for TPX3.\n\t";
        throw InvalidValueError(m_config, "calibrationPath", "Parsing error in calibration file.");
    }

    f.close();
}

// Function to load data for a given device, into the relevant container
bool Timepix3EventLoader::loadData(Clipboard* clipboard, Detector* detector, Pixels* devicedata, SpidrSignals* spidrData) {

    string detectorID = detector->name();

    bool extra = false; // temp

    LOG(DEBUG) << "Loading data for device " << detectorID;
    while(1) {
        // Check if current file is at its end and move to the next:
        if((*m_file_iterator[detectorID])->eof()) {
            m_file_iterator[detectorID]++;
            LOG(INFO) << "Starting to read next file for " << detectorID << ": " << (*m_file_iterator[detectorID]).get();
        }

        // Check if the last file is finished:
        if(m_file_iterator[detectorID] == m_files[detectorID].end()) {
            LOG(INFO) << "EOF for all files of " << detectorID;
            break;
        }

        // Now read the data packets.
        ULong64_t pixdata = 0;
        UShort_t thr = 0;

        // If we can't read data anymore, jump to begin of loop:
        if(!(*m_file_iterator[detectorID])->read(reinterpret_cast<char*>(&pixdata), sizeof pixdata)) {
            LOG(INFO) << "No more data in current file for " << detectorID << ": " << (*m_file_iterator[detectorID]).get();
            continue;
        }

        LOG(TRACE) << "0x" << hex << pixdata << dec << " - " << pixdata;

        // Get the header (first 4 bits) and do things depending on what it is
        // 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
        const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;

        // Use header 0x4 to get the long timestamps (called syncTime here)
        if(header == 0x4) {
            LOG(TRACE) << "Found syncTime data";

            // The 0x4 header tells us that it is part of the timestamp
            // There is a second 4-bit header that says if it is the most
            // or least significant part of the timestamp
            const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

            // This is a bug fix. There appear to be errant packets with garbage data
            // - source to be tracked down.
            // Between the data and the header the intervening bits should all be 0,
            // check if this is the case
            const UChar_t intermediateBits = ((pixdata & 0x00FF000000000000) >> 48) & 0xFF;
            if(intermediateBits != 0x00) {
                LOG(DEBUG) << "Detector " << detectorID << ": intermediateBits error";
                continue;
            }

            // 0x4 is the least significant part of the timestamp
            if(header2 == 0x4) {
                // The data is shifted 16 bits to the right, then 12 to the left in
                // order to match the timestamp format (net 4 right)
                m_syncTime[detectorID] =
                    (m_syncTime[detectorID] & 0xFFFFF00000000000) + ((pixdata & 0x0000FFFFFFFF0000) >> 4);
            }
            // 0x5 is the most significant part of the timestamp
            if(header2 == 0x5) {
                // The data is shifted 16 bits to the right, then 44 to the left in
                // order to match the timestamp format (net 28 left)
                m_syncTime[detectorID] =
                    (m_syncTime[detectorID] & 0x00000FFFFFFFFFFF) + ((pixdata & 0x00000000FFFF0000) << 28);
                if(!m_clearedHeader[detectorID] && (double)m_syncTime[detectorID] / (4096. * 40000000.) < 6.) {
                    m_clearedHeader[detectorID] = true;
                    LOG(DEBUG) << detectorID << ": Cleared header";
                }
            }
        }

        // In data taking during 2015 there was sometimes still data left in the buffers at the start of
        // a run. For that reason we keep skipping data until this "header" data has been cleared, when
        // the heart beat signal starts from a low number (~few seconds max)
        if(!m_clearedHeader[detectorID]) {
            LOG(TRACE) << "Header not cleared, skipping data block.";
            continue;
        }

        // Header 0x06 and 0x07 are the start and stop signals for power pulsing
        if(header == 0x0) {

            // Only want to read these packets from the DUT
            if(detectorID != m_config.get<std::string>("DUT")) {
                continue;
            }

            // Get the second part of the header
            const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

            // New implementation of power pulsing signals from Adrian
            if(header2 == 0x6) {
                LOG(TRACE) << "Found power pulsing - start";

                // Read time stamp and convert to nanoseconds
                const double timestamp = ((pixdata & 0x0000000FFFFFFFFF) << 12) / (4096 * 0.04);
                const uint64_t controlbits = ((pixdata & 0x00F0000000000000) >> 52) & 0xF;

                const uint64_t powerOn = ((controlbits & 0x2) >> 1);
                const uint64_t shutterClosed = ((controlbits & 0x1));

                // Stop looking at data if the signal is after the current event window
                // (and rewind the file
                // reader so that we start with this signal next event)
                if(temporalSplit) {
                    if(timestamp > clipboard->get_persistent("eventEnd")) {
                        (*m_file_iterator[detectorID])->seekg(-1 * sizeof(pixdata), std::ios_base::cur);
                        LOG(TRACE) << "Signal has a time beyond the current event: " << Units::display(timestamp, "ns");
                        break;
                    }
                }

                SpidrSignal* powerSignal =
                    (powerOn ? new SpidrSignal("powerOn", timestamp) : new SpidrSignal("powerOff", timestamp));
                spidrData->push_back(powerSignal);
                LOG(DEBUG) << "Power is " << (powerOn ? "on" : "off") << " power! Time: " << Units::display(timestamp, "ns");

                LOG(TRACE) << "Shutter closed: " << hex << shutterClosed << dec;

                SpidrSignal* shutterSignal = (shutterClosed ? new SpidrSignal("shutterClosed", timestamp)
                                                            : new SpidrSignal("shutterOpen", timestamp));
                if(!shutterClosed) {
                    spidrData->push_back(shutterSignal);
                    m_shutterOpen = true;
                    LOG(TRACE) << "Have opened shutter with signal " << shutterSignal->type() << " at time "
                               << Units::display(timestamp, "ns");
                }

                if(shutterClosed && m_shutterOpen) {
                    spidrData->push_back(shutterSignal);
                    m_shutterOpen = false;
                    LOG(TRACE) << "Have closed shutter with signal " << shutterSignal->type() << " at time "
                               << Units::display(timestamp, "ns");
                }

                LOG(DEBUG) << "Shutter is " << (shutterClosed ? "closed" : "open")
                           << ". Time: " << Units::display(timestamp, "ns");
            }

            /*
            // 0x6 is power on
            if(header2 == 0x6){
              const double timestamp = ((pixdata & 0x0000000FFFFFFFFF) << 12 ) / (4096 * 0.04);
              SpidrSignal* signal = new SpidrSignal("powerOn",timestamp);
              spidrData->push_back(signal);
              LOG(DEBUG) <<"Turned on power! Time: " << Units::display(timestamp, "ns");
            }
            // 0x7 is power off
            if(header2 == 0x7){
              const double timestamp = ((pixdata & 0x0000000FFFFFFFFF) << 12 ) / (4096 * 0.04);
              SpidrSignal* signal = new SpidrSignal("powerOff",timestamp);
              spidrData->push_back(signal);
              LOG(DEBUG) <<"Turned off power! Time: " << Units::display(timestamp, "ns");
            }
             */
        }

        // Header 0x6 indicate trigger data
        if(header == 0x6) {
            const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;
            if(header2 == 0xF) {
                unsigned long long int stamp = (pixdata & 0x1E0) >> 5;
                long long int timestamp_raw = (pixdata & 0xFFFFFFFFE00) >> 9;
                long long int timestamp = 0;
                int triggerNumber = ((pixdata & 0xFFF00000000000) >> 44);
                int intermediate = (pixdata & 0x1F);
                if(intermediate != 0)
                    continue;

                // if jump back in time is larger than 1 sec, overflow detected...
                if((m_syncTimeTDC[detectorID] - timestamp_raw) > 0x1312d000) {
                    m_TDCoverflowCounter[detectorID]++;
                }
                m_syncTimeTDC[detectorID] = timestamp_raw;
                timestamp = timestamp_raw + ((unsigned long long int)(m_TDCoverflowCounter[detectorID]) << 35);

                double triggerTime = (timestamp + stamp / 12.) / (8. * 0.04); // 320 MHz clock
                SpidrSignal* triggerSignal = new SpidrSignal("trigger", triggerTime);
                spidrData->push_back(triggerSignal);
            }
        }

        // Header 0xA and 0xB indicate pixel data
        if(header == 0xA || header == 0xB) {
            LOG(TRACE) << "Found pixel data";

            // Decode the pixel information from the relevant bits
            const UShort_t dcol = ((pixdata & 0x0FE0000000000000) >> 52);
            const UShort_t spix = ((pixdata & 0x001F800000000000) >> 45);
            const UShort_t pix = ((pixdata & 0x0000700000000000) >> 44);
            const UShort_t col = (dcol + pix / 4);
            const UShort_t row = (spix + (pix & 0x3));

            // Check if this pixel is masked
            if(detector->masked(col, row)) {
                LOG(DEBUG) << "Detector " << detectorID << ": pixel " << col << "," << row << " masked";
                continue;
            }

            // Get the rest of the data from the pixel
            const UShort_t pixno = col * 256 + row;
            const UInt_t data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
            const unsigned int tot = (data & 0x00003FF0) >> 4;
            const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
            const uint64_t ftoa(data & 0x0000000F);
            const uint64_t toa((data & 0x0FFFC000) >> 14);

            // Calculate the timestamp.
            long long int time =
                (((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8) + (m_syncTime[detectorID] & 0xFFFFFC0000000000);

            // Adjusting phases for double column shift
            time += ((col / 2 - 1) % 16) * 256;

            // Add the timing offset (in nano seconds) from the detectors file (if any)
            time += (long long int)(detector->timingOffset() * 4096. * 0.04);

            // The time from the pixels has a maximum value of ~26 seconds. We compare the pixel time to the "heartbeat"
            // signal (which has an overflow of ~4 years) and check if the pixel time has wrapped back around to 0

            // If the counter overflow happens before reading the new heartbeat
            //      while( abs(m_syncTime[detectorID]-time) > 0x0000020000000000 ){
            if(!extra) {
                while(m_syncTime[detectorID] - time > 0x0000020000000000) {
                    time += 0x0000040000000000;
                }
            } else {
                while(m_prevTime - time > 0x0000020000000000) {
                    time += 0x0000040000000000;
                }
            }

            // Convert final timestamp into ns:
            const double timestamp = time / (4096 * 0.04);
            LOG(DEBUG) << "Timestamp = " << Units::display(timestamp, {"s", "ns"});
            // If events are loaded based on time intervals, take all hits where the
            // time is within this window

            // Ignore pixel data if it is before the "currentTime" read from the clipboard storage:
            if(temporalSplit && (timestamp < clipboard->get_persistent("eventStart"))) {
                LOG(TRACE) << "Skipping pixel, is before event window (" << Units::display(timestamp, {"s", "us", "ns"})
                           << " < " << Units::display(clipboard->get_persistent("eventStart"), {"s", "us", "ns"}) << ")";
                continue;
            }

            // Stop looking at data if the pixel is after the current event window
            // (and rewind the file reader so that we start with this pixel next event)
            if(temporalSplit && (timestamp > clipboard->get_persistent("eventEnd"))) {
                LOG(DEBUG) << "Stopping processing event, pixel is after "
                              "event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " > "
                           << Units::display(clipboard->get_persistent("eventEnd"), {"s", "us", "ns"}) << ")";
                (*m_file_iterator[detectorID])->seekg(-1 * sizeof(pixdata), std::ios_base::cur);
                break;
            }

            // Otherwise create a new pixel object
            pixelToT_beforecalibration->Fill((int)tot);

            // Apply calibration if applyCalibration is true
            if(applyCalibration && detectorID == m_config.get<std::string>("DUT")) {
                LOG(DEBUG) << "Applying calibration to DUT";
                float a = vtot.at(256 * row + col).at(2);
                float b = vtot.at(256 * row + col).at(3);
                float c = vtot.at(256 * row + col).at(4);
                float t = vtot.at(256 * row + col).at(5);

                float toa_c = vtoa.at(256 * row + col).at(2);
                float toa_t = vtoa.at(256 * row + col).at(3);
                float toa_d = vtoa.at(256 * row + col).at(4);

                // Calculating calibrated tot and toa
                float fvolts =
                    (sqrt(a * a * t * t + 2 * a * b * t + 4 * a * c - 2 * a * t * tot + b * b - 2 * b * tot + tot * tot) +
                     a * t - b + tot) /
                    (2 * a);
                float fcharge = fvolts * 1e-3 * 3e-15 * 6241.509 * 1e15; // capacitance is 3 fF or 18.7 e-/mV

                /* Note 1: fvolts is the inverse to f(x) = a*x + b - c/(x-t). Note the +/- signs! */
                /* Note 2: The capacitance is actually smaller than 3 fC, more like 2.5 fC. But there is an offset when when
                 * using testpulses. Multiplying the voltage value with 20 [e-/mV] is a good approximation but means one is
                 * over estimating the input capacitance to compensate the missing information of the offset. */

                float t_shift = toa_c / (fvolts - toa_t) + toa_d;
                timeshiftPlot->Fill(Units::convert(t_shift, "ns"));
                const double ftimestamp = timestamp - t_shift;
                LOG(DEBUG) << "Time shift= " << Units::display(t_shift, {"s", "ns"});
                LOG(DEBUG) << "Timestamp calibrated = " << Units::display(ftimestamp, {"s", "ns"});
                // creating new pixel object with calibrated values of tot and toa
                Pixel* pixel = new Pixel(detectorID, row, col, fcharge, ftimestamp);
                devicedata->push_back(pixel);
                LOG(DEBUG) << "Pixel Charge = " << fcharge << "; ToT value = " << tot;
                pixelToT_aftercalibration->Fill(fcharge);
            } else {
                LOG(DEBUG) << "Pixel hit at " << Units::display(timestamp, {"s", "ns"});
                // creating new pixel object with non-calibrated values of tot and toa
                Pixel* pixel = new Pixel(detectorID, row, col, (int)tot, timestamp);
                devicedata->push_back(pixel);
            }

            m_prevTime = time;
        }

        // Stop when we reach some large number of pixels (if events not based on time)
        if(!temporalSplit && devicedata->size() >= m_numberPixelHits) {
            break;
        }
    }

    // Now we have data buffered into the temporary storage. We will sort this by time, and then load
    // the data from one event onto it.

    // If no data was loaded, return false
    if(devicedata->empty()) {
        return false;
    }

    // Count events:
    m_currentEvent++;
    return true;
}

void Timepix3EventLoader::finalise() {}
