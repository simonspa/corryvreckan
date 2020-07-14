/**
 * @file
 * @brief Implementation of module EventLoaderEUDAQ2
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderEUDAQ2.h"
#include "eudaq/FileReader.hh"

using namespace corryvreckan;

EventLoaderEUDAQ2::EventLoaderEUDAQ2(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<bool>("get_time_residuals", false);
    config_.setDefault<bool>("get_tag_vectors", false);
    config_.setDefault<bool>("ignore_bore", true);
    config_.setDefault<double>("skip_time", 0.);
    config_.setDefault<int>("buffer_depth", 0);
    config_.setDefault<int>("shift_triggers", 0);
    config_.setDefault<bool>("inclusive", true);

    m_filename = config_.getPath("file_name", true);
    m_get_time_residuals = config_.get<bool>("get_time_residuals");

    m_get_tag_vectors = config_.get<bool>("get_tag_vectors");
    m_ignore_bore = config_.get<bool>("ignore_bore");
    m_skip_time = config_.get<double>("skip_time");
    m_adjust_event_times = config_.getMatrix<std::string>("adjust_event_times", {});
    m_buffer_depth = config_.get<int>("buffer_depth");
    m_shift_triggers = config_.get<int>("shift_triggers");
    m_inclusive = config_.get<bool>("inclusive");

    // Prepare EUDAQ2 config object
    eudaq::Configuration cfg;

    // Forward all settings to EUDAQ
    // WARNING: the EUDAQ Configuration class is not very flexible and e.g. booleans have to be passed as 1 and 0.
    auto configs = config_.getAll();
    for(const auto& key : configs) {
        LOG(DEBUG) << "Forwarding key \"" << key.first << " = " << key.second << "\" to EUDAQ converter";
        cfg.Set(key.first, key.second);
    }

    // Provide the calibration file specified in the detector geometry:
    // NOTE: This should go last to allow overwriting the calibration_file key in the module config with absolute path
    auto calibration_file =
        config_.has("calibration_file") ? config_.getPath("calibration_file", true) : m_detector->calibrationFile();
    if(!calibration_file.empty()) {
        LOG(DEBUG) << "Forwarding detector calibration file: " << calibration_file;
        cfg.Set("calibration_file", calibration_file);
    }

    // Converting the newly built configuration to a shared pointer of a const configuration object
    // Unfortunbately EUDAQ does not provide appropriate member functions for their configuration class to avoid this dance
    const eudaq::Configuration eu_cfg = cfg;
    eudaq_config_ = std::make_shared<const eudaq::Configuration>(eu_cfg);
}

void EventLoaderEUDAQ2::initialize() {

    // Declare histograms
    std::string title = ";EUDAQ event start time[ms];# entries";
    hEudaqEventStart = new TH1D("eudaqEventStart", title.c_str(), 3e6, 0, 3e3);

    title = ";EUDAQ event start time[s];# entries";
    hEudaqEventStart_long = new TH1D("eudaqEventStart_long", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (on clipboard); Corryvreckan event start time [ms];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (on clipboard); Corryvreckan event start time [s];# entries";
    hClipboardEventStart_long = new TH1D("clipboardEventStart_long", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event end times (on clipboard); Corryvreckan event end time [ms];# entries";
    hClipboardEventEnd = new TH1D("clipboardEventEnd", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event end times (on clipboard); Corryvreckan event duration [ms];# entries";
    hClipboardEventDuration = new TH1D("clipboardEventDuration", title.c_str(), 3e6, 0, 3e3);

    hTriggersPerEvent = new TH1D("hTriggersPerEvent", "hTriggersPerEvent;triggers per event;entries", 20, -0.5, 19.5);

    hEudaqeventsPerCorry = new TH1D(
        "hEudaqeventsPerCorryEvent",
        ("hEudaqeventsPerCorryEvent; number of events from " + m_detector->getName() + " per corry event; entries").c_str(),
        50,
        -.5,
        49.5);
    // Create the following histograms only when detector is not auxiliary:
    if(!m_detector->isAuxiliary()) {
        title = "hitmap;column;row;# events";
        hitmap = new TH2F("hitmap",
                          title.c_str(),
                          m_detector->nPixels().X(),
                          -0.5,
                          m_detector->nPixels().X() - 0.5,
                          m_detector->nPixels().Y(),
                          -0.5,
                          m_detector->nPixels().Y() - 0.5);

        title = "rawValues; column; row; raw values";
        hRawValuesMap = new TProfile2D("hRawValuesMap",
                                       title.c_str(),
                                       m_detector->nPixels().X(),
                                       -0.5,
                                       m_detector->nPixels().X() - 0.5,
                                       m_detector->nPixels().Y(),
                                       -0.5,
                                       m_detector->nPixels().Y() - 0.5);

        title = ";hit time [ms];# events";
        hPixelTimes = new TH1F("hPixelTimes", title.c_str(), 3e6, 0, 3e3);

        title = ";hit timestamp [s];# events";
        hPixelTimes_long = new TH1F("hPixelTimes_long", title.c_str(), 3e6, 0, 3e3);

        title = ";pixel raw values;# events";
        hPixelRawValues = new TH1F("hPixelRawValues", title.c_str(), 1024, -0.5, 1023.5);

        title = "Pixel Multiplicity per EUDAQ Event;# pixels;# events";
        hPixelMultiplicityPerEudaqEvent = new TH1F("hPixelMultiplicityPerEudaqEvent", title.c_str(), 1000, -0.5, 999.5);

        title = "Pixel Multiplicity per Corry Event;# pixels;# events";
        hPixelMultiplicityPerCorryEvent = new TH1F("hPixelMultiplicityPerCorryEvent", title.c_str(), 1000, -0.5, 999.5);

        if(m_get_time_residuals) {
            hPixelTimeEventBeginResidual =
                new TH1F("hPixelTimeEventBeginResidual",
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
        }
    }

    // open the input file with the eudaq reader
    try {
        reader_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << m_filename
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(config_, "file_path", "Parsing error!");
    }

    // Check if all elements of m_adjust_event_times have a valid size of 3, if not throw error.
    for(auto& shift_times : m_adjust_event_times) {
        if(shift_times.size() != 3) {
            throw InvalidValueError(
                config_,
                "adjust_event_times",
                "Parameter needs 3 values per row: [\"event type\", shift event start, shift event end]");
        }
    }
}

std::shared_ptr<eudaq::StandardEvent> EventLoaderEUDAQ2::get_next_sorted_std_event() {

    // Refill the event buffer if necessary:
    while(static_cast<int>(sorted_events_.size()) < m_buffer_depth) {
        LOG(DEBUG) << "Filling buffer with new event.";
        // fill buffer with new std event:
        auto new_event = get_next_std_event();
        sorted_events_.push(new_event);
    }

    // get first element of queue and erase it
    auto stdevt = sorted_events_.top();
    sorted_events_.pop();
    return stdevt;
}

std::shared_ptr<eudaq::StandardEvent> EventLoaderEUDAQ2::get_next_std_event() {
    auto stdevt = eudaq::StandardEvent::MakeShared();
    bool decoding_failed = true;
    do {
        // Create new StandardEvent
        stdevt = eudaq::StandardEvent::MakeShared();

        // Check if we need a new full event or if we still have some in the cache:
        if(events_.empty()) {
            LOG(TRACE) << "Reading new EUDAQ event from file";
            auto new_event = reader_->GetNextEvent();
            if(!new_event) {
                LOG(DEBUG) << "Reached EOF";
                throw EndOfFile();
            }
            // Build buffer from all sub-events:
            events_ = new_event->GetSubEvents();
            // The main event might also contain data, so add it to the buffer:
            if(events_.empty()) {
                events_.push_back(new_event);
            }
        }
        LOG(TRACE) << "Buffer contains " << events_.size() << " (sub-) events:";
        for(auto& evt : events_) {
            LOG(TRACE) << "  (sub-) event of type " << evt->GetDescription();
        }

        // Retrieve first and remove from buffer:
        auto event = events_.front();
        events_.erase(events_.begin());

        // If this is a Begin-of-Run event and we should ignore it, please do so:
        if(event->IsBORE() && m_ignore_bore) {
            LOG(DEBUG) << "Found EUDAQ2 BORE event, ignoring it";
            continue;
        }

        // Read and store tag information:
        if(m_get_tag_vectors) {
            retrieve_event_tags(event);
        }

        decoding_failed = !eudaq::StdEventConverter::Convert(event, stdevt, eudaq_config_);
        LOG(DEBUG) << event->GetDescription() << ": EventConverter returned " << (decoding_failed ? "false" : "true");
    } while(decoding_failed);
    return stdevt;
}

void EventLoaderEUDAQ2::retrieve_event_tags(const eudaq::EventSPC evt) {
    auto tags = evt->GetTags();

    for(auto tag_pair : tags) {
        // Trying to convert tag value to double:
        try {
            double value = std::stod(tag_pair.second);

            // Check if histogram exists already, if not: create it
            if(hTagValues.find(tag_pair.first) == hTagValues.end()) {
                std::string histName = "hTagValues_" + tag_pair.first;
                std::string histTitle = "tag_" + tag_pair.first + ";event / 1000;tag value";
                hTagValues[tag_pair.first] = new TProfile(histName.c_str(), histTitle.c_str(), 2e5, 0, 100);
            }
            hTagValues[tag_pair.first]->Fill(evt->GetEventN() / 1000, value, 1);
        } catch(std::exception& e) {
        }
    }
}
Event::Position EventLoaderEUDAQ2::is_within_event(const std::shared_ptr<Clipboard>& clipboard,
                                                   std::shared_ptr<eudaq::StandardEvent> evt) const {

    // Check if this event has timestamps available:
    if(evt->GetTimeBegin() == 0 && evt->GetTimeEnd() == 0) {
        LOG(DEBUG) << evt->GetDescription() << ": Event has no timestamp, comparing trigger IDs";

        // If there is no event defined yet, there is little we can do:
        if(!clipboard->isEventDefined()) {
            LOG(DEBUG) << "No Corryvreckan event defined - cannot define without timestamps.";
            return Event::Position::UNKNOWN;
        }

        // Get position of this time frame with respect to the defined event:
        auto trigger_position = clipboard->getEvent()->getTriggerPosition(evt->GetTriggerN());
        if(trigger_position == Event::Position::BEFORE) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " before triggers registered in Corryvreckan event";
        } else if(trigger_position == Event::Position::AFTER) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " after triggers registered in Corryvreckan event";
        } else if(trigger_position == Event::Position::UNKNOWN) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " within Corryvreckan event range but not registered";
        } else {
            evt->SetTimeBegin(static_cast<uint64_t>(clipboard->getEvent()->getTriggerTime(evt->GetTriggerN()) * 1000));
            evt->SetTimeEnd(static_cast<uint64_t>(clipboard->getEvent()->getTriggerTime(evt->GetTriggerN()) * 1000));
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " found in Corryvreckan event";
        }
        return trigger_position;
    }

    // Read time from EUDAQ2 event and convert from picoseconds to nanoseconds:
    double event_start = static_cast<double>(evt->GetTimeBegin()) / 1000 + m_detector->timeOffset();
    double event_end = static_cast<double>(evt->GetTimeEnd()) / 1000 + m_detector->timeOffset();
    // Store the original position of the event before adjusting its length:
    double event_timestamp = event_start;
    LOG(DEBUG) << "event_start = " << Units::display(event_start, "us")
               << ", event_end = " << Units::display(event_end, "us");

    // If adjustment of event start/end is required:
    const auto it = std::find_if(m_adjust_event_times.begin(),
                                 m_adjust_event_times.end(),
                                 [evt](const std::vector<std::string>& x) { return x.front() == evt->GetDescription(); });

    if(it != m_adjust_event_times.end()) {
        event_start += corryvreckan::from_string<double>(it->at(1));
        event_end += corryvreckan::from_string<double>(it->at(2));
        LOG(DEBUG) << "Adjusting " << it->at(0) << " event_start by "
                   << Units::display(corryvreckan::from_string<double>(it->at(1)), {"us", "ns"}) << ", event_end by "
                   << Units::display(corryvreckan::from_string<double>(it->at(2)), {"us", "ns"});
        LOG(DEBUG) << "Adjusted event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
    }

    // Skip if later start is requested:
    if(event_start < m_skip_time) {
        LOG(DEBUG) << "Event start before requested skip time: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(m_skip_time, {"us", "ns"});
        return Event::Position::BEFORE;
    }

    // Check if an event is defined or if we need to create it:
    if(!clipboard->isEventDefined()) {
        LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
        clipboard->putEvent(std::make_shared<Event>(event_start, event_end));
    } else {
        LOG(DEBUG) << "Corryvreckan event found on clipboard.";
    }

    // Get position of this time frame with respect to the defined event:
    auto position = clipboard->getEvent()->getFramePosition(event_start, event_end, m_inclusive);
    if(position == Event::Position::BEFORE) {
        LOG(DEBUG) << "Event start before Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(clipboard->getEvent()->start(), {"us", "ns"});
    } else if(position == Event::Position::AFTER) {
        LOG(DEBUG) << "Event end after Corryvreckan event: " << Units::display(event_end, {"us", "ns"}) << " > "
                   << Units::display(clipboard->getEvent()->end(), {"us", "ns"});
    } else {
        // check if event has valid trigger ID (flag = 0x10):
        if(evt->IsFlagTrigger()) {
            // Potentially shift the trigger IDs if requested
            auto trigger_id = static_cast<uint32_t>(static_cast<int>(evt->GetTriggerN()) + m_shift_triggers);
            // Store potential trigger numbers, assign to center of event:
            clipboard->getEvent()->addTrigger(trigger_id, event_timestamp);
            LOG(DEBUG) << "Stored trigger ID " << evt->GetTriggerN() << " at "
                       << Units::display(event_timestamp, {"us", "ns"});
        }
    }

    return position;
}

PixelVector EventLoaderEUDAQ2::get_pixel_data(std::shared_ptr<eudaq::StandardEvent> evt, int plane_id) const {

    PixelVector pixels;

    // No plane found:
    if(plane_id < 0) {
        return pixels;
    }

    auto plane = evt->GetPlane(static_cast<size_t>(plane_id));
    // Concatenate plane name according to naming convention: sensor_type + "_" + int
    auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
    auto detector_name = m_detector->getName();
    // Convert to lower case before string comparison to avoid errors by the user:
    std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
    std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);
    LOG(TRACE) << plane_name << " (ID " << plane_id << ") with " << plane.HitPixels() << " pixel hits";

    // Loop over all hits and add to pixels vector:
    for(unsigned int i = 0; i < plane.HitPixels(); i++) {

        auto col = static_cast<int>(plane.GetX(i));
        auto row = static_cast<int>(plane.GetY(i));
        auto raw = static_cast<int>(plane.GetPixel(i)); // generic pixel raw value (could be ToT, ADC, ...)

        double ts;
        if(plane.GetTimestamp(i) == 0) {
            // If the plane timestamp is not defined, we place all pixels in the center of the EUDAQ2 event:
            ts = static_cast<double>(evt->GetTimeBegin() + evt->GetTimeEnd()) / 2 / 1000 + m_detector->timeOffset();
        } else {
            ts = static_cast<double>(plane.GetTimestamp(i)) / 1000 + m_detector->timeOffset();
        }

        if(col >= m_detector->nPixels().X() || row >= m_detector->nPixels().Y()) {
            LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix with size "
                         << m_detector->nPixels();
        }

        if(m_detector->masked(col, row)) {
            LOG(TRACE) << "Masking pixel (col, row) = (" << col << ", " << row << ")";
            continue;
        } else {
            LOG(TRACE) << "Storing (col, row, timestamp) = (" << col << ", " << row << ", "
                       << Units::display(ts, {"ns", "us", "ms"}) << ") from EUDAQ2 event data";
        }

        // when calibration is not available, set charge = raw
        auto pixel = std::make_shared<Pixel>(m_detector->getName(), col, row, raw, raw, ts);

        hitmap->Fill(col, row);
        hPixelTimes->Fill(static_cast<double>(Units::convert(ts, "ms")));
        hPixelTimes_long->Fill(static_cast<double>(Units::convert(ts, "s")));
        hPixelRawValues->Fill(raw);
        hRawValuesMap->Fill(col, row, raw);

        pixels.push_back(pixel);
    }
    hPixelMultiplicityPerEudaqEvent->Fill(static_cast<int>(pixels.size()));
    LOG(DEBUG) << m_detector->getName() << ": Plane contains " << pixels.size() << " pixels";

    return pixels;
}

bool EventLoaderEUDAQ2::filter_detectors(std::shared_ptr<eudaq::StandardEvent> evt, int& plane_id) const {
    // Check if the detector type matches the currently processed detector type:
    auto detector_type = evt->GetDetectorType();
    std::transform(detector_type.begin(), detector_type.end(), detector_type.begin(), ::tolower);
    // Fall back to parsing the description if not set:
    if(detector_type.empty()) {
        LOG(TRACE) << "Using fallback comparison with EUDAQ2 event description";
        auto description = evt->GetDescription();
        std::transform(description.begin(), description.end(), description.begin(), ::tolower);
        if(description.find(m_detector->getType()) == std::string::npos) {
            LOG(DEBUG) << "Ignoring event because description doesn't match type " << m_detector->getType() << ": "
                       << description;
            return false;
        }
    } else if(detector_type != m_detector->getType()) {
        LOG(DEBUG) << "Ignoring event because detector type doesn't match: " << detector_type;
        return false;
    }

    // To the best of our knowledge, this is the detector we are looking for.
    LOG(DEBUG) << "Found matching event for detector type " << m_detector->getType();
    if(evt->NumPlanes() == 0) {
        return true;
    }

    // Check if we can identify the detector itself among the planes:
    for(size_t i_plane = 0; i_plane < evt->NumPlanes(); i_plane++) {
        auto plane = evt->GetPlane(i_plane);

        // Concatenate plane name according to naming convention: sensor_type + "_" + int
        auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
        auto detector_name = m_detector->getName();
        // Convert to lower case before string comparison to avoid errors by the user:
        std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
        std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);

        if(detector_name == plane_name) {
            plane_id = static_cast<int>(i_plane);
            LOG(DEBUG) << "Found matching plane in event for detector " << m_detector->getName();
            return true;
        } else {
            LOG(DEBUG) << "plane " << plane_name << "does not match " << detector_name;
        }
    }

    // Detector not found among planes of this event
    LOG(DEBUG) << "Ignoring event because no matching plane could be found for detector " << m_detector->getName();
    return false;
}

StatusCode EventLoaderEUDAQ2::run(const std::shared_ptr<Clipboard>& clipboard) {
    size_t m_events = 0;

    PixelVector pixels;

    Event::Position current_position = Event::Position::UNKNOWN;
    while(1) {
        // Retrieve next event from file/buffer:
        if(!event_) {
            try {
                if(m_buffer_depth == 0) {
                    // simply get next decoded EUDAQ StandardEvent from buffer
                    event_ = get_next_std_event();
                } else {
                    // get next decoded EUDAQ StandardEvent from timesorted buffer
                    event_ = get_next_sorted_std_event();
                }
            } catch(EndOfFile&) {
                return StatusCode::EndRun;
            }
        }

        // Filter out "wrong" detectors and store plane ID if found:
        int plane_id = -1;
        if(!filter_detectors(event_, plane_id)) {
            event_.reset();
            continue;
        }

        // Check if this event is within the currently defined Corryvreckan event:
        current_position = is_within_event(clipboard, event_);

        if(current_position == Event::Position::DURING) {
            m_events++;
            LOG(DEBUG) << "Is within current Corryvreckan event, storing data";
            // Store data on the clipboard
            auto new_pixels = get_pixel_data(event_, plane_id);
            m_hits += new_pixels.size();
            pixels.insert(pixels.end(), new_pixels.begin(), new_pixels.end());
        }

        // If this event was after the current event or if we have not enough information, stop reading:
        if(current_position == Event::Position::AFTER || current_position == Event::Position::UNKNOWN) {
            break;
        }

        // Do not fill if current_position == Event::Position::AFTER to avoid double-counting!
        // Converting EUDAQ2 picoseconds into Corryvreckan nanoseconds:
        hEudaqEventStart->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e9);       // here convert from ps to ms
        hEudaqEventStart_long->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e12); // here convert from ps to seconds
        if(clipboard->isEventDefined()) {
            hClipboardEventStart->Fill(static_cast<double>(Units::convert(clipboard->getEvent()->start(), "ms")));
            hClipboardEventStart_long->Fill(static_cast<double>(Units::convert(clipboard->getEvent()->start(), "s")));
            hClipboardEventEnd->Fill(static_cast<double>(Units::convert(clipboard->getEvent()->end(), "ms")));
            hClipboardEventDuration->Fill(
                static_cast<double>(Units::convert(clipboard->getEvent()->end() - clipboard->getEvent()->start(), "ms")));
        }

        // Reset this shared event pointer to get a new event from the stack:
        event_.reset();
    }

    auto event = clipboard->getEvent();
    hTriggersPerEvent->Fill(static_cast<double>(event->triggerList().size()));
    LOG(DEBUG) << "Triggers on clipboard event: " << event->triggerList().size();
    for(auto& trigger : event->triggerList()) {
        LOG(DEBUG) << "\t ID: " << trigger.first << ", time: " << Units::display(trigger.second, "us");
    }

    // histogram only exists for non-auxiliary detectors:
    if(!m_detector->isAuxiliary()) {
        hPixelMultiplicityPerCorryEvent->Fill(static_cast<int>(pixels.size()));
    }

    // Loop over pixels for plotting
    if(m_get_time_residuals) {
        for(auto& pixel : pixels) {
            hPixelTimeEventBeginResidual->Fill(
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));
            hPixelTimeEventBeginResidual_wide->Fill(
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));
            hPixelTimeEventBeginResidualOverTime->Fill(
                static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));

            size_t iTrigger = 0;
            for(auto& trigger : event->triggerList()) {
                // check if histogram exists already, if not: create it
                if(hPixelTriggerTimeResidual.find(iTrigger) == hPixelTriggerTimeResidual.end()) {
                    std::string histName = "hPixelTriggerTimeResidual_" + to_string(iTrigger);
                    std::string histTitle = histName + ";pixel_ts - trigger_ts [us];# entries";
                    hPixelTriggerTimeResidual[iTrigger] = new TH1D(histName.c_str(), histTitle.c_str(), 2e5, -100, 100);
                }
                // use iTrigger, not trigger ID (=trigger.first) (which is unique and continuously incrementing over the
                // runtime)
                hPixelTriggerTimeResidual[iTrigger]->Fill(
                    static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
                if(iTrigger == 0) { // fill only for 0th trigger
                    hPixelTriggerTimeResidualOverTime->Fill(
                        static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                        static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
                }
                iTrigger++;
            }
        }
    }

    hEudaqeventsPerCorry->Fill(static_cast<double>(m_events));
    // Store the full event data on the clipboard:
    clipboard->putData(pixels, m_detector->getName());

    LOG(DEBUG) << "Finished Corryvreckan event";
    return StatusCode::Success;
}

void EventLoaderEUDAQ2::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(INFO) << "Found " << m_hits << " hits in the data.";
}
