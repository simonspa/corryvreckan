/**
 * @file
 * @brief Implementation of module EventLoaderATLASpix
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderATLASpix.h"
#include <regex>

using namespace corryvreckan;
using namespace std;

EventLoaderATLASpix::EventLoaderATLASpix(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    m_inputDirectory = config_.getPath("input_directory");

    config_.setDefault<double>("clock_cycle", Units::get<double>(6.25, "ns"));
    // config_.setDefault<int>("clkdivend", 0.);
    config_.setDefault<int>("clkdivend2", 0.);
    config_.setDefault<int>("high_tot_cut", 40);
    config_.setDefault<int>("buffer_depth", 1000);
    config_.setDefault<double>("time_offset", 0.);

    m_clockCycle = config_.get<double>("clock_cycle");
    // m_clkdivendM = config_.get<int>("clkdivend") + 1;
    m_clkdivend2M = config_.get<int>("clkdivend2") + 1;
    m_highToTCut = config_.get<int>("high_tot_cut");
    m_buffer_depth = config_.get<int>("buffer_depth");
    m_time_offset = config_.get<double>("time_offset");

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

void EventLoaderATLASpix::initialize() {

    if(m_buffer_depth < 1) {
        throw InvalidValueError(config_, "buffer_depth", "Buffer depth must be larger than 0.");
    } else {
        LOG(INFO) << "Using buffer_depth = " << m_buffer_depth;
    }

    // File structure is RunX/data.bin
    // Assume that the ATLASpix is the DUT (if running this algorithm)

    // Open the root directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        throw ModuleError("Directory " + m_inputDirectory + " does not exist");
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

    // If no data was loaded, throw an exception
    if(m_filename.length() == 0) {
        throw ModuleError("No data file was found for ATLASpix in " + m_inputDirectory);
    }
    LOG(STATUS) << "Opened data file for ATLASpix: (dbg)" << m_filename;

    // Open the binary data file for later
    m_file.open(m_filename.c_str(), ios::in | ios::binary);
    LOG(DEBUG) << "Opening file " << m_filename;

    std::string title = m_detector->getName() + ": number of different messages;message type;# events";
    hMessages = new TH1F("hMessages", title.c_str(), 5, 1, 6);
    hMessages->GetXaxis()->SetBinLabel(1, "UNKNOWN_MESSAGE");
    hMessages->GetXaxis()->SetBinLabel(2, "BUFFER_OVERFLOW");
    hMessages->GetXaxis()->SetBinLabel(3, "SERDES_LOCK_ESTABLISHED");
    hMessages->GetXaxis()->SetBinLabel(4, "SERDES_LOCK_LOST");
    hMessages->GetXaxis()->SetBinLabel(5, "WEIRD_DATA");

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap",
                       "hitMap; pixel column; pixel row; # events",
                       m_detector->nPixels().X(),
                       -0.5,
                       m_detector->nPixels().X() - 0.5,
                       m_detector->nPixels().Y(),
                       -0.5,
                       m_detector->nPixels().Y() - 0.5);

    hHitMap_highTot = new TH2F("hitMap_highTot",
                               "hitMap_hithTot; pixel column; pixel row; # events",
                               m_detector->nPixels().X(),
                               -0.5,
                               m_detector->nPixels().X() - 0.5,
                               m_detector->nPixels().Y(),
                               -0.5,
                               m_detector->nPixels().Y() - 0.5);

    hHitMap_totWeighted = new TProfile2D("hHitMap_totWeighted",
                                         "hHitMap_totWeighted; pixel column; pixel row; # events",
                                         m_detector->nPixels().X(),
                                         -0.5,
                                         m_detector->nPixels().X() - 0.5,
                                         m_detector->nPixels().Y(),
                                         -0.5,
                                         m_detector->nPixels().Y() - 0.5,
                                         0,
                                         100);

    hPixelToT = new TH1F("pixelToT", "pixelToT; pixel ToT in TS2 clock cycles; # events", 64, -0.5, 63.5);
    hPixelToT_beforeCorrection = new TH1F("pixelToT_beforeCorrection",
                                          "pixelToT_beforeCorrection; pixel ToT in TS2 clock cycles; # events",
                                          2 * 64,
                                          -64.5,
                                          63.5);
    hPixelCharge = new TH1F("pixelCharge", "pixelCharge; pixel charge [e]; # events", 100, -0.5, 99.5);
    hPixelToA = new TH1F("pixelToA", "pixelToA; pixel ToA [ns]; # events", 100, 0, 100);
    hPixelMultiplicity = new TH1F("pixelMultiplicity", "Pixel Multiplicity; # pixels; # events", 200, -0.5, 199.5);
    hPixelTimes = new TH1F("hPixelTimes", "pixelTimes; hit timestamp [ms]; # events", 3e6, 0, 3e3);
    hPixelTimes_long = new TH1F("hPixelTimes_long", "pixelTimes_long; hit timestamp [s]; # events", 3e6, 0, 3e3);

    hPixelTS1 = new TH1F("pixelTS1", "pixelTS1; pixel TS1 [lsb]; # events", 2050, -0.5, 2049.5);
    hPixelTS2 = new TH1F("pixelTS2", "pixelTS2; pixel TS2 [lsb]; # events", 130, -0.5, 129.5);
    hPixelTS1bits = new TH1F("pixelTS1bits", "pixelTS1bits; pixel TS1 bit [lsb->msb]; # events", 12, -0.5, 11.5);
    hPixelTS2bits = new TH1F("pixelTS2bits", "pixelTS2bits; pixel TS2 bit [lsb->msb]; # events", 8, -0.5, 7.5);

    hTriggersPerEvent = new TH1D("hTriggersPerEvent", "hTriggersPerEvent;triggers per event;entries", 20, -0.5, 19.5);

    // low ToT:
    hPixelTS1_lowToT = new TH1F("pixelTS1_lowToT", "pixelTS1_lowToT; pixel TS1 [lsb]; # events", 2050, -0.5, 2049.5);
    hPixelTS2_lowToT = new TH1F("pixelTS2_lowToT", "pixelTS2_lowToT; pixel TS2 [lsb]; # events", 130, -0.5, 129.5);
    hPixelTS1bits_lowToT =
        new TH1F("pixelTS1bits_lowToT", "pixelTS1bits_lowToT; pixel TS1 bit [lsb->msb]; # events", 12, -0.5, 11.5);
    hPixelTS2bits_lowToT =
        new TH1F("pixelTS2bits_lowToT", "pixelTS2bits_lowToT; pixel TS2 bit [lsb->msb]; # events", 8, -0.5, 7.5);

    // high ToT:
    hPixelTS1_highToT = new TH1F("pixelTS1_highToT", "pixelTS1_highToT; pixel TS1 [lsb]; # events", 2050, -0.5, 2049.5);
    hPixelTS2_highToT = new TH1F("pixelTS2_highToT", "pixelTS2_highToT; pixel TS2 [lsb]; # events", 130, -0.5, 129.5);
    hPixelTS1bits_highToT =
        new TH1F("pixelTS1bits_highToT", "pixelTS1bits_highToT; pixel TS1 bit [lsb->msb]; # events", 12, -0.5, 11.5);
    hPixelTS2bits_highToT =
        new TH1F("pixelTS2bits_highToT", "pixelTS2bits_highToT; pixel TS2 bit [lsb->msb]; # events", 8, -0.5, 7.5);

    hPixelTimeEventBeginResidual = new TH1F("hPixelTimeEventBeginResidual",
                                            "hPixelTimeEventBeginResidual;pixel_ts - clipboard event begin [us]; # entries",
                                            2.1e5,
                                            -10,
                                            200);
    hPixelTimeEventBeginResidual_wide =
        new TH1F("hPixelTimeEventBeginResidual_wide",
                 "hPixelTimeEventBeginResidual_wide;pixel_ts - clipboard event begin [us]; # entries",
                 1e5,
                 -5000,
                 5000);
    hPixelTimeEventBeginResidualOverTime =
        new TH2F("hPixelTimeEventBeginResidualOverTime",
                 "hPixelTimeEventBeginResidualOverTime; pixel time [s];pixel_ts - clipboard event begin [us]",
                 3e3,
                 0,
                 3e3,
                 2.1e4,
                 -10,
                 200);

    std::string histTitle = "hPixelTriggerTimeResidualOverTime_0;time [s];pixel_ts - trigger_ts [us];# entries";
    hPixelTriggerTimeResidualOverTime =
        new TH2D("hPixelTriggerTimeResidualOverTime_0", histTitle.c_str(), 3e3, 0, 3e3, 1e4, -50, 50);

    LOG(INFO) << "Using clock cycle length of " << m_clockCycle << " ns." << std::endl;

    eof_reached = false;
}

StatusCode EventLoaderATLASpix::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Check if event frame is defined:
    if(!clipboard->isEventDefined()) {
        LOG(WARNING) << "No event defined on clipboard. Make sure an event is defined before this eventloader.";
        return StatusCode::Failure;
    }
    auto event = clipboard->getEvent();

    double start_time = event->start();
    double end_time = event->end();

    // prepare pixels vector
    PixelVector pixels;
    while(true) {

        if(sorted_pixels_.empty() && eof_reached) {
            // break while loop but still go until the end of the run function
            LOG(TRACE) << "break while(true) --> end of file reached";
            break;
        }

        // read data from file and fill timesorted buffer
        while(static_cast<int>(sorted_pixels_.size()) < m_buffer_depth && !eof_reached) {
            // read_caribou_data returns false when EOF is reached and true otherwise
            if(!read_caribou_data()) {
                LOG(TRACE) << "read_caribou_data returns false: reached EOF.";
                break;
            }
        }

        // get next pixel from sorted queue
        auto pixel = sorted_pixels_.top();

        if(pixel->timestamp() > end_time) {
            LOG(DEBUG) << "Keep pixel for next event, pixel (" << pixel->column() << "," << pixel->row()
                       << ") is after event window (" << Units::display(pixel->timestamp(), {"s", "us", "ns"}) << " > "
                       << Units::display(end_time, {"s", "us", "ns"}) << ")";
            break;
        }

        // if before or during:
        if(!sorted_pixels_.empty()) {
            LOG(TRACE) << "buffer size = " << sorted_pixels_.size() << " --> pop()";
            // remove pixel from sorted queue
            sorted_pixels_.pop();
        } else {
            continue;
        }

        if(pixel->timestamp() < start_time) {
            LOG(DEBUG) << "Skipping pixel hit, pixel is before event window ("
                       << Units::display(pixel->timestamp(), {"s", "us", "ns"}) << " < "
                       << Units::display(start_time, {"s", "us", "ns"}) << ")";
            continue;
        }

        // add to vector of pixels
        LOG(DEBUG) << "Pixel is during event: (" << pixel->column() << ", " << pixel->row()
                   << ") ts: " << Units::display(pixel->timestamp(), {"ns", "us", "ms"});
        pixels.push_back(pixel);

        // fill all per-pixel histograms:
        hHitMap->Fill(pixel->column(), pixel->row());
        if(pixel->raw() > m_highToTCut) {
            hHitMap_highTot->Fill(pixel->column(), pixel->row());
        }
        hHitMap_totWeighted->Fill(pixel->column(), pixel->row(), pixel->raw());
        hPixelToT->Fill(pixel->raw());
        hPixelCharge->Fill(pixel->charge());
        hPixelToA->Fill(pixel->timestamp());

        hPixelTimeEventBeginResidual->Fill(static_cast<double>(Units::convert(pixel->timestamp() - start_time, "us")));
        hPixelTimeEventBeginResidual_wide->Fill(static_cast<double>(Units::convert(pixel->timestamp() - start_time, "us")));
        hPixelTimeEventBeginResidualOverTime->Fill(
            static_cast<double>(Units::convert(pixel->timestamp(), "s")),
            static_cast<double>(Units::convert(pixel->timestamp() - start_time, "us")));
        size_t iTrigger = 0;
        for(auto& trigger : event->triggerList()) {
            // check if histogram exists already, if not: create it
            if(hPixelTriggerTimeResidual.find(iTrigger) == hPixelTriggerTimeResidual.end()) {
                std::string histName = "hPixelTriggerTimeResidual_" + to_string(iTrigger);
                std::string histTitle = histName + ";pixel_ts - trigger_ts [us];# entries";
                hPixelTriggerTimeResidual[iTrigger] = new TH1D(histName.c_str(), histTitle.c_str(), 2e5, -100, 100);
            }
            // use iTrigger, not trigger ID (=trigger.first) (which is unique and continuously incrementing over the runtime)
            hPixelTriggerTimeResidual[iTrigger]->Fill(
                static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
            if(iTrigger == 0) { // fill only for 0th trigger
                hPixelTriggerTimeResidualOverTime->Fill(
                    static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                    static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
            }
            iTrigger++;
        }
        hPixelTimes->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "ms")));
        hPixelTimes_long->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "s")));
    }

    // Here fill some histograms for data quality monitoring:
    auto nTriggers = event->triggerList().size();
    LOG(DEBUG) << "nTriggers = " << nTriggers;
    hTriggersPerEvent->Fill(static_cast<double>(nTriggers));

    hPixelMultiplicity->Fill(static_cast<double>(pixels.size()));

    // Put the data on the clipboard
    clipboard->putData(pixels, m_detector->getName());

    if(sorted_pixels_.empty() && eof_reached) {
        LOG(DEBUG) << "Reached EOF";
        return StatusCode::EndRun;
    }

    if(pixels.empty()) {
        LOG(DEBUG) << "Returning <NoData> status, no hits found.";
        return StatusCode::NoData;
    }

    // Return value telling analysis to keep running
    LOG(DEBUG) << "Returning <Success> status, hits found in this event window.";
    return StatusCode::Success;
}

bool EventLoaderATLASpix::read_caribou_data() { // return false when reaching eof

    // Read file and load data
    uint32_t datain;

    // Read next 4-byte data from file
    m_file.read(reinterpret_cast<char*>(&datain), 4);
    if(m_file.eof()) {
        LOG(TRACE) << "EOF...";
        eof_reached = true;
        return false;
    }

    // Check if current word is a pixel data:
    if(datain & 0x80000000) {
        // Do not return and decode pixel data before T0 arrived
        if(t0_seen_ == 0) {
            return true;
        } else if(t0_seen_ > 1) {
            LOG(ERROR) << "Detected 2nd T0 signal. Finish the reconstruction and throw this event away!";
            eof_reached = true;
            return false;
        }
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

        long long ts_diff = ts1 - static_cast<long long>(readout_ts_ & 0x07FF);

        if(ts_diff > 0) {
            // Hit probably came before readout started and meanwhile an OVF of TS1 happened
            if(ts_diff > 0x01FF) {
                ts_diff -= 0x0800;
            }
        } else {
            // Hit probably came after readout started and after OVF of TS1.
            if(ts_diff < (0x01FF - 0x0800)) {
                ts_diff += 0x0800;
            }
        }

        long long hit_ts = static_cast<long long>(readout_ts_) + ts_diff;

        // Convert the timestamp to nanoseconds:
        double timestamp = m_clockCycle * static_cast<double>(hit_ts) + m_detector->timeOffset();

        // If this pixel is masked, do not save it
        if(m_detector->masked(col, row)) {
            return true;
        }

        // calculate ToT only when pixel is good for storing (division is time consuming)
        int tot = static_cast<int>(ts2 - ((hit_ts % static_cast<long long>(64 * m_clkdivend2M)) / m_clkdivend2M));
        hPixelToT_beforeCorrection->Fill(tot);
        if(tot < 0) {
            tot += 64;
        }

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

        if(col >= m_detector->nPixels().X() || row >= m_detector->nPixels().Y()) {
            LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix.";
            return true;
        }

        timestamp += m_time_offset;
        LOG(DEBUG) << "Adding time_offset of " << m_time_offset << " to pixel timestamp. New pixel timestamp: " << timestamp;

        // since calibration is not implemented yet, set charge = tot
        auto pixel = std::make_shared<Pixel>(m_detector->getName(), col, row, tot, tot, timestamp);

        // FIXME: implement conversion from ToT to charge:
        // thres-->e: 1620e/0.15V, or 1080e/100mV
        // How to get from ToT to charge?
        //
        // Do something like:
        // charge = charge_calibration(tot);
        // pixel->setCharge(charge);
        // pixel->setCalibrated = true;

        LOG(DEBUG) << "Adding to buffer: " << *pixel;
        sorted_pixels_.push(pixel);
        return true;

    } else {
        // data is not hit information

        // Decode the message content according to 8 MSBits
        unsigned int message_type = (datain >> 24);
        LOG(TRACE) << "Message type " << std::hex << message_type << std::dec;
        if(message_type == 0b01000000) {
            uint64_t atp_ts = (datain >> 7) & 0x1FFFE;
            long long ts_diff = static_cast<long long>(atp_ts) - static_cast<long long>(fpga_ts_ & 0x1FFFF);

            if(ts_diff > 0) {
                if(ts_diff > 0x10000) {
                    ts_diff -= 0x20000;
                }
            } else {
                if(ts_diff < -0x1000) {
                    ts_diff += 0x20000;
                }
            }
            readout_ts_ = static_cast<unsigned long long>(static_cast<long long>(fpga_ts_) + ts_diff);
            LOG(DEBUG) << "RO_ts " << std::hex << readout_ts_ << " atp_ts " << atp_ts << std::dec;
        } else if(message_type == 0b00010000) {
            // Trigger counter from FPGA [23:0] (1/4)
        } else if(message_type == 0b00110000) {
            // Trigger counter from FPGA [31:24] and timestamp from FPGA [63:48] (2/4)
            fpga_ts1_ = ((static_cast<unsigned long long>(datain) << 48) & 0xFFFF000000000000);
            new_ts1_ = true;
        } else if(message_type == 0b00100000) {

            // Timestamp from FPGA [47:24] (3/4)
            uint64_t fpga_tsx = ((static_cast<unsigned long long>(datain) << 24) & 0x0000FFFFFF000000);
            if((!new_ts1_) && (fpga_tsx < fpga_ts2_)) {
                fpga_ts1_ += 0x0001000000000000;
                LOG(DEBUG) << "Missing TS_FPGA_1, adding one";
            }
            new_ts1_ = false;
            new_ts2_ = true;
            fpga_ts2_ = fpga_tsx;
        } else if(message_type == 0b01100000) {

            // Timestamp from FPGA [23:0] (4/4)
            uint64_t fpga_tsx = ((datain)&0xFFFFFF);
            if((!new_ts2_) && (fpga_tsx < fpga_ts3_)) {
                fpga_ts2_ += 0x0000000001000000;
                LOG(DEBUG) << "Missing TS_FPGA_2, adding one";
            }
            new_ts2_ = false;
            fpga_ts3_ = fpga_tsx;
            fpga_ts_ = fpga_ts1_ | fpga_ts2_ | fpga_ts3_;
        } else if(message_type == 0b00000010) {
            // BUSY was asserted due to FIFO_FULL + 24 LSBs of FPGA timestamp when it happened
        } else if(message_type == 0b01110000) {
            // T0 received
            LOG(DEBUG) << "T0 event was found in the data";
            new_ts1_ = false;
            new_ts2_ = false;
            fpga_ts_ = 0;
            fpga_ts1_ = 0;
            fpga_ts2_ = 0;
            fpga_ts3_ = 0;
            t0_seen_++;
        } else if(message_type == 0b00000000) {

            // Empty data - should not happen
            LOG(DEBUG) << "EMPTY_DATA";
        } else {

            // Other options...
            // LOG(DEBUG) << "...Other";
            // Unknown message identifier
            if(message_type & 0b11110010) {
                LOG(DEBUG) << "UNKNOWN_MESSAGE";
                hMessages->Fill(1);
            } else {
                // Buffer for chip data overflow (data that came after this word were lost)
                if((message_type & 0b11110011) == 0b00000001) {
                    LOG(DEBUG) << "BUFFER_OVERFLOW";
                    hMessages->Fill(2);
                }
                // SERDES lock established (after reset or after lock lost)
                if((message_type & 0b11111110) == 0b00001000) {
                    LOG(DEBUG) << "SERDES_LOCK_ESTABLISHED";
                    hMessages->Fill(3);
                }
                // SERDES lock lost (data might be nonsense, including up to 2 previous messages)
                else if((message_type & 0b11111110) == 0b00001100) {
                    LOG(DEBUG) << "SERDES_LOCK_LOST";
                    hMessages->Fill(4);
                }
                // Unexpected data came from the chip or there was a checksum error.
                else if((message_type & 0b11111110) == 0b00000100) {
                    LOG(DEBUG) << "WEIRD_DATA";
                    hMessages->Fill(5);
                }
                // Unknown message identifier
                else {
                    LOG(DEBUG) << "UNKNOWN_MESSAGE";
                    hMessages->Fill(1);
                }
            }
        }
    }
    return true;
}
