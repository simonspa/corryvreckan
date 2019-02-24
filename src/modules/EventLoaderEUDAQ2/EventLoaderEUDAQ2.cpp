/**
 * @file
 * @brief Implementation of [EventLoaderEUDAQ2] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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
}

std::pair<double, double>
EventLoaderEUDAQ2::get_event_times(double start, double end, std::shared_ptr<Clipboard>& clipboard) {

    std::pair<double, double> evt_times;
    if(clipboard->event_defined()) {
        LOG(DEBUG) << "\tEvent found on clipboard:";
        evt_times.first = clipboard->get_event()->start();
        evt_times.second = clipboard->get_event()->end();
    } else {
        LOG(DEBUG) << "\tNo event found on clipboard. New event times: ";
        // clipboard->put_event(std::make_shared<Event>(start-1e6, end+1e6)); // UGLY HACK: increasing event time interval
        // artificially!!!
        clipboard->put_event(std::make_shared<Event>(start, end));
        evt_times = {start, end};
    } // end else

    LOG(DEBUG) << "\t--> start = " << Units::display(evt_times.first, {"ns", "us", "ms", "s"})
               << ", end = " << Units::display(evt_times.second, {"ns", "us", "ms", "s"})
               << ", length = " << Units::display(evt_times.second - evt_times.first, {"ns", "us", "ms", "s"});
    return evt_times;
}

EventLoaderEUDAQ2::EventPosition EventLoaderEUDAQ2::process_tlu_event(eudaq::EventSPC evt,
                                                                      std::shared_ptr<Clipboard>& clipboard) {

    // If TLU event: don't convert to standard event but only use time information
    LOG(DEBUG) << "\tEvent description: " << evt->GetDescription() << ", ts_begin = " << evt->GetTimestampBegin()
               << " lsb, ts_end = " << evt->GetTimestampEnd() << " lsb"
               << ", trigN = " << evt->GetTriggerN();

    std::pair<double, double> current_event_times;
    std::pair<double, double> event_search_times;
    // parameters to
    // 115 us until rolling shutter goes across full matrix

    current_event_times.first = static_cast<double>(evt->GetTimestampBegin()) - 115000;  // in ns
    current_event_times.second = static_cast<double>(evt->GetTimestampBegin()) + 230000; // in ns
    event_search_times.first = static_cast<double>(evt->GetTimestampBegin()) + 100;      // in ns
    event_search_times.second = static_cast<double>(evt->GetTimestampBegin()) - 100;     // in ns
    // Do not use:
    // "evt_end = static_cast<double>(evt->GetTimestampEnd());"
    // because this is only 25ns larger than GetTimeStampBegin and doesn't have a meaning!

    auto clipboard_event_times = get_event_times(current_event_times.first, current_event_times.second, clipboard);

    hEventBegin->Fill(static_cast<double>(evt->GetTimestampBegin()));

    LOG(DEBUG) << "Check current_event_times.first = " << Units::display(current_event_times.first, {"ns", "us", "ms", "s"})
               << ", current_event_times.second = " << Units::display(current_event_times.second, {"ns", "us", "ms", "s"});
    LOG(DEBUG) << "Check clipboard_event_times.first = "
               << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"}) << ", clipboard_event_times.second = "
               << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});

    // Drop events which are outside of the clipboard time window:
    if(event_search_times.first < clipboard_event_times.first) {
        LOG(DEBUG) << "Frame dropped because frame begins BEFORE event: "
                   << Units::display(event_search_times.first, {"ns", "us", "ms", "s"}) << " earlier than "
                   << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"});
        return before_window;
    }

    if(event_search_times.second > clipboard_event_times.second) {
        LOG(DEBUG) << "Frame dropped because frame begins AFTER event: "
                   << Units::display(event_search_times.second, {"ns", "us", "ms", "s"}) << " later than "
                   << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});
        return after_window;
    }

    LOG(DEBUG) << "-------------- adding TriggerID " << evt->GetTriggerN();
    clipboard->get_event()->add_trigger_id(evt->GetTriggerN());
    return in_window;
}

void EventLoaderEUDAQ2::process_event(eudaq::EventSPC evt, std::shared_ptr<Clipboard>& clipboard) {

    LOG(DEBUG) << "\tEvent description: " << evt->GetDescription() << ", ts_begin = " << evt->GetTimestampBegin()
               << " lsb, ts_end = " << evt->GetTimestampEnd() << " lsb"
               << ", trigN = " << evt->GetTriggerN();

    // If other than TLU event:

    // Prepare standard event:
    auto stdevt = eudaq::StandardEvent::MakeShared();

    // Convert event to standard event:
    if(!(eudaq::StdEventConverter::Convert(evt, stdevt, nullptr))) {
        LOG(DEBUG) << "Cannot convert to StandardEvent.";
        return;
    }

    if(stdevt->NumPlanes() == 0) {
        LOG(DEBUG) << "No plane found in event.";
        return;
    }

    // Get event begin/end:
    std::pair<double, double> current_event_times;
    current_event_times.first = stdevt->GetTimeBegin();
    current_event_times.second = stdevt->GetTimeEnd();

    auto clipboard_event_times = get_event_times(current_event_times.first, current_event_times.second, clipboard);
    LOG(DEBUG) << "Check current_event_times.first = " << Units::display(current_event_times.first, {"ns", "us", "ms", "s"})
               << ", current_event_times.second = " << Units::display(current_event_times.second, {"ns", "us", "ms", "s"});
    LOG(DEBUG) << "Check clipboard_event_times.first = "
               << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"}) << ", clipboard_event_times.second = "
               << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});

    // If hit timestamps are invalid/non-existent (like for MIMOSA26 telescope),
    // we need to make sure there is a corresponding TLU event with the same triggerID:
    if(!clipboard->event_defined()) {
        LOG(WARNING) << "No event defined! Something went wrong!";
        return;
    }
    // Process MIMOSA26 telescope frames separately because they don't have sensible hit timestamps:
    // But do not use "if(evt->GetTimestampBegin()==0) {" because sometimes the timestamps are not 0 but 1ns
    // Pay attention to this when working with a different chip without hit timestamps!
    if(evt->GetDescription() == "NiRawDataEvent") {
        if(!clipboard->get_event()->has_trigger_id(evt->GetTriggerN())) {
            LOG(DEBUG) << "Frame dropped because event does not contain TriggerID " << evt->GetTriggerN();
        } else {
            LOG(DEBUG) << "Found TriggerID.";
        }
    }
    // For chips with valid hit timestamps:
    else {
        if(current_event_times.first < clipboard_event_times.first) {
            LOG(DEBUG) << "Frame dropped because frame begins BEFORE event: "
                       << Units::display(current_event_times.first, {"ns", "us", "ms", "s"}) << " earlier than "
                       << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"});
            return;
        }
        if(current_event_times.second > clipboard_event_times.second) {
            LOG(DEBUG) << "Frame dropped because frame begins AFTER event: "
                       << Units::display(current_event_times.second, {"ns", "us", "ms", "s"}) << " later than "
                       << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});
            return;
        }
    } // end if not NiRawDataEvent

    // Create vector of pixels:
    Pixels* pixels = new Pixels();

    LOG(DEBUG) << "\tNumber of planes: " << stdevt->NumPlanes();
    // Loop over all planes and take only the one corresponding to current detector:
    for(size_t i_plane = 0; i_plane < stdevt->NumPlanes(); i_plane++) {

        auto plane = stdevt->GetPlane(i_plane);
        // Concatenate plane name according to naming convention: "sensor_type + int"
        auto plane_name = plane.Sensor() + "_" + std::to_string(i_plane);
        if(m_detector->name() != plane_name) {
            LOG(DEBUG) << "\tWrong plane, continue. Detector: " << m_detector->name() << " != " << plane_name;
            continue;
        }

        auto nHits = plane.GetPixels<int>().size(); // number of hits
        auto nPixels = plane.TotalPixels();         // total pixels in matrix
        LOG(DEBUG) << "\tNumber of hits: " << nHits << " / total pixel number: " << nPixels;

        LOG(DEBUG) << "\tType: " << plane.Type() << " Name: " << plane.Sensor();
        // Loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < nHits; i++) {

            // auto col = static_cast<int>(plane.GetX(i));
            // auto row = static_cast<int>(plane.GetY(i));

            auto col = static_cast<int>(plane.GetX(i));
            auto row = static_cast<int>(plane.GetY(i));

            auto tot = static_cast<int>(plane.GetPixel(i));
            double ts;

            // for pixels without valid timestamp use event timestamp:
            if(evt->GetTimestampBegin() == 0) {
                ts = current_event_times.first;
            }
            // for valid pixel timestamps (like CLICpix2) use plane timestamp:
            else {
                ts = plane.GetTimestamp(i);
            }

            LOG(DEBUG) << "\t\t x: " << col << "\ty: " << row << "\ttot: " << tot
                       << "\tts: " << Units::display(ts, {"ns", "us", "ms"});

            // If this pixel is masked, do not save it
            if(m_detector->masked(col, row)) {
                continue;
            }
            Pixel* pixel = new Pixel(m_detector->name(), row, col, tot, ts);

            // Fill pixel-related histograms here:
            hitmap->Fill(pixel->column(), pixel->row());
            hHitTimes->Fill(pixel->timestamp()); // this doesn't always make sense, e.g. for MIMOSA telescope, there are no
                                                 // pixel timestamps

            pixels->push_back(pixel);
        } // loop over hits
    }     // loop over planes

    hPixelsPerFrame->Fill(static_cast<int>(pixels->size()));
    // hEventBegin->Fill(clipboard_event_times.first,static_cast<double>(pixels->size()));
    hEventBegin->Fill(clipboard_event_times.first);

    // Put the pixel data on the clipboard:
    if(!pixels->empty()) {
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
        LOG(DEBUG) << "Add pixels to clipboard.";
    } else {
        // if empty, clean up --> delete pixels object
        delete pixels;
    }

    return;
}

void EventLoaderEUDAQ2::initialise() {

    auto detectorID = m_detector->name();
    LOG(DEBUG) << "Initialise for detector " + detectorID;

    m_skipBeforeT0 = m_config.get<bool>("skip_before_t0", false);

    // Some simple histograms:
    std::string title = detectorID + ": hitmap;x [px];y [px];events";
    hitmap = new TH2F("hitmap",
                      title.c_str(),
                      m_detector->nPixels().X(),
                      0,
                      m_detector->nPixels().X(),
                      m_detector->nPixels().Y(),
                      0,
                      m_detector->nPixels().Y());
    title = detectorID + ": hitTimes;hitTimes [ns]; events";
    hHitTimes = new TH1F("hitTimes", title.c_str(), 3000000, 0, 3e12);

    title = m_detector->name() + " Pixel multiplicity;pixels;frames";
    hPixelsPerFrame = new TH1F("pixelsPerFrame", title.c_str(), 1000, 0, 1000);
    title = m_detector->name() + " eventBegin;eventBegin [ns];entries";
    hEventBegin = new TH1D("eventBegin", title.c_str(), 1e6, 0, 1e9);

    // open the input file with the eudaq reader
    try {
        reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << m_filename
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(m_config, "file_path", "Parsing error!");
    }

    // Read first event to a global variable:
    if(!m_skipBeforeT0) {
        current_evt = reader->GetNextEvent();
    }
    // Skip everything before T0:
    else {
        LOG(INFO) << "Skipping all events before T0.";

        unsigned int prev_event_number = 0;
        long unsigned prev_timestamp = 0;
        while(1) {
            current_evt = reader->GetNextEvent();
            if(!current_evt) {
                LOG(DEBUG) << "Previous event number: " << prev_event_number
                           << ". Reached end of file without finding T0! Something's wrong!";
                return;
            } else {
                LOG(DEBUG) << "Looking at event number " << current_evt->GetEventNumber();
            }

            auto sub_events = current_evt->GetSubEvents();
            LOG(DEBUG) << "There are " << sub_events.size() << " sub events.";

            auto this_event_number = current_evt->GetEventNumber();
            long unsigned int this_timestamp = 0;
            bool found_timestamp = false;

            if(sub_events.size() == 0) {
                LOG(DEBUG) << "\tNo subevent, process event.";
                this_timestamp = current_evt->GetTimestampBegin();
                found_timestamp = true;
            }
            // If there are subevents, find TLU subevent:
            else {
                LOG(INFO) << "There is no T0 in TLU events. Don't skip anything, start from file beginning.";
                return;
            } // end else (there are subevents)

            if(found_timestamp) {
                if(this_timestamp < prev_timestamp) {
                    LOG(INFO) << "Found T0 at timestamp = " << this_timestamp
                              << " lsb after prev_timestamp = " << prev_timestamp << " lsb at event number "
                              << this_event_number;
                    break;
                } // end if
                prev_timestamp = this_timestamp;
                prev_event_number = this_event_number;

            } else {
                LOG(WARNING) << "In event " << this_event_number << ": Couldn't find a timestamp! Something is wrong!";
            }
        } // end while(1)
    }     // end if skip before t0
}

StatusCode EventLoaderEUDAQ2::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "=====================\n==== Next event: ====\n=====================";
    // So this means that CLICpix2 frames without hits are discarded?
    // Consequently we can't measure the efficiency of the CLICpix2!
    //
    // This also means we keep running forever because no event != end_of_file

    // Global variable with an event stored from init or previous run: current_evt

    // explain
    while(1) {
        if(!current_evt) {
            LOG(DEBUG) << "!ev --> return, empty event --> end of file!";
            return StatusCode::Failure;
        }

        LOG(DEBUG) << "#ev: " << current_evt->GetEventNumber() << ", descr " << current_evt->GetDescription() << ", version "
                   << current_evt->GetVersion() << ", type " << current_evt->GetType() << ", devN "
                   << current_evt->GetDeviceN() << ", trigN " << current_evt->GetTriggerN() << ", evID "
                   << current_evt->GetEventID() << ", ts_begin " << current_evt->GetTimestampBegin() << ", ts_end "
                   << current_evt->GetTimestampEnd();

        // check if there are subevents:
        // if not --> convert event, if yes, loop over subevents and convert each
        auto sub_events = current_evt->GetSubEvents();
        LOG(DEBUG) << "There are " << sub_events.size() << " sub events.";

        if(sub_events.size() == 0) {
            LOG(DEBUG) << "No subevent, process event.";
            process_event(current_evt, clipboard);
            // read next event for next run:
            current_evt = reader->GetNextEvent();
            return StatusCode::Success;

        } else {
            // Important: first process TLU event (if available) --> sets event begin/end, then others
            // Note: at the moment, we're not checking if there is a 2nd TLU subevent (but that shouldn't occur).

            // drop frame if number of subevents==1, i.e. there is only telescope but not tlu data or vice versa
            if(sub_events.size() == 1) {
                LOG(INFO) << "Dropping frame because there is only 1 subevent of type " << sub_events[0]->GetDescription();
                current_evt = reader->GetNextEvent();
                return StatusCode::NoData;
            }

            // loop over subevents and process ONLY TLU event:
            bool found_tlu_event = false;
            enum EventPosition event_position;
            for(auto& subevt : sub_events) {
                LOG(DEBUG) << "Processing subevent.";
                if(subevt->GetDescription() != "TluRawDataEvent") {
                    LOG(DEBUG) << "\t---> Subevent is no TLU event -> continue.";
                    continue;
                } // end if
                LOG(DEBUG) << "\t---> Found TLU subevent -> process.";
                found_tlu_event = true;
                event_position = process_tlu_event(subevt, clipboard);
                break;
            } // end for

            if(!found_tlu_event) {
                LOG(WARNING) << "Did not find TLU subevent. Something is wrong.";
                current_evt = reader->GetNextEvent();
                continue;
            }

            // If before, read next event and check again:
            if(event_position == before_window) {
                LOG(DEBUG) << "Trigger is before event window. Read next event and continue.";
                current_evt = reader->GetNextEvent();
                continue;
            }

            // if in,
            if(event_position == in_window) {
                LOG(DEBUG) << "Trigger is inside event window. Loop over subevents.";
                // loop over subevents and process all OTHER events (except TLU):
                for(auto& subevt : sub_events) {
                    LOG(DEBUG) << "Processing subevent.";
                    if(subevt->GetDescription() == "TluRawDataEvent") {
                        LOG(DEBUG) << "\t---> Subevent is TLU event -> continue.";
                        continue;
                    } // end if
                    process_event(subevt, clipboard);
                } // end for

                // read next event for next run() iteration
                current_evt = reader->GetNextEvent();
                continue;
            } // end if

            // if after:
            if(event_position == after_window) {
                LOG(DEBUG) << "Trigger is after event window. Finish event window.";
                return StatusCode::Success;
            }
        } // end else (sub_events.size() > 0)

    } // end while(1)
}
