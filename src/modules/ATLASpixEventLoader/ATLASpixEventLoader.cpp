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

    // m_clkdivendM = m_config.get<int>("clkdivend", 0.) + 1;
    m_clkdivend2M = m_config.get<int>("clkdivend2", 0.) + 1;

    // ts1Range = 0x800 * m_clkdivendM;
    ts2Range = 0x40 * m_clkdivend2M;
}

uint32_t ATLASpixEventLoader::gray_decode(uint32_t gray) {
    uint32_t bin = gray;
    while(gray >>= 1) {
        bin ^= gray;
    }
    return bin;
}

/*
long int ATLASpixEventLoader::ts_correction(long int coarseLSBs, long int fine, long int finePeriod, long int maxFwd) {
    long int diff = fine - coarseLSBs;
    if (diff > 0) {
        if (diff > maxFwd) {
            diff -= finePeriod;
        }
    }
    else {
        if (diff < (maxFwd-finePeriod) {
            diff += finePeriod;
        }
    }
}
*/

void ATLASpixEventLoader::initialise() {

    uint32_t datain;

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
        if(filename.find("data.bin") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0) {
        LOG(WARNING) << "No data file was found for ATLASpix in " << m_inputDirectory;
    } else {
        LOG(STATUS) << "Opened data file for ATLASpix: (dbg)" << m_filename;
    }

    // Open the binary data file for later
    m_file.open(m_filename.c_str(), ios::in | ios::binary);
    LOG(DEBUG) << "Opening file " << m_filename;

    // fast forward file to T0 event
    oldfpga_ts = 0;
    while(1) {
        m_file.read((char*)&datain, 4);
        if(m_file.eof()) {
            m_file.clear();
            m_file.seekg(ios::beg);
            LOG(WARNING) << "No T0 event was found in file " << m_filename
                         << ". Rewinding the file to the beginning and loading all events.";
            break;
        } else if((datain & 0xFF000000) == 0x70000000) {
            LOG(STATUS) << "Found T0 event at position " << m_file.tellg() << ". Skipping all data before this event.";
            oldpos = m_file.tellg();
            unsigned long ts3 = datain & 0x00FFFFFF;
            std::streampos tmppos = (int)oldpos - 8;
            m_file.seekg(tmppos);
            while((int)m_file.tellg() > 0) {
                m_file.read((char*)&datain, 4);
                unsigned int message_type = (datain >> 24);
                // TS2
                if(message_type == 0b00100000) {
                    oldfpga_ts = (((unsigned long long)(datain & 0x00FFFFFF)) << 24) | ts3;
                    LOG(DEBUG) << "Set oldfpga_ts to " << oldfpga_ts;
                    break;
                }
                // TS3
                else if(message_type == 0b01100000) {
                    if(ts3 != (datain & 0x00FFFFFF)) {
                        LOG(WARNING) << "Last FPGA timestamp " << (datain & 0x00FFFFFF) << " does not match to T0 event "
                                     << ts3 << ". Some timestamps at the begining might be corrupted.";
                    }
                }
                tmppos = (int)tmppos - 4;
                m_file.seekg(tmppos);
            }
            m_file.seekg(oldpos);
            break;
        }
    }

    // Make histograms for debugging
    auto det = get_detector(m_detectorID);
    hHitMap = new TH2F("hitMap", "hitMap", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    //    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 256, 0, 256);
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
    LOG(DEBUG) << "Searching for events in interval from " << Units::display(start_time, {"s", "us", "ns"}) << " to "
               << Units::display(end_time, {"s", "us", "ns"}) << ", file read position " << m_file.tellg()
               << ", oldfpga_ts = " << oldfpga_ts << ".";

    // Pixel container
    Pixels* pixels = new Pixels();

    // Detector we're looking at:
    auto detector = get_detector(m_detectorID);

    // double timestamp = 0;

    // Read file and load data
    uint32_t datain;
    long ts_diff; // tmp

    // *TBD: Can be cleared only in the first call and kept for next call:
    // Initialize all to 0 for a case that hit data come before timestamp/trigger data
    long long hit_ts = 0;                    // 64bit value of a hit timestamp combined from readout and pixel hit timestamp
    unsigned long atp_ts = 0;                // 16bit value of ATLASpix readout timestamp
    unsigned long trig_cnt = 0;              // 32bit value of trigger counter (in FPGA)
    unsigned long long readout_ts = 0;       // 64bit value of a readout timestamp combined from FPGA and ATp timestamp
    unsigned long long fpga_ts = oldfpga_ts; // 64bit value of FPGA readout timestamp
    unsigned long long fpga_ts1 = 0;         // tmp [63:48] of FPGA readout timestamp
    unsigned long long fpga_ts2 = 0;         // tmp [47:24] of FPGA readout timestamp
    unsigned long long fpga_ts3 = 0;         // tmp [23:0] of FPGA readout timestamp
    unsigned long long fpga_tsx = 0;         // tmp for FPGA readout timestamp
    bool new_ts1 = true;
    bool new_ts2 = true;

    // Repeat until input EOF:
    while(true) {
        // Read next 4-byte data from file
        m_file.read((char*)&datain, 4);
        if(m_file.eof()) {
            LOG(DEBUG) << "EOF...";
            break;
        }

        // LOG(DEBUG) << "Found " << (datain & 0x80000000 ? "pixel data" : "header information");

        // Check if current word is a pixel data:
        if(datain & 0x80000000) {
            data_pixel_++;

            // Structure: {1'b1, column_addr[5:0], row_addr[8:0], rise_timestamp[9:0], fall_timestamp[5:0]}
            // Extract pixel data
            long ts2 = gray_decode((datain)&0x003F);
            // long ts2 = gray_decode((datain>>6)&0x003F);
            // TS1 counter is by default half speed of TS2. By multiplying with 2 we make it equal.
            long ts1 = (gray_decode((datain >> 6) & 0x03FF)) << 1;
            // long ts1 = (gray_decode(((datain << 4) & 0x3F0) | ((datain >> 12) & 0xF)))<<1;
            long row = ((datain >> (6 + 10)) & 0x01FF);
            long col = ((datain >> (6 + 10 + 9)) & 0x003F);
            // long tot = 0;

            // correction for clkdivend
            // ts1 *= (m_clkdivendM);
            // correction for clkdivend2
            ts2 *= (m_clkdivend2M);

            ts_diff = ts1 - (readout_ts & 0x07FF);

            if(ts_diff > 0) {
                // Hit probably came before readout started and meanwhile an OVF of TS1 happened
                if(ts_diff > 0x01FF) {
                    ts_diff -= 0x0800;
                }
                // Hit probably came after readout started and is within range.
                // else {
                //    // OK...
                //}
            } else {
                // Hit probably came after readout started and after OVF of TS1.
                if(ts_diff < (0x01FF - 0x0800)) {
                    ts_diff += 0x0800;
                }
                // Hit probably came before readout started and is within range.
                // else {
                //    // OK...
                //}
            }

            hit_ts = readout_ts + ts_diff;

            // hit_ts = (readout_ts & 0xFFFFFFFFFFFFF800) + ts1;

            long tot = ts2 - (ts1 % ts2Range);
            if(tot < 0) {
                tot += ts2Range;
            }

            // Convert the timestamp to nanoseconds:
            double timestamp = hit_ts * m_clockCycle;
            // double tot_ns = tot * m_clockCycle;

            LOG(DEBUG) << "HIT_TS1:\t" << ts1 << "\t" << std::hex << ts1;
            LOG(DEBUG) << "HIT_TS2:\t" << ts2 << "\t" << std::hex << ts2;
            LOG(DEBUG) << "HIT_TS:\t" << hit_ts << "\t" << Units::display(timestamp, {"s", "us", "ns"});
            LOG(DEBUG) << "HIT_TOT:\t" << tot; // << "\t" << Units::display(tot_ns, {"s", "us", "ns"});

            // Stop looking at data if the pixel is after the current event window
            // (and rewind the file reader so that we start with this pixel next event)
            if(timestamp > end_time) {
                LOG(DEBUG) << "Stopping processing event, pixel is after event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " > "
                           << Units::display(end_time, {"s", "us", "ns"}) << ")";
                // Rewind to previous position:
                LOG(TRACE) << "Rewinding to file pointer : " << oldpos;
                m_file.seekg(oldpos);
                data_pixel_--;
                // fpga_ts = oldfpga_ts;

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
            LOG(DEBUG) << "PIXEL:\t" << *pixel;
            pixels->push_back(pixel);

        } else {
            data_header_++;

            // Decode the message content according to 8 MSBits
            unsigned int message_type = (datain >> 24);
            switch(message_type) {
            // Timestamp from ATLASpix [23:0]
            case 0b01000000:
                // uint32_t atp_ts_g, atp_ts_b;
                // atp_ts_g = gray_decode(datain & 0xFF);
                // atp_ts_b = (datain >> 8) & 0x3FF;

                atp_ts = (datain >> 7) & 0x1FFFE;
                ts_diff = atp_ts - (fpga_ts & 0x1FFFF);

                if(ts_diff > 0) {
                    if(ts_diff > 0x10000) {
                        ts_diff -= 0x20000;
                    }
                } else {
                    if(ts_diff < -0x1000) {
                        ts_diff += 0x20000;
                    }
                }
                readout_ts = fpga_ts; // + ts_diff;

                LOG(DEBUG) << "ATP_TS:\t" << atp_ts << "\t" << std::hex << atp_ts;
                LOG(DEBUG) << "READOUT_TS:\t" << readout_ts << "\t" << std::hex << readout_ts;
                LOG(DEBUG) << "TS_DIFF:\t" << ts_diff << "\t" << std::hex << ts_diff;
                break;

            // Trigger counter from FPGA [23:0] (1/4)
            case 0b00010000:
                // LOG(DEBUG) << "...FPGA_TS 1/4";
                trig_cnt = datain & 0x00FFFFFF;

                LOG(DEBUG) << "TRG_FPGA_1:\t" << trig_cnt << "\t" << std::hex << trig_cnt;
                break;

            // Trigger counter from FPGA [31:24] and timestamp from FPGA [63:48] (2/4)
            case 0b00110000:
                trig_cnt |= (datain << 8) & 0xFF000000;
                fpga_ts1 = (((unsigned long long)datain << 48) & 0xFFFF000000000000);
                // LOG(DEBUG) << "TRIGGER\t" << trig_cnt;

                LOG(DEBUG) << "TRG_FPGA_2:\t" << trig_cnt << "\t" << std::hex << trig_cnt;
                LOG(DEBUG) << "TS_FPGA_1:\t" << fpga_ts1 << "\t" << std::hex << fpga_ts1;
                new_ts1 = true;
                break;

            // Timestamp from FPGA [47:24] (3/4)
            case 0b00100000:
                fpga_tsx = (((unsigned long long)datain << 24) & 0x0000FFFFFF000000);
                LOG(DEBUG) << "TS_FPGA_2:\t" << fpga_tsx << "\t" << std::hex << fpga_tsx;
                if((!new_ts1) && (fpga_tsx < fpga_ts2)) {
                    fpga_ts1 += 0x0001000000000000;
                    LOG(DEBUG) << "TS_FPGA_1: ADDING_ONE";
                }
                new_ts1 = false;
                new_ts2 = true;
                fpga_ts2 = fpga_tsx;
                break;

            // Timestamp from FPGA [23:0] (4/4)
            case 0b01100000:
                m_identifiers["FPGA_TS"]++;
                fpga_tsx = ((datain)&0xFFFFFF);
                LOG(DEBUG) << "TS_FPGA_3:\t" << fpga_tsx << "\t" << std::hex << fpga_tsx;
                if((!new_ts2) && (fpga_tsx < fpga_ts3)) {
                    fpga_ts2 += 0x0000000001000000;
                    LOG(DEBUG) << "TS_FPGA_2: ADDING_ONE";
                }
                new_ts2 = false;
                fpga_ts3 = fpga_tsx;
                fpga_ts = fpga_ts1 | fpga_ts2 | fpga_ts3;
                LOG(DEBUG) << "TS_FPGA_M:\t" << fpga_ts << "\t" << std::hex << fpga_ts;

                // Store this position in the file in case we need to rewind:
                LOG(TRACE) << "Storing file pointer position: " << m_file.tellg();
                oldpos = m_file.tellg();
                oldfpga_ts = fpga_ts;

                break;

            // BUSY was asserted due to FIFO_FULL + 24 LSBs of FPGA timestamp when it happened
            case 0b00000010:
                m_identifiers["BUSY_ASSERT"]++;

                // LOG(DEBUG) << "BUSY_ASSERTED\t" << ((datain)&0xFFFFFF);
                break;

            // T0 received
            case 0b01110000:
                LOG(WARNING) << "Another T0 event was found in the data at position " << m_file.tellg();
                break;

            // Empty data - should not happen
            case 0b00000000:
                m_identifiers["EMPTY_DATA"]++;
                // LOG(DEBUG) << "...Emtpy";
                // LOG(DEBUG) << "EMPTY_DATA";
                break;

            // Other options...
            default:
                // LOG(DEBUG) << "...Other";
                // Unknown message identifier
                if(message_type & 0b11110010) {
                    m_identifiers["UNKNOWN_MESSAGE"]++;
                    // LOG(DEBUG) << "UNKNOWN_MESSAGE";
                } else {
                    // Buffer for chip data overflow (data that came after this word were lost)
                    if((message_type & 0b11110011) == 0b00000001) {
                        m_identifiers["BUFFER_OVERFLOW"]++;
                        // LOG(DEBUG) << "BUFFER_OVERFLOW";
                    }
                    // SERDES lock established (after reset or after lock lost)
                    if((message_type & 0b11111110) == 0b00001000) {
                        m_identifiers["SERDES_LOCK_ESTABLISHED"]++;
                        // LOG(DEBUG) << "SERDES_LOCK_ESTABLISHED";
                    }
                    // SERDES lock lost (data might be nonsense, including up to 2 previous messages)
                    else if((message_type & 0b11111110) == 0b00001100) {
                        m_identifiers["SERDES_LOCK_LOST"]++;
                        // LOG(DEBUG) << "SERDES_LOCK_LOST";
                    }
                    // Unexpected data came from the chip or there was a checksum error.
                    else if((message_type & 0b11111110) == 0b00000100) {
                        m_identifiers["WEIRD_DATA"]++;
                        // LOG(DEBUG) << "WEIRD_DATA";
                    }
                    // Unknown message identifier
                    else {
                        m_identifiers["UNKNOWN_MESSAGE"]++;
                        LOG(WARNING) << "UNKNOWN_MESSAGE";
                    }
                }
                break;
                // End case
            }
        }
    }
    LOG(DEBUG) << "Returning " << pixels->size() << " pixels";
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

    LOG(INFO) << "Found " << data_pixel_ << " pixel data blocks and " << data_header_ << " header words";
}
