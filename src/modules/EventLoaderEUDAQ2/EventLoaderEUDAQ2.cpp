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
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

using namespace corryvreckan;

EventLoaderEUDAQ2::EventLoaderEUDAQ2(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_filename = m_config.getPath("file_name", true);
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

    // Create vector of pixels:
    Pixels* pixels = new Pixels();

    // Read next event from EUDAQ2 reader:
    m_eventNumber++;
    auto evt = reader->GetNextEvent();
    if(!evt) {
        LOG(DEBUG) << "!ev --> return, empty event!";
        return StatusCode::NoData;
    }

    LOG(DEBUG) << "#ev: " << evt->GetEventNumber() << ", descr " << evt->GetDescription();

    // prepare standard event:
    auto stdev = eudaq::StandardEvent::MakeShared();

    // Convert event to standard event:
    if(eudaq::StdEventConverter::Convert(evt, stdev, nullptr)) {
        if(stdev->NumPlanes() == 0) {
            // return if plane is empty
            LOG(DEBUG) << "empty plane";
            return StatusCode::NoData;
        }
        LOG(INFO) << "number of planes: " << stdev->NumPlanes();
        // for now assume only 1 plane (for telescope think again!!!
        auto plane = stdev->GetPlane(0);
        auto nHits = plane.GetPixels<int>().size(); // number of hits
        auto nPixels = plane.TotalPixels();         // total pixels in matrix
        LOG(INFO) << "Number of Hits: " << nHits << " / total pixel number: " << nPixels;

        // loop over all hits and add to pixels vector:
        for(unsigned int i = 0; i < nHits; i++) {
            LOG(INFO) << "\t x: " << plane.GetX(i, 0) << " y: " << plane.GetY(i, 0) << " tot: " << plane.GetPixel(i)
                      << " ts: " << Units::display(plane.GetTimestamp(i), {"ns", "us", "ms"});
            Pixel* pixel = new Pixel(m_detector->name(),
                                     static_cast<int>(plane.GetY(i, 0)),
                                     static_cast<int>(plane.GetX(i, 0)),
                                     static_cast<int>(plane.GetPixel(i)),
                                     plane.GetTimestamp(i));
            pixels->push_back(pixel);
        }
    }
    // Put the data on the clipboard
    if(!pixels->empty()) {
        clipboard->put(m_detector->name(), "pixels", reinterpret_cast<Objects*>(pixels));
    } else {
        // if empty, clean up --> delete pixels object
        delete pixels;
        return StatusCode::NoData;
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderEUDAQ2::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
