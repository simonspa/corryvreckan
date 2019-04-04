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
    hHitTimes = new TH1F("hitTimes", title.c_str(), 3e6, 0, 3e12);

    title = "pixel values (depending on chip functionality: pixelToT, ADC, ...);pixel values [a.u.];# events";
    hPixelValues = new TH1F("hPixelValues", title.c_str(), 1024, 0, 1024);

    title = "Pixel multiplicity per frame;# pixels per frame;# frames";
    hPixelsPerFrame = new TH1F("pixelsPerFrame", title.c_str(), 1000, 0, 1000);

    title = ";EUDAQ event start time[ns];# entries";
    hEudaqEventStart = new TH1D("eudaqEventStart", title.c_str(), 1e6, 0, 1e9);

    title = "Corryvreckan event start times (on clipboard); Corryvreckan event start time [ns];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 1e6, 0, 1e9);

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
                "Parameter \"adjust_event_times\" needs 3 values per row: event type, shift event start, shift");
        }
    } // end for
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
            LOG(DEBUG) << "Trigger ID not found in current event.";
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

    // If adjustment of event start/end is required:
    if(!adjust_event_times.empty()) {
        for(auto& shift_times : adjust_event_times) {
            if(shift_times.front() == (evt->GetDescription())) {
                LOG(DEBUG) << "Adjusting " << shift_times.at(0) << ": event_start by " << shift_times.at(1)
                           << ", event_end by " << shift_times.at(2);
                event_start += corryvreckan::from_string<double>(shift_times.at(1));
                event_end += corryvreckan::from_string<double>(shift_times.at(2));
            }
        } // end for
    }     // end if(do_adjust_event_times)

    // Skip if later start is requested:
    if(event_start < m_skip_time) {
        LOG(DEBUG) << "Event start before requested skip time: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(m_skip_time, {"us", "ns"});
        return EventPosition::BEFORE;
    }

    if(!clipboard->event_defined()) {
        LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
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
        clipboard->get_event()->addTrigger(evt->GetTriggerN(), (event_start + event_start) / 2);
        LOG(DEBUG) << "Stored trigger ID " << evt->GetTriggerN() << " at "
                   << Units::display((event_start + event_start) / 2, {"us", "ns"});
        return EventPosition::DURING;
    }
}

void EventLoaderEUDAQ2::store_data(std::shared_ptr<Clipboard> clipboard, std::shared_ptr<eudaq::StandardEvent> evt) {

    Pixels* pixels = new Pixels();

    // Loop over all planes, select the relevant detector:
    for(size_t i_plane = 0; i_plane < evt->NumPlanes(); i_plane++) {
        auto plane = evt->GetPlane(i_plane);

        // Concatenate plane name according to naming convention: sensor_type + "_" + int
        auto plane_name = plane.Sensor() + "_" + std::to_string(i_plane);
        if(m_detector->name() != plane_name) {
            continue;
        }

        // Loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < plane.GetPixels<int>().size(); i++) {
            auto col = static_cast<int>(plane.GetX(i));
            auto row = static_cast<int>(plane.GetY(i));
            auto value = static_cast<int>(plane.GetPixel(i)); // generic pixel value (could be ToT, ADC, ...)
            auto ts = plane.GetTimestamp(i);

            LOG(DEBUG) << "col " << col << ", row " << row;
            if(m_detector->masked(col, row)) {
                continue;
            }

            // Note: in many cases, the pixel value corresponds to the pixel ToT or ADC value:
            Pixel* pixel = new Pixel(m_detector->name(), row, col, value, ts);

            hitmap->Fill(col, row);
            hHitTimes->Fill(ts);
            hPixelValues->Fill(value);
            pixels->push_back(pixel);
        }
        hPixelsPerFrame->Fill(static_cast<int>(pixels->size()));
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
            LOG(DEBUG) << "Is withing current event, storing data";
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
    return StatusCode::Success;
}
