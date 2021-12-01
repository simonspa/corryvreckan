/**
 * @file
 * @brief Implementation of module AnalysisParticleFlux
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
    config_.setDefault<double>("trackIntercept", 0.0);
    // Angle unit
    config_.setDefault<std::string>("angleUnit", "deg");

    // Read configuration settings
    // Track intercept
    m_trackIntercept = config_.get<double>("trackIntercept");
    // Granularities
    m_azimuthGranularity = config_.get<int>("azimuthGranularity");
    m_zenithGranularity = config_.get<int>("zenithGranularity");
    // Histogram bounds
    m_azimuthLow = config_.get<double>("azimuthLow");
    m_azimuthHigh = config_.get<double>("azimuthHigh");
    m_zenithLow = config_.get<double>("zenithLow");
    m_zenithHigh = config_.get<double>("zenithHigh");
    // Handle angle unit conversion
    m_angleUnit = config_.get<std::string>("angleUnit");
    if(m_angleUnit == "deg") {
        m_angleConversion = 180.0 / ROOT::Math::Pi();
        m_angleUnit = "#circ";
    } else if(m_angleUnit == "rad") {
        m_angleConversion = 1.0;
    } else if(m_angleUnit == "mrad") {
        m_angleConversion = 1000.0;
    } else {
        ModuleError("angleUnit can only be \"deg\", \"rad\" or \"mrad\"");
    }
}

void AnalysisParticleFlux::initialize() {

    // Initialise histograms
    m_azimuthHistogram = new TH1D("azimuth",
                                  ("Azimuthal distribution of tracks;#varphi [" + m_angleUnit + "];# tracks").c_str(),
                                  m_azimuthGranularity,
                                  m_azimuthLow * m_angleConversion,
                                  m_azimuthHigh * m_angleConversion);
    m_zenithHistogram = new TH1D("zenith",
                                 ("Zenith angle distribution of tracks;#vartheta [" + m_angleUnit + "];# tracks").c_str(),
                                 m_zenithGranularity,
                                 m_zenithLow * m_angleConversion,
                                 m_zenithHigh * m_angleConversion);
    m_combinedHistogram =
        new TH2D("zenithVSazimuth",
                 ("Zenith angle vs azimuth;#varphi [" + m_angleUnit + "];#vartheta [" + m_angleUnit + "]").c_str(),
                 m_azimuthGranularity,
                 m_azimuthLow * m_angleConversion,
                 m_azimuthHigh * m_angleConversion,
                 m_zenithGranularity,
                 m_zenithLow * m_angleConversion,
                 m_zenithHigh * m_angleConversion);

    // Initialise member variables
    m_eventNumber = 0;
    m_numberOfTracks = 0;

    // Initialise flux histograms
    m_azimuthFlux = static_cast<TH1D*>(m_azimuthHistogram->Clone());
    m_azimuthFlux->SetNameTitle("azimuthFlux",
                                ("Azimuthal flux distribution;#varphi [" + m_angleUnit + "];# tracks / sr").c_str());
    m_zenithFlux = static_cast<TH1D*>(m_zenithHistogram->Clone());
    m_zenithFlux->SetNameTitle("zenithFlux",
                               ("Zenith angle flux distribution;#vartheta [" + m_angleUnit + "];# tracks / sr").c_str());
    m_combinedFlux = static_cast<TH2D*>(m_combinedHistogram->Clone());
    m_combinedFlux->SetNameTitle(
        "zenithVSazimuthFlux",
        ("Zenith angle vs azimuth angle flux;#varphi [" + m_angleUnit + "];#vartheta [" + m_angleUnit + "]").c_str());
}

/**
 * @brief [Calculate zenith and azimuth, fill histograms]
 */
void AnalysisParticleFlux::calculateAngles(Track* track) {
    ROOT::Math::XYZVector track_direction = track->getDirection(m_trackIntercept);
    double phi = (track_direction.Phi() + ROOT::Math::Pi()) * m_angleConversion;
    double theta = track_direction.theta() * m_angleConversion;
    m_azimuthHistogram->Fill(phi);
    m_zenithHistogram->Fill(theta);
    m_combinedHistogram->Fill(phi, theta);
}
/**
 * @brief [Get solid angle for given bin (inputs in rad)]
 */
double AnalysisParticleFlux::solidAngle(double zenithLow, double zenithHigh, double azimuthLow, double azimuthHigh) {
    return (azimuthHigh - azimuthLow) * (std::cos(zenithLow) - std::cos(zenithHigh));
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
    LOG(INFO) << "Got " << m_numberOfTracks / solidAngle(m_zenithLow, m_zenithHigh, m_azimuthLow, m_zenithHigh)
              << " Tracks / sr in the ROI";

    // Filling flux histograms
    double zenithWidth = (m_zenithHigh - m_zenithLow) / m_zenithGranularity;
    double azimuthWidth = (m_azimuthHigh - m_azimuthLow) / m_azimuthGranularity;
    for(int i = 0; i < m_zenithGranularity; i++) {
        // Zenith loop
        for(int j = 0; j < m_azimuthGranularity; j++) {
            // Azimuth loop
            if(i == 0) {
                // Fill azimuth flux histogram
                int azimuthBinNumber = m_azimuthHistogram->FindBin((j + 0.5) * azimuthWidth * m_angleConversion);
                double azimuthTracks = m_azimuthHistogram->GetBinContent(azimuthBinNumber);
                double sA = solidAngle(m_zenithLow, m_zenithHigh, j * azimuthWidth, (j + 1) * azimuthWidth);
                m_azimuthFlux->SetBinContent(azimuthBinNumber, azimuthTracks / sA);
            }
            // Fill combined flux histogram
            int combinedBinNumber = m_combinedHistogram->FindBin((i + 0.5) * zenithWidth * m_angleConversion,
                                                                 (j + 0.5) * azimuthWidth * m_angleConversion);
            double combinedTracks = m_combinedHistogram->GetBinContent(combinedBinNumber);
            double sA = solidAngle(i * zenithWidth, (i + 1) * zenithWidth, j * azimuthWidth, (j + 1) * azimuthWidth);
            m_combinedFlux->SetBinContent(combinedBinNumber, combinedTracks / sA);
        }
        // Fill zenith flux histogram
        int zenithBinNumber = m_zenithHistogram->FindBin((i + 0.5) * zenithWidth * m_angleConversion);
        double zenithTracks = m_zenithHistogram->GetBinContent(zenithBinNumber);
        double sA = solidAngle(i * zenithWidth, (i + 1) * zenithWidth, m_azimuthLow, m_azimuthHigh);
        m_zenithFlux->SetBinContent(zenithBinNumber, zenithTracks / sA);
    }
}