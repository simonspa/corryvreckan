/**
 * @file
 * @brief Implementation of [EventLoaderMuPix] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderMuPix.h"

using namespace corryvreckan;

EventLoaderMuPix::EventLoaderMuPix(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detector)) {}

void EventLoaderMuPix::initialise() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->name();
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EventLoaderMuPix::run(std::shared_ptr<Clipboard>) {

    // Loop over all detectors
    for(auto& detector : get_detectors()) {
        // Get the detector name
        std::string detectorName = detector->name();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderMuPix::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
