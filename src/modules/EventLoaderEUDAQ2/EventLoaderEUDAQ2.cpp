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

void EventLoaderEUDAQ2::get_event_times(std::pair<double, double>& evt_times, std::shared_ptr<Clipboard>& clipboard) {

    if(clipboard->event_defined()) {
        LOG(DEBUG) << "\tEvent found on clipboard:";
        evt_times.first = clipboard->get_event()->start();
        evt_times.second = clipboard->get_event()->end();
    } else {
        LOG(DEBUG) << "\tNo event found on clipboard. New event times: ";
        clipboard->put_event(std::make_shared<Event>(evt_times.first, evt_times.second));
    } // end else

    LOG(DEBUG) << "\t--> start = " << Units::display(evt_times.first, {"ns", "us", "ms", "s"})
               << ", end = " << Units::display(evt_times.second, {"ns", "us", "ms", "s"})
               << ", length = " << Units::display(evt_times.second - evt_times.first, {"ns", "us", "ms", "s"});
    return;
}

void EventLoaderEUDAQ2::process_event(eudaq::EventSPC evt, std::shared_ptr<Clipboard>& clipboard) {

    LOG(DEBUG) << "\tEvent description: " << evt->GetDescription() << ", ts_begin = " << evt->GetTimestampBegin()
               << " lsb, ts_end = " << evt->GetTimestampEnd() << " lsb";

    std::pair<double, double> event_times(-1., -1.); // initialize to illogical value

    // If TLU event: don't convert to standard event but only use time information
    if(evt->GetDescription() == "TluRawDataEvent") {
        LOG(DEBUG) << "\t--> Found TLU Event.";

        event_times.first = static_cast<double>(evt->GetTimestampBegin()) * 25.; // 40 MHz --> ns
        event_times.second = static_cast<double>(evt->GetTimestampEnd()) * 25.;  // 40 MHz --> ns
        get_event_times(event_times, clipboard);

        LOG(DEBUG) << "after get_event_times(): event_times.first = "
                   << Units::display(event_times.first, {"ns", "us", "ms", "s"})
                   << ", event_times.second = " << Units::display(event_times.second, {"ns", "us", "ms", "s"});
        return;
    } // end if

    // Create vector of pixels:
    Pixels* pixels = new Pixels();

    // Prepare standard event:
    auto stdevt = eudaq::StandardEvent::MakeShared();

    // Convert event to standard event:
    if(!(eudaq::StdEventConverter::Convert(evt, stdevt, nullptr))) {
        LOG(DEBUG) << "eudaq::StdEventConverter -> cannot convert.";
        return;
    }

    if(stdevt->NumPlanes() == 0) {
        LOG(DEBUG) << "No plane found in event.";
        return;
    }

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
            LOG(INFO) << "\t\t x: " << plane.GetX(i, 0) << "\ty: " << plane.GetY(i, 0) << "\ttot: " << plane.GetPixel(i)
                      << "\tts: " << Units::display(plane.GetTimestamp(i), {"ns", "us", "ms"});
            Pixel* pixel = new Pixel(m_detector->name(),
                                     static_cast<int>(plane.GetY(i, 0)),
                                     static_cast<int>(plane.GetX(i, 0)),
                                     static_cast<int>(plane.GetPixel(i)),
                                     plane.GetTimestamp(i));
            pixels->push_back(pixel);
        } // loop over hits
    }     // loop over planes

    // get event begin/end:
    event_times.first = stdevt->GetTimeBegin();
    event_times.second = stdevt->GetTimeEnd();
    get_event_times(event_times, clipboard);
    LOG(DEBUG) << "after calling get_event_times(): event_times.first = "
               << Units::display(event_times.first, {"ns", "us", "ms", "s"})
               << ", event_times.second = " << Units::display(event_times.second, {"ns", "us", "ms", "s"});

    // Check if event is fully inside frame:
    // ...
    // ... think about logic here ...
    // ...

    // Put the pixel data on the clipboard:
    if(!pixels->empty()) {
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
        LOG(DEBUG) << "Add pixels to clipboard.";
    } else {
        // if empty, clean up --> delete pixels object
        delete pixels;
        return;
    }

    return;
}

void EventLoaderEUDAQ2::initialise() {

    auto detectorID = m_detector->name();
    LOG(DEBUG) << "Initialise for detector " + detectorID;

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

        // loop over subevents and process only TLU event:
        for(auto& subevt : sub_events) {
            LOG(DEBUG) << "Processing subevent.";
            if(subevt->GetDescription() != "TluRawDataEvent") {
                LOG(DEBUG) << "\t---> subevent is no TLU event -> continue.";
                continue;
            } // end if
            process_event(subevt, clipboard);
        } // end for
        // loop over subevents and process all other events (except TLU):
        for(auto& subevt : sub_events) {
            LOG(DEBUG) << "Processing subevent.";
            if(subevt->GetDescription() == "TluRawDataEvent") {
                LOG(DEBUG) << "\t---> subevent is no TLU event -> continue.";
                continue;
            } // end if
            process_event(subevt, clipboard);
        } // end for

    } // end else

    return StatusCode::Success;
}
