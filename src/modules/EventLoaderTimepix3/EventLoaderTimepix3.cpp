#include "EventLoaderTimepix3.h"

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

EventLoaderTimepix3::EventLoaderTimepix3(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector), m_currentEvent(0), m_prevTime(0), m_shutterOpen(false) {

    // Take input directory from global parameters
    m_inputDirectory = m_config.getPath("input_directory");

    // Calibration parameters
    if(m_config.has("calibration_path")) {
        calibrationPath = m_config.getPath("calibration_path");
        threshold = m_config.get<std::string>("threshold", "");
    }
}

void EventLoaderTimepix3::initialise() {

    // File structure is RunX/ChipID/files.dat

    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        throw ModuleError("Directory " + m_inputDirectory + " does not exist");
    } else {
        LOG(TRACE) << "Found directory " << m_inputDirectory;
    }
    dirent* entry;
    dirent* file;

    // Buffer for file names:
    std::vector<std::string> detector_files;

    // Read the entries in the folder
    while((entry = readdir(directory))) {

        // Ignore UNIX functional directories:
        if(std::string(entry->d_name).at(0) == '.') {
            continue;
        }

        // If these are folders then the name is the chip ID
        // For some file systems, dirent only returns DT_UNKNOWN - in this case, check the dir. entry starts with "W"
        if(entry->d_type == DT_DIR || (entry->d_type == DT_UNKNOWN && std::string(entry->d_name).at(0) == 'W')) {

            // Only read files from correct directory:
            if(entry->d_name != m_detector->name()) {
                continue;
            }
            LOG(DEBUG) << "Found directory for detector " << entry->d_name;

            // Open the folder for this device
            string dataDirName = m_inputDirectory + "/" + entry->d_name;
            DIR* dataDir = opendir(dataDirName.c_str());

            // Get all of the files for this chip
            while((file = readdir(dataDir))) {
                string filename = dataDirName + "/" + file->d_name;

                // Check if file has extension .dat
                if(string(file->d_name).find(".dat") != string::npos) {
                    LOG(INFO) << "Enqueuing data file for " << entry->d_name << ": " << filename;
                    detector_files.push_back(filename);
                }

                // If not a data file, it might be a trimdac file, with the list of masked pixels etc.
                if(string(file->d_name).find("trimdac") != string::npos) {
                    // Apply the pixel masking
                    maskPixels(filename);
                }
            }
        }
    }

    // Check that we have files for this detector and sort them correctly:
    if(detector_files.empty()) {
        LOG(ERROR) << "No data file found for detector " << m_detector->name() << " in input directory " << std::endl
                   << m_inputDirectory;
        return;
    }

    // Initialise null values for later
    m_syncTime = 0;
    m_clearedHeader = false;
    m_syncTimeTDC = 0;
    m_TDCoverflowCounter = 0;

    // Sort all files by extracting the "serial number" from the file name while ignoring the timestamp:
    std::sort(detector_files.begin(), detector_files.end(), [](std::string a, std::string b) {
        auto get_serial = [](std::string name) {
            const auto pos1 = name.find_last_of('-');
            const auto pos2 = name.find_last_of('.');
            return name.substr(pos1 + 1, pos2 - pos1 - 1);
        };
        return std::stoi(get_serial(a)) < std::stoi(get_serial(b));
    });

    // Open them:
    for(auto& filename : detector_files) {

        auto new_file = std::make_unique<std::ifstream>(filename);
        if(new_file->is_open()) {
            LOG(DEBUG) << "Opened data file for " << m_detector->name() << ": " << filename;

            // The header is repeated in every new data file, thus skip it for all.
            uint32_t headerID;
            if(!new_file->read(reinterpret_cast<char*>(&headerID), sizeof headerID)) {
                throw ModuleError("Cannot read header ID for " + m_detector->name() + " in file " + filename);
            }
            if(headerID != 1380208723) {
                throw ModuleError("Incorrect header ID for " + m_detector->name() + " in file " + filename + ": " +
                                  std::to_string(headerID));
            }
            LOG(TRACE) << "Header ID: \"" << headerID << "\"";

            // Skip the rest of the file header
            uint32_t headerSize;
            if(!new_file->read(reinterpret_cast<char*>(&headerSize), sizeof headerSize)) {
                throw ModuleError("Cannot read header size for " + m_detector->name() + " in file " + filename);
            }

            // Skip the full header:
            new_file->seekg(headerSize);
            LOG(TRACE) << "Skipped header (" << headerSize << "b)";

            // Store the file in the data vector:
            m_files.push_back(std::move(new_file));
        } else {
            throw ModuleError("Could not open data file " + filename);
        }
    }

    // Set the file iterator to the first file for every detector:
    m_file_iterator = m_files.begin();

    // Calibration
    pixelToT_beforecalibration = new TH1F("pixelToT_beforecalibration", "pixelToT_beforecalibration", 100, 0, 200);

    if(m_detector->isDUT() && m_config.has("calibration_path") && m_config.has("threshold")) {
        LOG(INFO) << "Applying calibration from " << calibrationPath;
        applyCalibration = true;

        // get DUT plane name
        std::string DUT = m_detector->name();

        // make paths to calibration files and read
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

        for(size_t row = 0; row < 256; row++) {
            for(size_t col = 0; col < 256; col++) {
                float a = vtot.at(256 * row + col).at(2);
                float b = vtot.at(256 * row + col).at(3);
                float c = vtot.at(256 * row + col).at(4);
                float t = vtot.at(256 * row + col).at(5);
                float toa_c = vtoa.at(256 * row + col).at(2);
                float toa_t = vtoa.at(256 * row + col).at(3);
                float toa_d = vtoa.at(256 * row + col).at(4);

                double cold = static_cast<double>(col);
                double rowd = static_cast<double>(row);
                pixelTOTParameterA->Fill(cold, rowd, a);
                pixelTOTParameterB->Fill(cold, rowd, b);
                pixelTOTParameterC->Fill(cold, rowd, c);
                pixelTOTParameterT->Fill(cold, rowd, t);
                pixelTOAParameterC->Fill(cold, rowd, toa_c);
                pixelTOAParameterD->Fill(cold, rowd, toa_d);
                pixelTOAParameterT->Fill(cold, rowd, toa_t);
            }
        }
    } else {
        LOG(INFO) << "No calibration file path or no DUT name given; data will be uncalibrated.";
        applyCalibration = false;
    }
    // Make debugging plots
    std::string title = m_detector->name() + " Hit map;x [px];y [px];pixels";
    hHitMap = new TH2F("hitMap",
                       title.c_str(),
                       m_detector->nPixels().X(),
                       0,
                       m_detector->nPixels().X(),
                       m_detector->nPixels().Y(),
                       0,
                       m_detector->nPixels().Y());
}

StatusCode EventLoaderTimepix3::run(std::shared_ptr<Clipboard> clipboard) {

    // This will loop through each timepix3 registered, and load data from each of them. This can
    // be done in one of two ways: by taking all data in the time interval (t,t+delta), or by
    // loading a fixed number of pixels (ie. 2000 at a time)

    // Check if event frame is defined:
    auto event = clipboard->get_event();

    LOG(TRACE) << "== New event";

    // If all files for this detector have been read, end the run:
    if(m_file_iterator == m_files.end()) {
        return StatusCode::Failure;
    }

    // Make a new container for the data
    auto deviceData = std::make_shared<PixelVector>();
    auto spidrData = std::make_shared<SpidrSignals>();

    // Load the next chunk of data
    bool data = loadData(clipboard, deviceData, spidrData);

    // If data was loaded then put it on the clipboard
    if(data) {
        LOG(DEBUG) << "Loaded " << deviceData->size() << " pixels for device " << m_detector->name();
        clipboard->put(deviceData, m_detector->name());
    }

    if(!spidrData->empty()) {
        clipboard->put(spidrData, m_detector->name());
    }

    // Otherwise tell event loop to keep running
    LOG_PROGRESS(DEBUG, "tpx3_loader") << "Current time: " << Units::display(event->start(), {"s", "ms", "us", "ns"});

    return StatusCode::Success;
}

// Function to load the pixel mask file
void EventLoaderTimepix3::maskPixels(string trimdacfile) {

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
                m_detector->maskChannel(t_col, t_row);
        }
    }

    // Close the files when finished
    trimdacs.close();
}

// Function to load calibration data
void EventLoaderTimepix3::loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat) {
    std::ifstream f;
    f.open(path);
    dat.clear();

    // check if file is open
    if(!f.is_open()) {
        LOG(ERROR) << "Cannot open input file:\n\t" << path;
        throw InvalidValueError(m_config, "calibration_path", "Parsing error in calibration file.");
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
        throw InvalidValueError(m_config, "calibration_path", "Parsing error in calibration file.");
    }

    f.close();
}

// Function to load data for a given device, into the relevant container
bool EventLoaderTimepix3::loadData(std::shared_ptr<Clipboard> clipboard,
                                   std::shared_ptr<PixelVector>& devicedata,
                                   std::shared_ptr<SpidrSignals>& spidrData) {

    std::string detectorID = m_detector->name();
    auto event = clipboard->get_event();

    bool extra = false; // temp

    LOG(DEBUG) << "Loading data for device " << detectorID;
    while(1) {
        // Check if current file is at its end and move to the next:
        if((*m_file_iterator)->eof()) {
            m_file_iterator++;
            LOG(INFO) << "Starting to read next file for " << detectorID << ": " << (*m_file_iterator).get();
        }

        // Check if the last file is finished:
        if(m_file_iterator == m_files.end()) {
            LOG(INFO) << "EOF for all files of " << detectorID;
            break;
        }

        // Now read the data packets.
        ULong64_t pixdata = 0;

        // If we can't read data anymore, jump to begin of loop:
        if(!(*m_file_iterator)->read(reinterpret_cast<char*>(&pixdata), sizeof pixdata)) {
            LOG(INFO) << "No more data in current file for " << detectorID << ": " << (*m_file_iterator).get();
            continue;
        }

        LOG(TRACE) << "0x" << hex << pixdata << dec << " - " << pixdata;

        // Get the header (first 4 bits) and do things depending on what it is
        // 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
        const UChar_t header = static_cast<UChar_t>((pixdata & 0xF000000000000000) >> 60) & 0xF;

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
                m_syncTime = (m_syncTime & 0xFFFFF00000000000) + ((pixdata & 0x0000FFFFFFFF0000) >> 4);
            }
            // 0x5 is the most significant part of the timestamp
            if(header2 == 0x5) {
                // The data is shifted 16 bits to the right, then 44 to the left in
                // order to match the timestamp format (net 28 left)
                m_syncTime = (m_syncTime & 0x00000FFFFFFFFFFF) + ((pixdata & 0x00000000FFFF0000) << 28);
                if(!m_clearedHeader && static_cast<double>(m_syncTime) / (4096. * 40000000.) < 6.) {
                    m_clearedHeader = true;
                    LOG(DEBUG) << detectorID << ": Cleared header";
                }
            }
        }

        // In data taking during 2015 there was sometimes still data left in the buffers at the start of
        // a run. For that reason we keep skipping data until this "header" data has been cleared, when
        // the heart beat signal starts from a low number (~few seconds max)
        if(!m_clearedHeader) {
            LOG(TRACE) << "Header not cleared, skipping data block.";
            continue;
        }

        // Header 0x06 and 0x07 are the start and stop signals for power pulsing
        if(header == 0x0) {
            // These packets should only come from the DUT. Otherwise ignore and throw warning.
            // (We observed these packets a few times per run in various telescope planes in the
            // November 2018 test beam.)
            if(!m_detector->isDUT()) {
                LOG(WARNING) << "Current time: " << Units::display(event->start(), {"s", "ms", "us", "ns"}) << " detector "
                             << detectorID << " "
                             << "header == 0x0! (indicates power pulsing.) Ignoring this.";
                continue;
            }
            // Note that the following code is probably outdated and/or not much tested
            // (Estel used her private code for her power-pulsing studies.) To be fixed!

            // Get the second part of the header
            const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

            // New implementation of power pulsing signals from Adrian
            if(header2 == 0x6) {
                LOG(TRACE) << "Found power pulsing - start";

                // Read time stamp and convert to nanoseconds
                const double timestamp = static_cast<double>((pixdata & 0x0000000FFFFFFFFF) << 12) / (4096 * 0.04);
                const uint64_t controlbits = ((pixdata & 0x00F0000000000000) >> 52) & 0xF;

                const uint64_t powerOn = ((controlbits & 0x2) >> 1);
                const uint64_t shutterClosed = ((controlbits & 0x1));

                // Stop looking at data if the signal is after the current event window
                // (and rewind the file
                // reader so that we start with this signal next event)
                if(timestamp > event->end()) {
                    (*m_file_iterator)->seekg(-1 * static_cast<int>(sizeof(pixdata)), std::ios_base::cur);
                    LOG(TRACE) << "Signal has a time beyond the current event: " << Units::display(timestamp, "ns");
                    break;
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
                long long int timestamp_raw = static_cast<long long int>(pixdata & 0xFFFFFFFFE00) >> 9;
                long long int timestamp = 0;
                // int triggerNumber = ((pixdata & 0xFFF00000000000) >> 44);
                int intermediate = (pixdata & 0x1F);
                if(intermediate != 0)
                    continue;

                // if jump back in time is larger than 1 sec, overflow detected...
                if((m_syncTimeTDC - timestamp_raw) > 0x1312d000) {
                    m_TDCoverflowCounter++;
                }
                m_syncTimeTDC = timestamp_raw;
                timestamp = timestamp_raw + (static_cast<long long int>(m_TDCoverflowCounter) << 35);

                double triggerTime =
                    static_cast<double>(timestamp + static_cast<long long int>(stamp) / 12) / (8. * 0.04); // 320 MHz clock
                SpidrSignal* triggerSignal = new SpidrSignal("trigger", triggerTime);
                spidrData->push_back(triggerSignal);
            }
        }

        // Header 0xA and 0xB indicate pixel data
        if(header == 0xA || header == 0xB) {
            LOG(TRACE) << "Found pixel data";

            // Decode the pixel information from the relevant bits
            const UShort_t dcol = static_cast<UShort_t>((pixdata & 0x0FE0000000000000) >> 52);
            const UShort_t spix = static_cast<UShort_t>((pixdata & 0x001F800000000000) >> 45);
            const UShort_t pix = static_cast<UShort_t>((pixdata & 0x0000700000000000) >> 44);
            const UShort_t col = static_cast<UShort_t>(dcol + pix / 4);
            const UShort_t row = static_cast<UShort_t>(spix + (pix & 0x3));

            // Check if this pixel is masked
            if(m_detector->masked(col, row)) {
                LOG(DEBUG) << "Detector " << detectorID << ": pixel " << col << "," << row << " masked";
                continue;
            }

            // Get the rest of the data from the pixel
            // const UShort_t pixno = col * 256 + row;
            const UInt_t data = static_cast<UInt_t>((pixdata & 0x00000FFFFFFF0000) >> 16);
            const unsigned int tot = (data & 0x00003FF0) >> 4;
            const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
            const uint64_t ftoa(data & 0x0000000F);
            const uint64_t toa((data & 0x0FFFC000) >> 14);

            // Calculate the timestamp.
            unsigned long long int time =
                (((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8) + (m_syncTime & 0xFFFFFC0000000000);

            // Adjusting phases for double column shift
            time += ((static_cast<unsigned long long int>(col) / 2 - 1) % 16) * 256;

            // The time from the pixels has a maximum value of ~26 seconds. We compare the pixel time to the "heartbeat"
            // signal (which has an overflow of ~4 years) and check if the pixel time has wrapped back around to 0

            // If the counter overflow happens before reading the new heartbeat
            //      while( abs(m_syncTime-time) > 0x0000020000000000 ){
            if(!extra) {
                while(static_cast<long long>(m_syncTime) - static_cast<long long>(time) > 0x0000020000000000) {
                    time += 0x0000040000000000;
                }
            } else {
                while(static_cast<long long>(m_prevTime) - static_cast<long long>(time) > 0x0000020000000000) {
                    time += 0x0000040000000000;
                }
            }

            // Convert final timestamp into ns and add the timing offset (in nano seconds) from the detectors file (if any)
            const double timestamp = static_cast<double>(time) / (4096. / 25.) + m_detector->timingOffset();

            // Ignore pixel data if it is before the "eventStart" read from the clipboard storage:
            if(timestamp < event->start()) {
                LOG(TRACE) << "Skipping pixel, is before event window (" << Units::display(timestamp, {"s", "us", "ns"})
                           << " < " << Units::display(event->start(), {"s", "us", "ns"}) << ")";
                continue;
            }

            // Stop looking at data if the pixel is after the current event window
            // (and rewind the file reader so that we start with this pixel next event)
            if(timestamp > event->end()) {
                LOG(DEBUG) << "Stopping processing event, pixel is after "
                              "event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " > "
                           << Units::display(event->end(), {"s", "us", "ns"}) << ")";
                (*m_file_iterator)->seekg(-1 * static_cast<int>(sizeof(pixdata)), std::ios_base::cur);
                break;
            }

            // Otherwise create a new pixel object
            pixelToT_beforecalibration->Fill(static_cast<int>(tot));

            // Apply calibration if applyCalibration is true
            if(applyCalibration && m_detector->isDUT()) {
                LOG(DEBUG) << "Applying calibration to DUT";
                size_t scol = static_cast<size_t>(col);
                size_t srow = static_cast<size_t>(row);
                float a = vtot.at(256 * srow + scol).at(2);
                float b = vtot.at(256 * srow + scol).at(3);
                float c = vtot.at(256 * srow + scol).at(4);
                float t = vtot.at(256 * srow + scol).at(5);

                float toa_c = vtoa.at(256 * srow + scol).at(2);
                float toa_t = vtoa.at(256 * srow + scol).at(3);
                float toa_d = vtoa.at(256 * srow + scol).at(4);

                // Calculating calibrated tot and toa
                float fvolts = (sqrt(a * a * t * t + 2 * a * b * t + 4 * a * c - 2 * a * t * static_cast<float>(tot) +
                                     b * b - 2 * b * static_cast<float>(tot) + static_cast<float>(tot * tot)) +
                                a * t - b + static_cast<float>(tot)) /
                               (2 * a);
                double fcharge = fvolts * 1e-3 * 3e-15 * 6241.509 * 1e15; // capacitance is 3 fF or 18.7 e-/mV

                /* Note 1: fvolts is the inverse to f(x) = a*x + b - c/(x-t). Note the +/- signs! */
                /* Note 2: The capacitance is actually smaller than 3 fC, more like 2.5 fC. But there is an offset when when
                 * using testpulses. Multiplying the voltage value with 20 [e-/mV] is a good approximation but means one is
                 * over estimating the input capacitance to compensate the missing information of the offset. */

                float t_shift = toa_c / (fvolts - toa_t) + toa_d;
                timeshiftPlot->Fill(static_cast<double>(Units::convert(t_shift, "ns")));
                const double ftimestamp = timestamp - t_shift;
                LOG(DEBUG) << "Time shift= " << Units::display(t_shift, {"s", "ns"});
                LOG(DEBUG) << "Timestamp calibrated = " << Units::display(ftimestamp, {"s", "ns"});

                if(col >= m_detector->nPixels().X() || row >= m_detector->nPixels().Y()) {
                    LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix.";
                }
                // creating new pixel object with calibrated values of tot and toa
                // when calibration is not available, set charge = tot
                Pixel* pixel = new Pixel(detectorID, col, row, static_cast<int>(tot), tot, ftimestamp);
                pixel->setCharge(fcharge);
                devicedata->push_back(pixel);
                hHitMap->Fill(col, row);
                LOG(DEBUG) << "Pixel Charge = " << fcharge << "; ToT value = " << tot;
                pixelToT_aftercalibration->Fill(fcharge);
            } else {
                LOG(DEBUG) << "Pixel hit at " << Units::display(timestamp, {"s", "ns"});
                // creating new pixel object with non-calibrated values of tot and toa
                // when calibration is not available, set charge = tot
                Pixel* pixel = new Pixel(detectorID, col, row, static_cast<int>(tot), tot, timestamp);
                devicedata->push_back(pixel);
                hHitMap->Fill(col, row);
            }

            m_prevTime = time;
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
