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
        clipboard->put_event(std::make_shared<Event>(start, end));
        evt_times = {start, end};
    } // end else

    LOG(DEBUG) << "\t--> start = " << Units::display(evt_times.first, {"ns", "us", "ms", "s"})
               << ", end = " << Units::display(evt_times.second, {"ns", "us", "ms", "s"})
               << ", length = " << Units::display(evt_times.second - evt_times.first, {"ns", "us", "ms", "s"});
    return evt_times;
}

void EventLoaderEUDAQ2::process_event(eudaq::EventSPC evt, std::shared_ptr<Clipboard>& clipboard) {

    LOG(DEBUG) << "\tEvent description: " << evt->GetDescription() << ", ts_begin = " << evt->GetTimestampBegin()
               << " lsb, ts_end = " << evt->GetTimestampEnd() << " lsb"
               << ", trigN = " << evt->GetTriggerN();

    // If TLU event: don't convert to standard event but only use time information
    if(evt->GetDescription() == "TluRawDataEvent") {
        LOG(DEBUG) << "\t--> Found TLU Event.";

        std::pair<double, double> current_event_times;
        current_event_times.first = static_cast<double>(evt->GetTimestampBegin());
        // Do not use:
        // "evt_end = static_cast<double>(evt->GetTimestampEnd());"
        // because this is only 25ns larger than begin and doesn't have a meaning!
        // Instead hardcode:
        current_event_times.second =
            current_event_times.first + 115000; // 115 us until rolling shutter goes across full matrix
        auto clipboard_event_times = get_event_times(current_event_times.first, current_event_times.second, clipboard);

        hEventBegin->Fill(current_event_times.first);

        LOG(DEBUG) << "Check current_event_times.first = "
                   << Units::display(current_event_times.first, {"ns", "us", "ms", "s"}) << ", current_event_times.second = "
                   << Units::display(current_event_times.second, {"ns", "us", "ms", "s"});
        LOG(DEBUG) << "Check clipboard_event_times.first = "
                   << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"})
                   << ", clipboard_event_times.second = "
                   << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});

        // use knowledge of 115 us frame length for rolling shutter to go across full matrix:
        if(current_event_times.first + 0.5e6 < clipboard_event_times.first) {
            LOG(INFO) << "Frame dropped because frame begins BEFORE event: "
                      << Units::display(current_event_times.first, {"ns", "us", "ms", "s"}) << " earlier than "
                      << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"});
            return;
        }

        if(current_event_times.second - 0.5e6 > clipboard_event_times.second) {
            LOG(INFO) << "Frame dropped because frame begins AFTER event: "
                      << Units::display(current_event_times.second, {"ns", "us", "ms", "s"}) << " later than "
                      << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});
            return;
        }

        // if frame is not rejected: add trigger ID --> use it for Mimosa hits to check if reject or not
        if(!clipboard->event_defined()) {
            LOG(WARNING) << "No event defined! Something went wrong!";
            return;
        }
        LOG(DEBUG) << "-------------- adding triggerID " << evt->GetTriggerN();
        clipboard->get_event()->add_trigger_id(evt->GetTriggerN());
        return;
    } // end if

    // // // // // // // //
    // If other than TLU:
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
            LOG(DEBUG) << "Frame dropped because event does not contain triggerID " << evt->GetTriggerN();
        } else {
            LOG(DEBUG) << "Found trigger ID.";
        }
    }
    // For chips with valid hit timestamps:
    else {
        if(current_event_times.first < clipboard_event_times.first) {
            LOG(INFO) << "Frame dropped because frame begins BEFORE event: "
                      << Units::display(current_event_times.first, {"ns", "us", "ms", "s"}) << " earlier than "
                      << Units::display(clipboard_event_times.first, {"ns", "us", "ms", "s"});
            return;
        }
        if(current_event_times.second > clipboard_event_times.second) {
            LOG(INFO) << "Frame dropped because frame begins AFTER event: "
                      << Units::display(current_event_times.second, {"ns", "us", "ms", "s"}) << " later than "
                      << Units::display(clipboard_event_times.second, {"ns", "us", "ms", "s"});
            return;
        }
    } // end if not NiRawDataEvent

    // Create vector of pixels:
    Pixels* pixels = new Pixels();

    LOG(INFO) << "\tNumber of planes: " << stdevt->NumPlanes();
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
        LOG(INFO) << "\tNumber of hits: " << nHits << " / total pixel number: " << nPixels;

        LOG(DEBUG) << "\tType: " << plane.Type() << " Name: " << plane.Sensor();
        // Loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < nHits; i++) {

            auto col = static_cast<int>(plane.GetY(i)); // X/Y: This is counter-intuitive!
            auto row = static_cast<int>(plane.GetX(i)); // X/Y: This is counter-intuitive!
            auto tot = static_cast<int>(plane.GetPixel(i));
            auto ts = plane.GetTimestamp(i);

            LOG(INFO) << "\t\t x: " << col << "\ty: " << row << "\ttot: " << tot
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
}

StatusCode EventLoaderEUDAQ2::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "=====================\n==== Next event: ====\n=====================";
    auto evt = reader->GetNextEvent();
    if(!evt) {
        LOG(DEBUG) << "!ev --> return, empty event!";
        return StatusCode::NoData;
    }

    LOG(DEBUG) << "#ev: " << evt->GetEventNumber() << ", descr " << evt->GetDescription() << ", version "
               << evt->GetVersion() << ", type " << evt->GetType() << ", devN " << evt->GetDeviceN() << ", trigN "
               << evt->GetTriggerN() << ", evID " << evt->GetEventID();

    // check if there are subevents:
    // if not --> convert event, if yes, loop over subevents and convert each
    auto sub_events = evt->GetSubEvents();
    LOG(DEBUG) << "There are " << sub_events.size() << " sub events.";

    if(sub_events.size() == 0) {
        LOG(DEBUG) << "No subevent, process event.";
        process_event(evt, clipboard);

    } else {
        // Important: first process TLU event (if available) --> sets event begin/end, then others
        // Note: at the moment, we're not checking if there is a 2nd TLU subevent (but that shouldn't occur).

        // drop frame if number of subevents==1, i.e. there is only telescope but not tlu data or vice versa
        if(sub_events.size() == 1) {
            LOG(INFO) << "Dropping frame because there is only 1 subevent.";
            return StatusCode::NoData;
        }

        // loop over subevents and process ONLY TLU event:
        for(auto& subevt : sub_events) {
            LOG(DEBUG) << "Processing subevent.";
            if(subevt->GetDescription() != "TluRawDataEvent") {
                LOG(DEBUG) << "\t---> subevent is no TLU event -> continue.";
                continue;
            } // end if
            process_event(subevt, clipboard);
        } // end for

        // loop over subevents and process all OTHER events (except TLU):
        for(auto& subevt : sub_events) {
            LOG(DEBUG) << "Processing subevent.";
            if(subevt->GetDescription() == "TluRawDataEvent") {
                LOG(DEBUG) << "\t---> subevent is TLU event -> continue.";
                continue;
            } // end if
            process_event(subevt, clipboard);
        } // end for

    } // end else

    return StatusCode::Success;
}
