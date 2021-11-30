/**
 * @file
 * @brief Implementation of module Particle Flux
 *
 * @copyright Copyright (c) 2017-2021 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ParticleFlux.h"

using namespace corryvreckan;
using namespace std;

ParticleFlux::ParticleFlux(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    // TODO: Initialize the config and get values
}

void ParticleFlux::initialize() {

    // TODO: Initialise histograms
}

StatusCode ParticleFlux::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all tracks, calculate all angles and fill histograms
    auto tracks = clipboard->getData<Track>();
    for(auto& track : tracks) {

        // TODO: Calculate the angles
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void ParticleFlux::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    std::stringstream config;
    // TODO: Fill stringstream with the module output
    LOG(INFO) << "\"ParticleFlux\":" << config.str();
}