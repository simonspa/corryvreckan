/**
 * @file
 * @brief Implementation of module EventLoaderWaveform
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderWaveform.h"

using namespace corryvreckan;

EventLoaderWaveform::EventLoaderWaveform(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    m_inputDirectory = config_.getPath("input_directory");
    m_channels = config_.getArray("channels");
}

void EventLoaderWaveform::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Initialise member variables
    m_eventNumber = 0;
    m_loader = std::make_unique(m_inputDirectory, m_channels);
}

StatusCode EventLoaderWaveform::run(const std::shared_ptr<Clipboard>&) {

    // Loop over all detectors
    for(auto& detector : get_detectors()) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderWaveform::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
