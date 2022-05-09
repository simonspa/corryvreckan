/**
 * @file
 * @brief Implementation of module EventLoaderTimestamp
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderTimestamp.h"
#include <dirent.h>

using namespace corryvreckan;

EventLoaderTimestamp::EventLoaderTimestamp(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("event_length", Units::get<double>(10, "us"));
    config_.setDefault<double>("time_offset", Units::get<double>(0, "ns"));
    config_.setDefault<size_t>("buffer_depth", 10);

    m_buffer_depth = config_.get<size_t>("buffer_depth");
    m_eventLength = config_.get<double>("event_length");
    m_time_offset = config_.get<double>("time_offset");

    // Take input directory from global parameters
    m_inputDirectory = config_.getPath("input_directory");
}

void EventLoaderTimestamp::initialize() {

    if(m_buffer_depth < 1) {
        throw InvalidValueError(config_, "buffer_depth", "Buffer depth must be larger than 0.");
    } else {
        LOG(INFO) << "Using buffer_depth = " << m_buffer_depth;
    }

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
            if(entry->d_name != m_detector->getName()) {
                continue;
            }
            LOG(DEBUG) << "Found directory for detector " << entry->d_name;

            // Open the folder for this device
            std::string dataDirName = m_inputDirectory + "/" + entry->d_name;
            DIR* dataDir = opendir(dataDirName.c_str());

            // Get all of the files for this chip
            while((file = readdir(dataDir))) {
                std::string filename = dataDirName + "/" + file->d_name;

                // Check if file has extension .dat
                if(std::string(file->d_name).find(".dat") != std::string::npos) {
                    LOG(INFO) << "Enqueuing data file for " << entry->d_name << ": " << filename;
                    detector_files.push_back(filename);
                }
            }
        }
    }

    // Check that we have files for this detector and sort them correctly:
    if(detector_files.empty()) {
        throw ModuleError("No data file found for detector " + m_detector->getName() + " in input directory " +
                          m_inputDirectory);
    }

    // Initialise null values for later
    m_syncTime = 0;
    m_clearedHeader = false;
    m_syncTimeTDC = 0;
    m_TDCoverflowCounter = 0;
    m_prevTriggerNumber = 0;
    m_triggerOverflowCounter = 0;
    eof_reached = false;

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
            LOG(DEBUG) << "Opened data file for " << m_detector->getName() << ": " << filename;

            // The header is repeated in every new data file, thus skip it for all.
            uint32_t headerID;
            if(!new_file->read(reinterpret_cast<char*>(&headerID), sizeof headerID)) {
                throw ModuleError("Cannot read header ID for " + m_detector->getName() + " in file " + filename);
            }
            if(headerID != 1380208723) {
                throw ModuleError("Incorrect header ID for " + m_detector->getName() + " in file " + filename + ": " +
                                  std::to_string(headerID));
            }
            LOG(TRACE) << "Header ID: \"" << headerID << "\"";

            // Skip the rest of the file header
            uint32_t headerSize;
            if(!new_file->read(reinterpret_cast<char*>(&headerSize), sizeof headerSize)) {
                throw ModuleError("Cannot read header size for " + m_detector->getName() + " in file " + filename);
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
}

bool EventLoaderTimestamp::decodeNextWord() {
    std::string detectorID = m_detector->getName();

    // Check if current file is at its end and move to the next:
    if((*m_file_iterator)->eof()) {
        m_file_iterator++;
        LOG(INFO) << "Starting to read next file for " << detectorID << ": " << (*m_file_iterator).get();
    }

    // Check if the last file is finished:
    if(m_file_iterator == m_files.end()) {
        LOG(INFO) << "EOF for all files of " << detectorID;
        eof_reached = true;
        return false;
    }

    // Now read the data packets.
    ULong64_t pixdata = 0;

    // If we can't read data anymore, jump to begin of loop:
    if(!(*m_file_iterator)->read(reinterpret_cast<char*>(&pixdata), sizeof pixdata)) {
        LOG(INFO) << "No more data in current file for " << detectorID << ": " << (*m_file_iterator).get();
        return true;
    }

    LOG(TRACE) << "0x" << std::hex << pixdata << std::dec << " - " << pixdata;

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
            return true;
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
        return true;
    }

    // Header 0x6 indicate trigger data
    if(header == 0x6) {
        const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;
        if(header2 == 0xF) {
            unsigned int stamp = (pixdata & 0x1E0) >> 5;
            long long int timestamp_raw = static_cast<long long int>(pixdata & 0xFFFFFFFFE00) >> 9;
            long long int timestamp = 0;
            int triggerNumber = ((pixdata & 0xFFF00000000000) >> 44);

            int intermediate = (pixdata & 0x1F);
            if(intermediate != 0) {
                return true;
            }

            if(triggerNumber < m_prevTriggerNumber) {
                m_triggerOverflowCounter++;
            }

            // if jump back in time is larger than 1 sec, overflow detected...
            if((m_syncTimeTDC - timestamp_raw) > 0x1312d000) {
                m_TDCoverflowCounter++;
            }

            timestamp = timestamp_raw + (static_cast<long long int>(m_TDCoverflowCounter) << 35);

            double triggerTime =
                (static_cast<double>(timestamp) + static_cast<double>(stamp) / 12) / (8. * 0.04); // 320 MHz clock

            m_syncTimeTDC = timestamp_raw;

            int triggerID = triggerNumber + (m_triggerOverflowCounter << 12);
            m_prevTriggerNumber = triggerNumber;

            auto triggerSignal = std::make_shared<SpidrSignal>("trigger", triggerTime + m_time_offset, triggerID);
            sorted_signals_.push(triggerSignal);
            LOG(DEBUG) << triggerID << ' ' << Units::display(triggerTime, {"s", "us", "ns"});
        }
    }

    return true;
}

void EventLoaderTimestamp::fillBuffer() {
    // read data from file and fill timesorted buffer
    while(sorted_signals_.size() < m_buffer_depth && !eof_reached) {
        // decodeNextWord returns false when EOF is reached and true otherwise
        if(!decodeNextWord()) {
            LOG(TRACE) << "decodeNextWord returns false: reached EOF.";
            break;
        }
    }
}

StatusCode EventLoaderTimestamp::run(const std::shared_ptr<Clipboard>& clipboard) {
    std::shared_ptr<Event> event;

    fillBuffer();

    if(!sorted_signals_.empty()) {
        auto signal = sorted_signals_.top();

        if(clipboard->isEventDefined()) {
            event = clipboard->getEvent();
        } else {
            event =
                std::make_shared<Event>(signal->timestamp() - m_eventLength / 2, signal->timestamp() + m_eventLength / 2);
            clipboard->putEvent(event);
        }

        for(;;) {
            fillBuffer();

            if(sorted_signals_.empty()) {
                return StatusCode::EndRun;
            }

            signal = sorted_signals_.top();

            auto position = event->getTimestampPosition(signal->timestamp());
            if(position == Event::Position::AFTER) {
                LOG(DEBUG) << "Stopping processing event, signal is after "
                              "event window ("
                           << Units::display(signal->timestamp(), {"s", "us", "ns"}) << " > "
                           << Units::display(event->end(), {"s", "us", "ns"}) << ")";

                return StatusCode::Success;
            } else if(position == Event::Position::BEFORE) {
                LOG(TRACE) << "Skipping signal, is before event window ("
                           << Units::display(signal->timestamp(), {"s", "us", "ns"}) << " < "
                           << Units::display(event->start(), {"s", "us", "ns"}) << ")";

                sorted_signals_.pop();
            } else {
                event->addTrigger(static_cast<uint32_t>(signal->trigger()), signal->timestamp());
                sorted_signals_.pop();
            }
        }
    }

    return StatusCode::EndRun;
}

void EventLoaderTimestamp::finalize(const std::shared_ptr<ReadonlyClipboard>&) {}
