#include "EventLoaderATLASpix.h"
#include <regex>

using namespace corryvreckan;
using namespace std;

EventLoaderATLASpix::EventLoaderATLASpix(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_inputDirectory = m_config.getPath("input_directory");
    m_clockCycle = m_config.get<double>("clock_cycle", Units::get<double>(6.25, "ns"));

    // Allow reading of legacy data format using the Karlsruhe readout system:
    m_legacyFormat = m_config.get<bool>("legacy_format", false);

    // m_clkdivendM = m_config.get<int>("clkdivend", 0.) + 1;
    m_clkdivend2M = m_config.get<int>("clkdivend2", 0.) + 1;

    m_highToTCut = m_config.get<int>("high_tot_cut", 40);

    if(m_config.has("calibration_file")) {
        m_calibrationFile = m_config.getPath("calibration_file");
    }

    // ts1Range = 0x800 * m_clkdivendM;
    ts2Range = 0x40 * m_clkdivend2M;
}

uint32_t EventLoaderATLASpix::gray_decode(uint32_t gray) {
    uint32_t bin = gray;
    while(gray >>= 1) {
        bin ^= gray;
    }
    return bin;
}

void EventLoaderATLASpix::initialise() {

    uint32_t datain;

    m_detectorBusy = false;

    // File structure is RunX/data.bin
    // Assume that the ATLASpix is the DUT (if running this algorithm)

    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    dirent* entry;

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
    old_fpga_ts = 0;
    while(1) {
        m_file.read(reinterpret_cast<char*>(&datain), 4);
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
            old_fpga_ts = (static_cast<unsigned long long>(ts3));
            int checkpos = static_cast<int>(oldpos) - 8;
            while(checkpos >= 0) {
                std::streampos tmppos = checkpos;
                m_file.seekg(tmppos);
                m_file.read(reinterpret_cast<char*>(&datain), 4);
                unsigned int message_type = (datain >> 24);
                // TS2
                if(message_type == 0b00100000) {
                    old_fpga_ts = ((static_cast<unsigned long long>(datain & 0x00FFFFFF)) << 24) | ts3;
                    LOG(DEBUG) << "Set old_fpga_ts to " << old_fpga_ts;
                    break;
                }
                // TS3
                else if(message_type == 0b01100000) {
                    if(ts3 != (datain & 0x00FFFFFF)) {
                        LOG(WARNING) << "Last FPGA timestamp " << (datain & 0x00FFFFFF) << " does not match to T0 event "
                                     << ts3 << ". Some timestamps at the begining might be corrupted.";
                    }
                }
                checkpos = static_cast<int>(tmppos) - 4;
            }
            m_file.seekg(oldpos);
            break;
        }
    }

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap",
                       "hitMap; pixel column; pixel row; # events",
                       m_detector->nPixels().X(),
                       0,
                       m_detector->nPixels().X(),
                       m_detector->nPixels().Y(),
                       0,
                       m_detector->nPixels().Y());

    hHitMap_highTot = new TH2F("hitMap_highTot",
                               "hitMap_hithTot; pixel column; pixel row; # events",
                               m_detector->nPixels().X(),
                               0,
                               m_detector->nPixels().X(),
                               m_detector->nPixels().Y(),
                               0,
                               m_detector->nPixels().Y());

    hHitMap_totWeighted = new TProfile2D("hHitMap_totWeighted",
                                         "hHitMap_totWeighted; pixel column; pixel row; # events",
                                         m_detector->nPixels().X(),
                                         0,
                                         m_detector->nPixels().X(),
                                         m_detector->nPixels().Y(),
                                         0,
                                         m_detector->nPixels().Y(),
                                         0,
                                         100);

    hPixelToT = new TH1F("pixelToT", "pixelToT; pixel ToT in TS2 clock cycles; # events", 64, 0, 64);
    hPixelToT_beforeCorrection = new TH1F(
        "pixelToT_beforeCorrection", "pixelToT_beforeCorrection; pixel ToT in TS2 clock cycles; # events", 2 * 64, -64, 64);
    hPixelCharge = new TH1F("pixelCharge", "pixelCharge; pixel charge [e]; # events", 100, 0, 100);
    hPixelToA = new TH1F("pixelToA", "pixelToA; pixel ToA [ns]; # events", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame; pixels per frame; # events", 200, 0, 200);
    hPixelsOverTime = new TH1F("pixelsOverTime", "pixelsOverTime; time [ns]; # events", 3e6, 0, 3e9);

    hPixelTS1 = new TH1F("pixelTS1", "pixelTS1; pixel TS1 [lsb]; # events", 2050, 0, 2050);
    hPixelTS2 = new TH1F("pixelTS2", "pixelTS2; pixel TS2 [lsb]; # events", 130, 0, 130);
    hPixelTS1bits = new TH1F("pixelTS1bits", "pixelTS1bits; pixel TS1 bit [lsb->msb]; # events", 12, 0, 12);
    hPixelTS2bits = new TH1F("pixelTS2bits", "pixelTS2bits; pixel TS2 bit [lsb->msb]; # events", 8, 0, 8);

    hTriggersPerEvent = new TH1D("hTriggersPerEvent", "hTriggersPerEvent;triggers per event;entries", 20, 0, 20);

    // low ToT:
    hPixelTS1_lowToT = new TH1F("pixelTS1_lowToT", "pixelTS1_lowToT; pixel TS1 [lsb]; # events", 2050, 0, 2050);
    hPixelTS2_lowToT = new TH1F("pixelTS2_lowToT", "pixelTS2_lowToT; pixel TS2 [lsb]; # events", 130, 0, 130);
    hPixelTS1bits_lowToT =
        new TH1F("pixelTS1bits_lowToT", "pixelTS1bits_lowToT; pixel TS1 bit [lsb->msb]; # events", 12, 0, 12);
    hPixelTS2bits_lowToT =
        new TH1F("pixelTS2bits_lowToT", "pixelTS2bits_lowToT; pixel TS2 bit [lsb->msb]; # events", 8, 0, 8);

    // high ToT:
    hPixelTS1_highToT = new TH1F("pixelTS1_highToT", "pixelTS1_highToT; pixel TS1 [lsb]; # events", 2050, 0, 2050);
    hPixelTS2_highToT = new TH1F("pixelTS2_highToT", "pixelTS2_highToT; pixel TS2 [lsb]; # events", 130, 0, 130);
    hPixelTS1bits_highToT =
        new TH1F("pixelTS1bits_highToT", "pixelTS1bits_highToT; pixel TS1 bit [lsb->msb]; # events", 12, 0, 12);
    hPixelTS2bits_highToT =
        new TH1F("pixelTS2bits_highToT", "pixelTS2bits_highToT; pixel TS2 bit [lsb->msb]; # events", 8, 0, 8);

    hPixelTimeEventBeginResidual = new TH1F("hPixelTimeEventBeginResidual",
                                            "hPixelTimeEventBeginResidual;pixel_ts - clipboard event begin [us]; # entries",
                                            2.1e5,
                                            -10,
                                            200);
    hPixelTimeEventBeginResidualOverTime =
        new TH2F("hPixelTimeEventBeginResidualOverTime",
                 "hPixelTimeEventBeginResidualOverTime; pixel time [s];pixel_ts - clipboard event begin [us]",
                 3e3,
                 0,
                 3e3,
                 2.1e4,
                 -10,
                 200);

    std::string histTitle = "hPixelTriggerTimeResidualOverTime_0;time [us];trigger_ts - pixel_ts [us];# entries";
    hPixelTriggerTimeResidualOverTime =
        new TH2D("hPixelTriggerTimeResidualOverTime_0", histTitle.c_str(), 3e3, 0, 3e3, 1e4, -50, 50);

    // Read calibration:
    m_calibrationFactors.resize(static_cast<size_t>(m_detector->nPixels().X() * m_detector->nPixels().Y()), 1.0);
    if(!m_calibrationFile.empty()) {
        std::ifstream calibration(m_calibrationFile);
        std::string line;
        std::getline(calibration, line);

        int col, row;
        double calibfactor;
        while(getline(calibration, line)) {
            std::istringstream(line) >> col >> row >> calibfactor;
            m_calibrationFactors.at(static_cast<size_t>(row * 25 + col)) = calibfactor;
        }
        calibration.close();
    }

    LOG(INFO) << "Using clock cycle length of " << m_clockCycle << " ns." << std::endl;

    m_oldtoa = 0;
    m_overflowcounter = 0;
}

StatusCode EventLoaderATLASpix::run(std::shared_ptr<Clipboard> clipboard) {

    // Check if event frame is defined:
    auto event = clipboard->get_event();

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        LOG(DEBUG) << "Returning <Failure> status, ATLASPix data file reached the end.";
        return StatusCode::Failure;
    }

    double start_time = event->start();
    double end_time = event->end();
    bool busy_at_start = m_detectorBusy;

    // Read pixel data
    Pixels* pixels = (m_legacyFormat ? read_legacy_data(start_time, end_time) : read_caribou_data(start_time, end_time));

    if(busy_at_start || m_detectorBusy) {
        LOG(DEBUG) << "Returning <DeadTime> status, ATLASPix is BUSY.";
        return StatusCode::DeadTime;
    }

    // Here fill some histograms for data quality monitoring:
    auto nTriggers = event->triggerList().size();
    LOG(DEBUG) << "nTriggers = " << nTriggers;
    hTriggersPerEvent->Fill(static_cast<double>(nTriggers));

    for(auto px : (*pixels)) {
        hHitMap->Fill(px->column(), px->row());
        if(px->raw() > m_highToTCut) {
            hHitMap_highTot->Fill(px->column(), px->row());
        }
        hHitMap_totWeighted->Fill(px->column(), px->row(), px->raw());
        hPixelToT->Fill(px->raw());
        hPixelCharge->Fill(px->charge());
        hPixelToA->Fill(px->timestamp());

        hPixelTimeEventBeginResidual->Fill(static_cast<double>(Units::convert(px->timestamp() - start_time, "us")));
        hPixelTimeEventBeginResidualOverTime->Fill(static_cast<double>(Units::convert(px->timestamp(), "s")),
                                                   static_cast<double>(Units::convert(px->timestamp() - start_time, "us")));
        size_t iTrigger = 0;
        for(auto& trigger : event->triggerList()) {
            // check if histogram exists already, if not: create it
            if(hPixelTriggerTimeResidual.find(iTrigger) == hPixelTriggerTimeResidual.end()) {
                std::string histName = "hPixelTriggerTimeResidual_" + to_string(iTrigger);
                std::string histTitle = histName + ";trigger_ts - pixel_ts [us];# entries";
                hPixelTriggerTimeResidual[iTrigger] = new TH1D(histName.c_str(), histTitle.c_str(), 2e5, -100, 100);
            }
            // use iTrigger, not trigger ID (=trigger.first) (which is unique and continuously incrementing over the runtime)
            hPixelTriggerTimeResidual[iTrigger]->Fill(
                static_cast<double>(Units::convert(px->timestamp() - trigger.second, "us")));
            if(iTrigger == 0) { // fill only for 0th trigger
                hPixelTriggerTimeResidualOverTime->Fill(
                    static_cast<double>(Units::convert(px->timestamp(), "s")),
                    static_cast<double>(Units::convert(px->timestamp() - trigger.second, "us")));
            }
            iTrigger++;
        }
        hPixelsOverTime->Fill(static_cast<double>(Units::convert(px->timestamp(), "ns")));
    }

    hPixelsPerFrame->Fill(static_cast<double>(pixels->size()));

    // Put the data on the clipboard
    if(!pixels->empty()) {
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
    } else {
        delete pixels;
        LOG(DEBUG) << "Returning <NoData> status, no hits found.";
        return StatusCode::NoData;
    }

    // Return value telling analysis to keep running
    LOG(DEBUG) << "Returning <Success> status, hits found in this event window.";
    return StatusCode::Success;
}

Pixels* EventLoaderATLASpix::read_caribou_data(double start_time, double end_time) {
    LOG(DEBUG) << "Searching for events in interval from " << Units::display(start_time, {"s", "us", "ns"}) << " to "
               << Units::display(end_time, {"s", "us", "ns"}) << ", file read position " << m_file.tellg()
               << ", old_fpga_ts = " << old_fpga_ts << ".";

    // Pixel container
    Pixels* pixels = new Pixels();

    // Read file and load data
    uint32_t datain;
    long long ts_diff; // tmp

    // Initialize all to 0 for a case that hit data come before timestamp/trigger data
    long long hit_ts = 0;            // 64bit value of a hit timestamp combined from readout and pixel hit timestamp
    unsigned long long atp_ts = 0;   // 16bit value of ATLASpix readout timestamp
    unsigned long long trig_cnt = 0; // 32bit value of trigger counter (in FPGA)
    unsigned long long readout_ts =
        old_readout_ts;                       // 64bit value of a readout timestamp combined from FPGA and ATp timestamp
    unsigned long long fpga_ts = old_fpga_ts; // 64bit value of FPGA readout timestamp
    unsigned long long fpga_ts1 = 0;          // tmp [63:48] of FPGA readout timestamp
    unsigned long long fpga_ts2 = 0;          // tmp [47:24] of FPGA readout timestamp
    unsigned long long fpga_ts3 = 0;          // tmp [23:0] of FPGA readout timestamp
    unsigned long long fpga_tsx = 0;          // tmp for FPGA readout timestamp
    bool new_ts1 = true;
    bool new_ts2 = true;
    bool window_end = false;
    bool keep_pointer_stored = false;
    bool keep_reading = true;

    // Repeat until input EOF:
    while(keep_reading) {
        // Read next 4-byte data from file
        m_file.read(reinterpret_cast<char*>(&datain), 4);
        if(m_file.eof()) {
            LOG(DEBUG) << "EOF...";
            break;
        }

        // LOG(DEBUG) << "Found " << (datain & 0x80000000 ? "pixel data" : "header information");

        // Check if current word is a pixel data:
        if(datain & 0x80000000) {
            // Structure: {1'b1, column_addr[5:0], row_addr[8:0], rise_timestamp[9:0], fall_timestamp[5:0]}
            // Extract pixel data
            long ts2 = gray_decode((datain)&0x003F);
            // long ts2 = gray_decode((datain>>6)&0x003F);
            // TS1 counter is by default half speed of TS2. By multiplying with 2 we make it equal.
            long ts1 = (gray_decode((datain >> 6) & 0x03FF)) << 1;
            // long ts1 = (gray_decode(((datain << 4) & 0x3F0) | ((datain >> 12) & 0xF)))<<1;
            int row = ((datain >> (6 + 10)) & 0x01FF);
            int col = ((datain >> (6 + 10 + 9)) & 0x003F);
            // long tot = 0;

            ts_diff = ts1 - static_cast<long long>(readout_ts & 0x07FF);

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

            hit_ts = static_cast<long long>(readout_ts) + ts_diff;

            // Convert the timestamp to nanoseconds:
            double timestamp = m_clockCycle * static_cast<double>(hit_ts);

            if(timestamp > end_time) {
                keep_pointer_stored = true;
                LOG(DEBUG) << "Skipping processing event, pixel is after event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " > "
                           << Units::display(end_time, {"s", "us", "ns"}) << ")";
                continue;
            }

            if(timestamp < start_time) {
                LOG(DEBUG) << "Skipping pixel hit, pixel is before event window ("
                           << Units::display(timestamp, {"s", "us", "ns"}) << " < "
                           << Units::display(start_time, {"s", "us", "ns"}) << ")";
                continue;
            }
            // this window still contains data in the event window, do not stop processing
            window_end = false;
            if(m_detectorBusy && (busy_readout_ts < readout_ts)) {
                LOG(WARNING) << "ATLASPix went BUSY between "
                             << Units::display((m_clockCycle * static_cast<double>(busy_readout_ts)), {"s", "us", "ns"})
                             << " and "
                             << Units::display((m_clockCycle * static_cast<double>(readout_ts)), {"s", "us", "ns"}) << ".";
                m_detectorBusy = false;
            }
            data_pixel_++;
            // If this pixel is masked, do not save it
            if(m_detector->masked(col, row)) {
                continue;
            }

            // calculate ToT only when pixel is good for storing (division is time consuming)
            int tot = static_cast<int>(ts2 - ((hit_ts % static_cast<long long>(64 * m_clkdivend2M)) / m_clkdivend2M));
            hPixelToT_beforeCorrection->Fill(tot);
            if(tot < 0) {
                tot += 64;
            }
            // convert ToT to nanoseconds
            // double tot_ns = tot * m_clockCycle;

            hPixelTS1->Fill(static_cast<double>(ts1));
            hPixelTS2->Fill(static_cast<double>(ts2));
            if(tot < m_highToTCut) {
                hPixelTS1_lowToT->Fill(static_cast<double>(ts1));
                hPixelTS2_lowToT->Fill(static_cast<double>(ts2));
            } else {
                hPixelTS1_highToT->Fill(static_cast<double>(ts1));
                hPixelTS2_highToT->Fill(static_cast<double>(ts2));
            }

            // histogram each bit of the 10-bit TS1
            for(int i = 0; i < 12; i++) {
                hPixelTS1bits->Fill(static_cast<double>(i), static_cast<double>((ts1 >> i) & 0b1));
                if(tot < m_highToTCut) {
                    hPixelTS1bits_lowToT->Fill(static_cast<double>(i), static_cast<double>((ts1 >> i) & 0b1));
                } else {
                    hPixelTS1bits_highToT->Fill(static_cast<double>(i), static_cast<double>((ts1 >> i) & 0b1));
                }
            }
            // histogram each bit of the 6-bit TS2
            for(int i = 0; i < 8; i++) {
                hPixelTS2bits->Fill(static_cast<double>(i), static_cast<double>((ts2 >> i) & 0b1));
                if(tot < m_highToTCut) {
                    hPixelTS2bits_lowToT->Fill(static_cast<double>(i), static_cast<double>((ts1 >> i) & 0b1));
                } else {
                    hPixelTS2bits_highToT->Fill(static_cast<double>(i), static_cast<double>((ts1 >> i) & 0b1));
                }
            }

            LOG(TRACE) << "HIT: TS1: " << ts1 << "\t0x" << std::hex << ts1 << "\tTS2: " << ts2 << "\t0x" << std::hex << ts2
                       << "\tTS_FULL: " << hit_ts << "\t" << Units::display(timestamp, {"s", "us", "ns"})
                       << "\tTOT: " << tot; // << "\t" << Units::display(tot_ns, {"s", "us", "ns"});

            // when calibration is not available, set charge = tot
            Pixel* pixel = new Pixel(m_detector->name(), col, row, tot, tot, timestamp);

            // FIXME: implement conversion from ToT to charge:
            // thres-->e: 1620e/0.15V, or 1080e/100mV
            // How to get from ToT to charge?
            //
            // Do something like:
            // charge = charge_calibration(tot);
            // pixel->setCharge(charge);
            // pixel->setCalibrated = true;

            LOG(DEBUG) << "PIXEL:\t" << *pixel;
            pixels->push_back(pixel);

        } else {
            // data is not hit information
            // if (keep_pointer_stored) then we will go through the data one more time and wee will count that next time.
            if(!keep_pointer_stored) {
                data_header_++;
            }

            // Decode the message content according to 8 MSBits
            unsigned int message_type = (datain >> 24);
            switch(message_type) {
            // Timestamp from ATLASpix [23:0]
            case 0b01000000:
                // the whole previous readout was behind the time window, we can stop processing and return
                if(window_end) {
                    LOG(TRACE) << "Rewinding to file pointer : " << oldpos;
                    m_file.seekg(oldpos);
                    // exit the while loop
                    keep_reading = false;
                    // exit case
                    break;
                }

                atp_ts = (datain >> 7) & 0x1FFFE;
                ts_diff = static_cast<long long>(atp_ts) - static_cast<long long>(fpga_ts & 0x1FFFF);

                if(ts_diff > 0) {
                    if(ts_diff > 0x10000) {
                        ts_diff -= 0x20000;
                    }
                } else {
                    if(ts_diff < -0x1000) {
                        ts_diff += 0x20000;
                    }
                }
                readout_ts = static_cast<unsigned long long>(static_cast<long long>(fpga_ts) + ts_diff);

                if(!keep_pointer_stored) {
                    // Store this position in the file in case we need to rewind:
                    LOG(TRACE) << "Storing file pointer position: " << m_file.tellg() << " and readout TS: " << readout_ts;
                    oldpos = m_file.tellg();
                    old_readout_ts = readout_ts;
                } else {
                    LOG(TRACE) << "File pointer position already stored for the first event out of the window. Current "
                                  "readout_ts = "
                               << readout_ts;
                }
                // If the readout time is after the window, mark it as a candidate for last readout in the window
                if((static_cast<double>(readout_ts) * m_clockCycle) > end_time) {
                    window_end = true;
                }
                break;

            // Trigger counter from FPGA [23:0] (1/4)
            case 0b00010000:
                trig_cnt = datain & 0x00FFFFFF;
                break;

            // Trigger counter from FPGA [31:24] and timestamp from FPGA [63:48] (2/4)
            case 0b00110000:
                trig_cnt |= (datain << 8) & 0xFF000000;
                fpga_ts1 = ((static_cast<unsigned long long>(datain) << 48) & 0xFFFF000000000000);
                new_ts1 = true;
                break;

            // Timestamp from FPGA [47:24] (3/4)
            case 0b00100000:
                fpga_tsx = ((static_cast<unsigned long long>(datain) << 24) & 0x0000FFFFFF000000);
                if((!new_ts1) && (fpga_tsx < fpga_ts2)) {
                    fpga_ts1 += 0x0001000000000000;
                    LOG(WARNING) << "Missing TS_FPGA_1, adding one";
                }
                new_ts1 = false;
                new_ts2 = true;
                fpga_ts2 = fpga_tsx;
                break;

            // Timestamp from FPGA [23:0] (4/4)
            case 0b01100000:
                m_identifiers["FPGA_TS"]++;
                fpga_tsx = ((datain)&0xFFFFFF);
                if((!new_ts2) && (fpga_tsx < fpga_ts3)) {
                    fpga_ts2 += 0x0000000001000000;
                    LOG(WARNING) << "Missing TS_FPGA_2, adding one";
                }
                new_ts2 = false;
                fpga_ts3 = fpga_tsx;
                fpga_ts = fpga_ts1 | fpga_ts2 | fpga_ts3;
                break;

            // BUSY was asserted due to FIFO_FULL + 24 LSBs of FPGA timestamp when it happened
            case 0b00000010:
                m_identifiers["BUSY_ASSERT"]++;
                busy_readout_ts = readout_ts;
                m_detectorBusy = true;
                break;

            // T0 received
            case 0b01110000:
                LOG(WARNING) << "Another T0 event was found in the data at position " << m_file.tellg();
                break;

            // Empty data - should not happen
            case 0b00000000:
                m_identifiers["EMPTY_DATA"]++;
                LOG(DEBUG) << "EMPTY_DATA";
                break;

            // Other options...
            default:
                // LOG(DEBUG) << "...Other";
                // Unknown message identifier
                if(message_type & 0b11110010) {
                    m_identifiers["UNKNOWN_MESSAGE"]++;
                    LOG(DEBUG) << "UNKNOWN_MESSAGE";
                } else {
                    // Buffer for chip data overflow (data that came after this word were lost)
                    if((message_type & 0b11110011) == 0b00000001) {
                        m_identifiers["BUFFER_OVERFLOW"]++;
                        LOG(DEBUG) << "BUFFER_OVERFLOW";
                    }
                    // SERDES lock established (after reset or after lock lost)
                    if((message_type & 0b11111110) == 0b00001000) {
                        m_identifiers["SERDES_LOCK_ESTABLISHED"]++;
                        LOG(DEBUG) << "SERDES_LOCK_ESTABLISHED";
                    }
                    // SERDES lock lost (data might be nonsense, including up to 2 previous messages)
                    else if((message_type & 0b11111110) == 0b00001100) {
                        m_identifiers["SERDES_LOCK_LOST"]++;
                        LOG(DEBUG) << "SERDES_LOCK_LOST";
                    }
                    // Unexpected data came from the chip or there was a checksum error.
                    else if((message_type & 0b11111110) == 0b00000100) {
                        m_identifiers["WEIRD_DATA"]++;
                        LOG(DEBUG) << "WEIRD_DATA";
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

Pixels* EventLoaderATLASpix::read_legacy_data(double, double) {

    // Pixel container
    Pixels* pixels = new Pixels();

    // Read file and load data
    while(!m_file.eof()) {

        int col, row, tot, ts;
        unsigned long long int toa, TriggerDebugTS, dummy, bincounter;

        m_file >> col >> row >> ts >> tot >> dummy >> dummy >> bincounter >> TriggerDebugTS;

        // If this pixel is masked, do not save it
        if(m_detector->masked(col, row)) {
            continue;
        }

        // TOT
        if(tot <= (ts * 2 & 0x3F)) {
            tot = 64 + tot - (ts * 2 & 0x3F);
        } else {
            tot = tot - (ts * 2 & 0x3F);
        }

        // Apply calibration:
        double cal_tot = tot * m_calibrationFactors.at(static_cast<size_t>(row * 25 + col));
        LOG(TRACE) << "Hit " << row << "\t" << col << ": " << m_calibrationFactors.at(static_cast<size_t>(row * 25 + col))
                   << " * " << tot << " = " << cal_tot;

        ts &= 0xFF;
        ts *= 2; // atlaspix timestamp runs at 10MHz, multiply by to to get 20.

        if((bincounter & 0x1FF) < static_cast<unsigned long long>(ts)) {
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

        // TriggerDebugTS *= 4096. / 5;              // runs with 200MHz, divide by 5 to scale counter value to 40MHz
        double toa_timestamp =
            4096. * 2 * static_cast<double>(toa); // runs with 20MHz, multiply by 2 to scale counter value to 40MHz

        // Convert TOA to nanoseconds:
        toa_timestamp /= (4096. * 0.04);

        // when calibration is not available, set charge = tot
        Pixel* pixel = new Pixel(m_detector->name(), col, row, tot, tot, toa_timestamp);
        pixel->setCharge(cal_tot);
        pixels->push_back(pixel);
    }

    return pixels;
}

void EventLoaderATLASpix::finalise() {

    LOG(INFO) << "Identifier distribution:";
    for(auto id : m_identifiers) {
        LOG(INFO) << "\t" << id.first << ": " << id.second;
    }

    LOG(INFO) << "Found " << data_pixel_ << " pixel data blocks and " << data_header_ << " header words";
}
