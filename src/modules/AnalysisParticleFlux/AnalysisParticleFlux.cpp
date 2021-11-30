/**
 * @file
<<<<<<< HEAD:src/modules/AnalysisParticleFlux/AnalysisParticleFlux.cpp
 * @brief Implementation of module AnalysisParticleFlux
=======
 * @brief Implementation of module AnalysisAnalysisParticleFlux
>>>>>>> 723262fb9042cbbac925fc9e2dc3559681c9034b:src/modules/ParticleFlux/ParticleFlux.cpp
 *
 * @copyright Copyright (c) 2021 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisParticleFlux.h"

using namespace corryvreckan;

AnalysisParticleFlux::AnalysisParticleFlux(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    // Setting config defaults
    // Azimuthal histogram
    config_.setDefault<double>("azimuthLow", Units::get<double>(0, "deg"));
    config_.setDefault<double>("azimuthHigh", Units::get<double>(360, "deg"));
    config_.setDefault<int>("azimuthGranularity", 36);
    // Zenith histogram
    config_.setDefault<double>("zenithLow", Units::get<double>(0, "deg"));
    config_.setDefault<double>("zenithHigh", Units::get<double>(90, "deg"));
    config_.setDefault<int>("zenithGranularity", 9);
    // Track intercept
    config_.setDefault<double>("track_intercept", 0.0);

    // Read configuration settings
    // Track intercept
    m_trackIntercept = config_.get<double>("track_intercept");
    // Granularities
    m_azimuthGranularity = config_.get<int>("azimuthGranularity");
    m_zenithGranularity = config_.get<int>("zenithGranularity");
    // Histogram bounds
    m_azimuthLow = config_.get<double>("azimuthLow");
    m_azimuthHigh = config_.get<double>("azimuthHigh");
    m_zenithLow = config_.get<double>("zenithLow");
    m_zenithHigh = config_.get<double>("zenithHigh");
}

void AnalysisParticleFlux::initialize() {

    // Initialise histograms
    m_azimuthHistogram = new TH1F("azimuth",
                                  "Azimuthal distribution of tracks;#varphi [rad];# entries",
                                  m_azimuthGranularity,
                                  m_azimuthLow,
                                  m_azimuthHigh);
    m_zenithHistogram = new TH1F("zenith",
                                 "Zenith angle distribution of tracks;#vartheta [rad];# entries",
                                 m_zenithGranularity,
                                 m_zenithLow,
                                 m_zenithHigh);
    m_combinedHistogram = new TH2F("zenith_vs_azimuth",
                                   "Zenith angle vs azimuth;#varphi [rad];#vartheta [rad]",
                                   m_azimuthGranularity,
                                   m_azimuthLow,
                                   m_azimuthHigh,
                                   m_zenithGranularity,
                                   m_zenithLow,
                                   m_zenithHigh);

    // Initialise member variables
    m_eventNumber = 0;
    m_numberOfTracks = 0;
}

/**
 * @brief [Calculate zenith and azimuth, fill histograms]
 */
void AnalysisParticleFlux::calculateAngles(Track* track) {
    ROOT::Math::XYZVector track_direction = track->getDirection(m_trackIntercept);
    double phi = track_direction.Phi();
    double theta = track_direction.theta();
    m_azimuthHistogram->Fill(phi);
    m_zenithHistogram->Fill(theta);
    m_combinedHistogram->Fill(phi, theta);
}

StatusCode AnalysisParticleFlux::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all tracks, calculate all angles and fill histograms
    auto tracks = clipboard->getData<Track>();
    for(auto& track : tracks) {
        calculateAngles(track.get());
        m_numberOfTracks++;
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisParticleFlux::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
    LOG(INFO) << "Analysed " << m_numberOfTracks << " tracks";
}