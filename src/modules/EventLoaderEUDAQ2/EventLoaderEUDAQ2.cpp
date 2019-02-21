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

void EventLoaderEUDAQ2::convert_to_std_event(eudaq::EventSPC evt, std::shared_ptr<Clipboard> clipboard) {

    // Create vector of pixels:
    Pixels* pixels = new Pixels();

    // Prepare standard event:
    auto stdev = eudaq::StandardEvent::MakeShared();

    // Convert event to standard event:
    if(!(eudaq::StdEventConverter::Convert(evt, stdev, nullptr))) {
        LOG(DEBUG) << "eudaq::StdEventConverter -> cannot convert.";
        // return StatusCode::NoData;
    } else {
        if(stdev->NumPlanes() == 0) {
            LOG(DEBUG) << "No plane found in event.";
            return;
        }
        LOG(INFO) << "Number of planes: " << stdev->NumPlanes();
        // Loop over all planes and take only the one corresponding to current detector:
        for(size_t i_plane = 0; i_plane < stdev->NumPlanes(); i_plane++) {

            auto plane = stdev->GetPlane(i_plane);
            // Concatenate plane name according to naming convention: "sensor_type + int"
            auto plane_name = plane.Sensor() + "_" + std::to_string(i_plane);
            if(m_detector->name() != plane_name) {
                LOG(DEBUG) << "Wrong plane, continue. Detector: " << m_detector->name() << " != " << plane_name;
                continue;
            }
            auto nHits = plane.GetPixels<int>().size(); // number of hits
            auto nPixels = plane.TotalPixels();         // total pixels in matrix
            LOG(INFO) << "Number of hits: " << nHits << " / total pixel number: " << nPixels;

            LOG(DEBUG) << "Type: " << plane.Type() << " Name: " << plane.Sensor();
            // Loop over all hits and add to pixels vector:
            for(unsigned int i = 0; i < nHits; i++) {
                LOG(INFO) << "\t x: " << plane.GetX(i, 0) << " y: " << plane.GetY(i, 0) << " tot: " << plane.GetPixel(i)
                          << " ts: " << Units::display(plane.GetTimestamp(i), {"ns", "us", "ms"});
                Pixel* pixel = new Pixel(m_detector->name(),
                                         static_cast<int>(plane.GetY(i, 0)),
                                         static_cast<int>(plane.GetX(i, 0)),
                                         static_cast<int>(plane.GetPixel(i)),
                                         plane.GetTimestamp(i));
                pixels->push_back(pixel);
            } // loop over hits
        }     // loop over planes
    }         // NumPlanes > 0

    // Check if event is fully inside frame of previous detector:
    auto evt_start = stdev->GetTimestampBegin();
    auto evt_stop = stdev->GetTimestampEnd();

    LOG(DEBUG) << "Event time: " << Units::display(evt_start, {"ns", "us", "s"})
               << ", length: " << Units::display((evt_stop - evt_start), {"ns", "us", "s"});
    // clipboard->put_persistent("eventStart", shutterStartTime);
    // clipboard->put_persistent("eventEnd", shutterStopTime);
    // clipboard->put_persistent("eventLength", (shutterStopTime - shutterStartTime));

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

    // Initialise member variables
    m_eventNumber = 0;
    m_totalEvents = 0;

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

    // Read next event from EUDAQ2 reader:
    m_eventNumber++;
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
    LOG(DEBUG) << "# sub events : " << sub_events.size();

    if(sub_events.size() == 0) {
        // convert event:
        convert_to_std_event(evt, clipboard);

    } else {
        // loop over subevents:
        for(auto& subevt : (sub_events)) {
            LOG(DEBUG) << "\t subevt : " << subevt->GetDescription() << " ts_beg = " << subevt->GetTimestampBegin()
                       << " ts_end = " << subevt->GetTimestampEnd();
            // convert subevtent (use function here)
            convert_to_std_event(subevt, clipboard);
        }
    } // end else

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderEUDAQ2::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
    LOG(DEBUG) << "eudaq::cstr2hash(\"TriggerIDSyncOnline\") = " << eudaq::cstr2hash("TriggerIDSyncOnline");
    LOG(DEBUG) << "eudaq::cstr2hash(\"TluRawDataEvent\") = " << eudaq::cstr2hash("TluRawDataEvent");
}
