/**
 * @file
 * @brief Implementation of module ParticleFlux
 *
 * @copyright Copyright (c) 2021 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ParticleFlux.h"

using namespace corryvreckan;

ParticleFlux::ParticleFlux(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    // Setting config defaults
    // Azimuthal histogram
    config_.setDefault<double>("azimuth_low", Units::get<double>(0, "degree"));
    config_.setDefault<double>("azimuth_high", Units::get<double>(360, "degree"));
    config_.setDefault<int>("azimuth_granularity", 36);
    // Zenith histogram
    config_.setDefault<double>("zenith_low", Units::get<double>(0, "degree"));
    config_.setDefault<double>("zenith_high", Units::get<double>(90, "degree"));
    config_.setDefault<int>("zenith_granularity", 9);

    // Read configuration settings
    // Granularities
    m_azimuth_granularity = config_.get<int>("azimuth_granularity");
    m_zenith_granularity = config_.get<int>("zenith_granularity");
    // Histogram bounds
    m_azimuth_low = config_.get<float>("azimuth_low");
    m_azimuth_high = config_.get<float>("azimuth_high");
    m_zenith_low = config_.get<float>("zenith_low");
    m_zenith_high = config_.get<float>("zenith_high");
}

void ParticleFlux::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ParticleFlux::run(const std::shared_ptr<Clipboard>&) {

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

void ParticleFlux::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
