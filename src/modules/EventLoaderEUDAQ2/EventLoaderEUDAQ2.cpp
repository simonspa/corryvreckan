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
    m_skip_time = m_config.get("skip_time", 0.);
    adjust_event_times = m_config.getMatrix<std::string>("adjust_event_times", {});
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

    title = ";hit timestamp [ns]; # events";
    hHitTimes = new TH1F("hitTimes", title.c_str(), 3e6, 0, 3e9);

    title = "pixel raw values";
    hPixelRawValues = new TH1F("hPixelRawValues", title.c_str(), 1024, 0, 1024);

    title = "Pixel multiplicity per Corry frame;# pixels per event;# entries";
    hPixelsPerEvent = new TH1F("pixelsPerFrame", title.c_str(), 1000, 0, 1000);

    title = ";EUDAQ event start time[ns];# entries";
    hEudaqEventStart = new TH1D("eudaqEventStart", title.c_str(), 3e6, 0, 3e9);

    title = "Corryvreckan event start times (on clipboard); Corryvreckan event start time [ns];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 3e6, 0, 3e9);

    hTluChipTimeResidual =
        new TH1F("hTluChipTimeResidual", "hTluChipTimeResidual; ts(tlu) - ts(Chip) [us]; # entries", 2e5, -100, 100);
    hTluChipTimeResidualvsTime = new TH2F("hTluChipTimeResidualvsTime",
                                          "hTluChipTimeResidualvsTime; event_time [s]; ts(tlu) - ts(Chip) [us]",
                                          3e3,
                                          0,
                                          3e3,
                                          3e4,
                                          -10,
                                          10);

    // open the input file with the eudaq reader
    try {
        reader_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << m_filename
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(m_config, "file_path", "Parsing error!");
    }

    // Check if all elements of adjust_event_times have a valid size of 3, if not throw error.
    for(auto& shift_times : adjust_event_times) {
        if(shift_times.size() != 3) {
            throw InvalidValueError(
                m_config,
                "adjust_event_times",
                "Parameter needs 3 values per row: [\"event type\", shift event start, shift event end]");
        }
    }
}

std::shared_ptr<eudaq::StandardEvent> EventLoaderEUDAQ2::get_next_event() {
    auto stdevt = eudaq::StandardEvent::MakeShared();
    bool decoding_failed = true;

    do {
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

            // FIXME get TLU events with trigger IDs before Ni - sort by name, reversed
            sort(events_.begin(), events_.end(), [](const eudaq::EventSPC& a, const eudaq::EventSPC& b) -> bool {
                return a->GetDescription() > b->GetDescription();
            });
        }
        LOG(TRACE) << "Buffer contains " << events_.size() << " (sub-) events:";
        for(int i = 0; i < static_cast<int>(events_.size()); i++) {
            LOG(TRACE) << "  (sub-) event " << i << " is a " << events_[static_cast<long unsigned int>(i)]->GetDescription();
        }
        auto event = events_.front();
        events_.erase(events_.begin());
        decoding_failed = !eudaq::StdEventConverter::Convert(event, stdevt, nullptr);
        LOG(DEBUG) << event->GetDescription() << ": Decoding " << (decoding_failed ? "failed" : "succeeded");
    } while(decoding_failed);
    return stdevt;
}

EventLoaderEUDAQ2::EventPosition EventLoaderEUDAQ2::is_within_event(std::shared_ptr<Clipboard> clipboard,
                                                                    std::shared_ptr<eudaq::StandardEvent> evt) {

    // Check if this event has timestamps available:
    if(evt->GetTimeBegin() == 0) {
        LOG(DEBUG) << evt->GetDescription() << ": Event has no timestamp, comparing trigger number";

        // If there is no event defined yet or the trigger number is unkown, there is little we can do:
        if(!clipboard->event_defined() || !clipboard->get_event()->hasTriggerID(evt->GetTriggerN())) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " not found in current event.";
            return EventPosition::UNKNOWN;
        }

        // Store trigger timestamp in event:
        auto trigger_time = clipboard->get_event()->getTriggerTime(evt->GetTriggerN());
        LOG(DEBUG) << "Assigning trigger time " << Units::display(trigger_time, {"us", "ns"}) << " to event with trigger ID "
                   << evt->GetTriggerN();
        evt->SetTimeBegin(trigger_time);
        evt->SetTimeEnd(trigger_time);
    }

    double event_start = evt->GetTimeBegin();
    double event_end = evt->GetTimeEnd();
    LOG(DEBUG) << "event_start = " << Units::display(event_start, "us")
               << ", event_end = " << Units::display(event_end, "us");

    // If adjustment of event start/end is required:
    const auto it = std::find_if(adjust_event_times.begin(),
                                 adjust_event_times.end(),
                                 [evt](const std::vector<std::string>& x) { return x.front() == evt->GetDescription(); });

    // Skip if later start is requested:
    if(event_start < m_skip_time) {
        LOG(DEBUG) << "Event start before requested skip time: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(m_skip_time, {"us", "ns"});
        return EventPosition::BEFORE;
    }

    double shift_start = 0;
    double shift_end = 0;

    if(!clipboard->event_defined()) {
        LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
        if(it != adjust_event_times.end()) {
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
    }

    double clipboard_start = clipboard->get_event()->start();
    double clipboard_end = clipboard->get_event()->end();

    if(event_start < clipboard_start) {
        LOG(DEBUG) << "Event start before Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(clipboard_start, {"us", "ns"});
        return EventPosition::BEFORE;
    } else if(clipboard_end < event_end) {
        LOG(DEBUG) << "Event end after Corryvreckan event: " << Units::display(event_end, {"us", "ns"}) << " > "
                   << Units::display(clipboard_end, {"us", "ns"});
        return EventPosition::AFTER;
    } else {
        // Store potential trigger numbers, assign to center of event:
        clipboard->get_event()->addTrigger(evt->GetTriggerN(), event_start - shift_start);
        LOG(DEBUG) << "Stored trigger ID " << evt->GetTriggerN() << " at "
                   << Units::display(event_start - shift_start, {"us", "ns"});

        if(evt->GetTriggerN() >= old_trigger_id + 2) {
            LOG(DEBUG) << "Trigger ID jumps from " << old_trigger_id << " to " << evt->GetTriggerN();
        }
        old_trigger_id = evt->GetTriggerN();
        return EventPosition::DURING;
    }
}

void EventLoaderEUDAQ2::store_data(std::shared_ptr<Clipboard> clipboard, std::shared_ptr<eudaq::StandardEvent> evt) {

    Pixels* pixels = new Pixels();

    // Loop over all planes, select the relevant detector:
    for(size_t i_plane = 0; i_plane < evt->NumPlanes(); i_plane++) {
        auto plane = evt->GetPlane(i_plane);

        // Concatenate plane name according to naming convention: sensor_type + "_" + int
        auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
        auto detector_name = m_detector->name();
        //  LOG(DEBUG) << plane_name <<", "<<detector_name;
        // Convert to lower case before string comparison to avoid errors by the user:
        std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
        std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);
        LOG(TRACE) << plane_name << " with  " << plane.HitPixels() << " hit pixels";

        if(detector_name != plane_name) {
            LOG(DEBUG) << "Wrong plane: " << detector_name << "!=" << plane_name << ". Continue.";
            continue;
        }

        LOG(DEBUG) << "Found correct plane.";
        // Loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < plane.HitPixels(); i++) {
            auto col = static_cast<int>(plane.GetX(i));
            auto row = static_cast<int>(plane.GetY(i));
            auto raw = static_cast<int>(plane.GetPixel(i)); // generic pixel raw value (could be ToT, ADC, ...)
            auto ts = plane.GetTimestamp(i);

            LOG(DEBUG) << "Read pixel (col, row) = (" << col << ", " << row << ") from EUDAQ2 event data (before masking).";
            if(m_detector->masked(col, row)) {
                continue;
            }

            // when calibration is not available, set charge = raw
            Pixel* pixel = new Pixel(m_detector->name(), col, row, raw, raw, ts);

            hitmap->Fill(col, row);
            hHitTimes->Fill(ts);
            hPixelRawValues->Fill(raw);

            auto event = clipboard->get_event();
            hTluChipTimeResidual->Fill(
                static_cast<double>(Units::convert(event->start() - ts, "us") + 115)); // revert adjust_event_times: 10us
            hTluChipTimeResidualvsTime->Fill(
                static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                static_cast<double>(Units::convert(event->start() - ts, "us") + 115)); // revert adjust_event_times 10us

            pixels->push_back(pixel);
        }
        // hPixelsPerEvent->Fill(static_cast<int>(pixels->size()));
        cnt_pixelsPerEvent += static_cast<int>(pixels->size());
    }

    if(!pixels->empty()) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " has " << pixels->size() << " pixels";
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
    } else {
        delete pixels;
    }
}

StatusCode EventLoaderEUDAQ2::run(std::shared_ptr<Clipboard> clipboard) {

    EventPosition current_position = EventPosition::UNKNOWN;
    while(1) {
        // Retrieve next event from file/buffer:
        if(!event_) {
            try {
                event_ = get_next_event();
            } catch(EndOfFile&) {
                return StatusCode::EndRun;
            }
        }

        // Check if this event is within the currently defined Corryvreckan event:
        current_position = is_within_event(clipboard, event_);

        if(current_position == EventPosition::DURING) {
            LOG(DEBUG) << "Is within current event, storing data";
            // Store data on the clipboard
            store_data(clipboard, event_);
        }

        // If this event was after the current event, stop reading:
        if(current_position == EventPosition::AFTER) {
            break;
        }

        // Do not fill if current_position == EventPosition::AFTER to avoid double-counting!
        hEudaqEventStart->Fill(event_->GetTimeBegin());
        if(clipboard->event_defined()) {
            hClipboardEventStart->Fill(clipboard->get_event()->start());
        }

        // Reset this event to get a new one:
        event_.reset();
    }

    LOG(DEBUG) << "Finished Corryvreckan event";
    hPixelsPerEvent->Fill(cnt_pixelsPerEvent);
    cnt_pixelsPerEvent = 0;

    return StatusCode::Success;
}
