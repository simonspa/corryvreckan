/**
 * @file
 * @brief Implementation of EventLoaderEUDAQ2 module
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderEUDAQ2.h"
#include "eudaq/FileReader.hh"

using namespace corryvreckan;

EventLoaderEUDAQ2::EventLoaderEUDAQ2(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_filename = m_config.getPath("file_name", true);
    m_get_time_residuals = m_config.get<bool>("get_time_residuals", false);
    m_skip_time = m_config.get("skip_time", 0.);
    m_adjust_event_times = m_config.getMatrix<std::string>("adjust_event_times", {});
    m_buffer_depth = m_config.get<int>("buffer_depth", 0);

    // Forward all settings to EUDAQ
    // WARNING: the EUDAQ Configuration class is not very flexible and e.g. booleans have to be passed as 1 and 0.
    eudaq::Configuration cfg;
    auto configs = m_config.getAll();
    for(const auto& key : configs) {
        LOG(DEBUG) << "Forwarding key \"" << key.first << " = " << key.second << "\" to EUDAQ converter";
        cfg.Set(key.first, key.second);
    }

    // Converting the newly built configuration to a shared pointer of a cont configuration object
    // Unfortunbately EUDAQ does not provide appropriate member functions for their configuration class to avoid this dance
    const eudaq::Configuration eu_cfg = cfg;
    eudaq_config_ = std::make_shared<const eudaq::Configuration>(eu_cfg);
}

void EventLoaderEUDAQ2::initialise() {

    // Declare histograms
    std::string title = "hitmap;column;row;# events";
    hitmap = new TH2F("hitmap",
                      title.c_str(),
                      m_detector->nPixels().X(),
                      0,
                      m_detector->nPixels().X(),
                      m_detector->nPixels().Y(),
                      0,
                      m_detector->nPixels().Y());

    title = ";hit time [ms];# events";
    hPixelTimes = new TH1F("hPixelTimes", title.c_str(), 3e6, 0, 3e3);

    title = ";hit timestamp [s];# events";
    hPixelTimes_long = new TH1F("hPixelTimes_long", title.c_str(), 3e6, 0, 3e3);

    title = ";pixel raw values;# events";
    hPixelRawValues = new TH1F("hPixelRawValues;", title.c_str(), 1024, 0, 1024);

    title = "Pixel multiplicity per Corry frame;# pixels per event;# entries";
    hPixelsPerEvent = new TH1F("pixelsPerFrame", title.c_str(), 1000, 0, 1000);

    title = ";EUDAQ event start time[ms];# entries";
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

    hTriggersPerEvent = new TH1D("hTriggersPerEvent", "hTriggersPerEvent;triggers per event;entries", 20, 0, 20);

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
        std::string histTitle = "hPixelTriggerTimeResidualOverTime_0;time [us];pixel_ts - trigger_ts [us];# entries";
        hPixelTriggerTimeResidualOverTime =
            new TH2D("hPixelTriggerTimeResidualOverTime_0", histTitle.c_str(), 3e3, 0, 3e3, 1e4, -50, 50);
    }

    // open the input file with the eudaq reader
    try {
        reader_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << m_filename
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(m_config, "file_path", "Parsing error!");
    }

    // Check if all elements of m_adjust_event_times have a valid size of 3, if not throw error.
    for(auto& shift_times : m_adjust_event_times) {
        if(shift_times.size() != 3) {
            throw InvalidValueError(
                m_config,
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

        decoding_failed = !eudaq::StdEventConverter::Convert(event, stdevt, eudaq_config_);
        LOG(DEBUG) << event->GetDescription() << ": EventConverter returned " << (decoding_failed ? "false" : "true");
    } while(decoding_failed);
    return stdevt;
}

Event::Position EventLoaderEUDAQ2::is_within_event(std::shared_ptr<Clipboard> clipboard,
                                                   std::shared_ptr<eudaq::StandardEvent> evt) {

    // Check if this event has timestamps available:
    if(evt->GetTimeBegin() == 0) {
        LOG(DEBUG) << evt->GetDescription() << ": Event has no timestamp, comparing trigger number";

        // If there is no event defined yet or the trigger number is unkown, there is little we can do:
        if(!clipboard->event_defined() || !clipboard->get_event()->hasTriggerID(evt->GetTriggerN())) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " not found in current event.";
            return Event::Position::UNKNOWN;
        }

        // Store trigger timestamp in event:
        auto trigger_time = clipboard->get_event()->getTriggerTime(evt->GetTriggerN());
        LOG(DEBUG) << "Assigning trigger time " << Units::display(trigger_time, {"us", "ns"}) << " to event with trigger ID "
                   << evt->GetTriggerN();
        // Set EUDAQ StandardEvent timestamp in picoseconds:
        evt->SetTimeBegin(static_cast<uint64_t>(trigger_time * 1000));
        evt->SetTimeEnd(static_cast<uint64_t>(trigger_time * 1000));
    }

    // Read time from EUDAQ2 event and convert from picoseconds to nanoseconds:
    double event_start = static_cast<double>(evt->GetTimeBegin()) / 1000 + m_detector->timingOffset();
    double event_end = static_cast<double>(evt->GetTimeEnd()) / 1000 + m_detector->timingOffset();
    LOG(DEBUG) << "event_start = " << Units::display(event_start, "us")
               << ", event_end = " << Units::display(event_end, "us");

    // If adjustment of event start/end is required:
    const auto it = std::find_if(m_adjust_event_times.begin(),
                                 m_adjust_event_times.end(),
                                 [evt](const std::vector<std::string>& x) { return x.front() == evt->GetDescription(); });

    // Skip if later start is requested:
    if(event_start < m_skip_time) {
        LOG(DEBUG) << "Event start before requested skip time: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(m_skip_time, {"us", "ns"});
        return Event::Position::BEFORE;
    }

    double shift_start = 0;
    double shift_end = 0;

    if(!clipboard->event_defined()) {
        LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
        if(it != m_adjust_event_times.end()) {
            shift_start = corryvreckan::from_string<double>(it->at(1));
            shift_end = corryvreckan::from_string<double>(it->at(2));
            event_start += shift_start;
            event_end += shift_end;
            LOG(DEBUG) << "Adjusting " << it->at(0) << ": event_start by " << Units::display(shift_start, {"us", "ns"})
                       << ", event_end by " << Units::display(shift_end, {"us", "ns"});
        }
        LOG(DEBUG) << "Shifted Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
        clipboard->put_event(std::make_shared<Event>(event_start, event_end));
    } else {
        LOG(DEBUG) << "Corryvreckan event found on clipboard.";
    }

    // Get position of this time frame with respect to the defined event:
    auto position = clipboard->get_event()->getFramePosition(event_start, event_end);
    if(position == Event::Position::BEFORE) {
        LOG(DEBUG) << "Event start before Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(clipboard->get_event()->start(), {"us", "ns"});
    } else if(position == Event::Position::AFTER) {
        LOG(DEBUG) << "Event end after Corryvreckan event: " << Units::display(event_end, {"us", "ns"}) << " > "
                   << Units::display(clipboard->get_event()->end(), {"us", "ns"});
    } else {
        // check if event has valid trigger ID (flag = 0x10):
        if(evt->IsFlagTrigger()) {
            // Store potential trigger numbers, assign to center of event:
            clipboard->get_event()->addTrigger(evt->GetTriggerN(), event_start - shift_start);
            LOG(DEBUG) << "Stored trigger ID " << evt->GetTriggerN() << " at "
                       << Units::display(event_start - shift_start, {"us", "ns"});
        }
    }

    return position;
}

Pixels* EventLoaderEUDAQ2::get_pixel_data(std::shared_ptr<eudaq::StandardEvent> evt) {

    Pixels* pixels = new Pixels();

    // Loop over all planes, select the relevant detector:
    for(size_t i_plane = 0; i_plane < evt->NumPlanes(); i_plane++) {
        auto plane = evt->GetPlane(i_plane);

        // Concatenate plane name according to naming convention: sensor_type + "_" + int
        auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
        auto detector_name = m_detector->name();
        // Convert to lower case before string comparison to avoid errors by the user:
        std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
        std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);
        LOG(TRACE) << plane_name << " (" << i_plane << " out of " << evt->NumPlanes() << ") with  " << plane.HitPixels()
                   << " hit pixels";

        if(detector_name != plane_name) {
            LOG(TRACE) << "Wrong plane: " << detector_name << "!=" << plane_name << ". Continue.";
            continue;
        }

        LOG(DEBUG) << "Found correct plane.";
        // Loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < plane.HitPixels(); i++) {
            auto col = static_cast<int>(plane.GetX(i));
            auto row = static_cast<int>(plane.GetY(i));
            auto raw = static_cast<int>(plane.GetPixel(i)); // generic pixel raw value (could be ToT, ADC, ...)
            auto ts = static_cast<double>(plane.GetTimestamp(i)) / 1000 + m_detector->timingOffset();

            if(col >= m_detector->nPixels().X() || row >= m_detector->nPixels().Y()) {
                LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix.";
            }

            LOG(DEBUG) << "Read pixel (col, row) = (" << col << ", " << row << ") from EUDAQ2 event data (before masking).";
            if(m_detector->masked(col, row)) {
                LOG(TRACE) << "Masked pixel (col, row) = (" << col << ", " << row << ")";
                continue;
            }

            // when calibration is not available, set charge = raw
            Pixel* pixel = new Pixel(m_detector->name(), col, row, raw, raw, ts);

            hitmap->Fill(col, row);
            hPixelTimes->Fill(static_cast<double>(Units::convert(ts, "ms")));
            hPixelTimes_long->Fill(static_cast<double>(Units::convert(ts, "s")));
            hPixelRawValues->Fill(raw);

            pixels->push_back(pixel);
        }
        hPixelsPerEvent->Fill(static_cast<int>(pixels->size()));
        LOG(DEBUG) << m_detector->name() << ": Plane contains " << pixels->size() << " pixels";
    }

    return pixels;
}

StatusCode EventLoaderEUDAQ2::run(std::shared_ptr<Clipboard> clipboard) {

    Pixels* pixels = new Pixels();

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

        // Check if this event is within the currently defined Corryvreckan event:
        current_position = is_within_event(clipboard, event_);

        if(current_position == Event::Position::DURING) {
            LOG(DEBUG) << "Is within current Corryvreckan event, storing data";
            // Store data on the clipboard
            auto new_pixels = get_pixel_data(event_);
            pixels->insert(pixels->end(), new_pixels->begin(), new_pixels->end());
            delete new_pixels;
        }

        // If this event was after the current event, stop reading:
        if(current_position == Event::Position::AFTER) {
            break;
        }

        // Do not fill if current_position == Event::Position::AFTER to avoid double-counting!
        // Converting EUDAQ2 picoseconds into Corryvreckan nanoseconds:
        hEudaqEventStart->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e9);       // here convert from ps to ms
        hEudaqEventStart_long->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e12); // here convert from ps to seconds
        if(clipboard->event_defined()) {
            hClipboardEventStart->Fill(static_cast<double>(Units::convert(clipboard->get_event()->start(), "ms")));
            hClipboardEventStart_long->Fill(static_cast<double>(Units::convert(clipboard->get_event()->start(), "s")));
            hClipboardEventEnd->Fill(static_cast<double>(Units::convert(clipboard->get_event()->end(), "ms")));
            hClipboardEventDuration->Fill(
                static_cast<double>(Units::convert(clipboard->get_event()->end() - clipboard->get_event()->start(), "ms")));
        }

        // Reset this shared event pointer to get a new event from the stack:
        event_.reset();
    }

    auto event = clipboard->get_event();
    hTriggersPerEvent->Fill(static_cast<double>(event->triggerList().size()));
    LOG(DEBUG) << "Triggers on clipboard event: " << event->triggerList().size();
    for(auto& trigger : event->triggerList()) {
        LOG(DEBUG) << "\t ID: " << trigger.first << ", time: " << Units::display(trigger.second, "us");
    }

    // Loop over pixels for plotting
    if(m_get_time_residuals) {
        for(auto& pixel : (*pixels)) {
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

    // Store the full event data on the clipboard:
    clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));

    LOG(DEBUG) << "Finished Corryvreckan event";
    return StatusCode::Success;
}
