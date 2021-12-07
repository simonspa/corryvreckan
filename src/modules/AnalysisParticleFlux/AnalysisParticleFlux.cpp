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
    config_.setDefault<double>("azimuth_low", Units::get<double>(0, "deg"));
    config_.setDefault<double>("azimuth_high", Units::get<double>(360, "deg"));
    config_.setDefault<int>("azimuth_granularity_", 36);
    // Zenith histogram
    config_.setDefault<double>("zenith_low", Units::get<double>(0, "deg"));
    config_.setDefault<double>("zenith_high", Units::get<double>(90, "deg"));
    config_.setDefault<int>("zenith_granularity", 9);
    // Track intercept
    config_.setDefault<double>("track_intercept", 0.0);
    // Angle unit
    config_.setDefault<bool>("output_plots_in_degrees", true);

    // Read configuration settings
    // Track intercept
    track_intercept_ = config_.get<double>("track_intercept");
    // Granularities
    azimuth_granularity_ = config_.get<int>("azimuth_granularity_");
    zenith_granularity_ = config_.get<int>("zenith_granularity");
    // Histogram bounds
    azimuth_low_ = config_.get<double>("azimuth_low");
    azimuth_high_ = config_.get<double>("azimuth_high");
    zenith_low_ = config_.get<double>("zenith_low");
    zenith_high_ = config_.get<double>("zenith_high");
    // Handle angle unit conversion
    output_plots_in_degrees_ = config_.get<bool>("output_plots_in_degrees");
    if(output_plots_in_degrees_) {
        angle_unit_ = "deg";
        angle_label_ = "#circ";
    } else {
        angle_unit_ = "rad";
        angle_label_ = "rad";
    };
}

void AnalysisParticleFlux::initialize() {

    // Initialise histograms
    azimuth_histogram_ = new TH1D("azimuth",
                                  ("Azimuthal distribution of tracks;#varphi [" + angle_label_ + "];# tracks").c_str(),
                                  azimuth_granularity_,
                                  Units::convert(azimuth_low_, angle_unit_),
                                  Units::convert(azimuth_high_, angle_unit_));
    zenith_histogram_ = new TH1D("zenith",
                                 ("Zenith angle distribution of tracks;#vartheta [" + angle_label_ + "];# tracks").c_str(),
                                 zenith_granularity_,
                                 Units::convert(zenith_low_, angle_unit_),
                                 Units::convert(zenith_high_, angle_unit_));
    combined_histogram_ =
        new TH2D("zenith_vs_azimuth",
                 ("Zenith angle vs azimuth;#varphi [" + angle_label_ + "];#vartheta [" + angle_label_ + "]").c_str(),
                 azimuth_granularity_,
                 Units::convert(azimuth_low_, angle_unit_),
                 Units::convert(azimuth_high_, angle_unit_),
                 zenith_granularity_,
                 Units::convert(zenith_low_, angle_unit_),
                 Units::convert(zenith_high_, angle_unit_));
    combined_histogram_->SetOption("COLZ");

    // Initialise flux histograms
    azimuth_flux_ = static_cast<TH1D*>(azimuth_histogram_->Clone());
    azimuth_flux_->SetNameTitle("azimuth_flux",
                                ("Azimuthal flux distribution;#varphi [" + angle_label_ + "];# tracks / sr").c_str());
    zenith_flux_ = static_cast<TH1D*>(zenith_histogram_->Clone());
    zenith_flux_->SetNameTitle("zenith_flux",
                               ("Zenith angle flux distribution;#vartheta [" + angle_label_ + "];# tracks / sr").c_str());
    combined_flux_ = static_cast<TH2D*>(combined_histogram_->Clone());
    combined_flux_->SetNameTitle(
        "zenith_vs_azimuth_flux",
        ("Zenith angle vs azimuth angle flux;#varphi [" + angle_label_ + "];#vartheta [" + angle_label_ + "]").c_str());
}

/**
 * @brief [Calculate zenith and azimuth, fill histograms]
 */
void AnalysisParticleFlux::calculateAngles(Track* track) {
    ROOT::Math::XYZVector track_direction = track->getDirection(track_intercept_);
    double phi = Units::convert((track_direction.Phi() + ROOT::Math::Pi()), angle_unit_);
    double theta = Units::convert(track_direction.theta(), angle_unit_);
    azimuth_histogram_->Fill(phi);
    zenith_histogram_->Fill(theta);
    combined_histogram_->Fill(phi, theta);
}
/**
 * @brief [Get solid angle for given bin (inputs in rad)]
 */
double AnalysisParticleFlux::solidAngle(double zenithLow, double zenithHigh, double azimuthLow, double azimuthHigh) const {
    return (azimuthHigh - azimuthLow) * (std::cos(zenithLow) - std::cos(zenithHigh));
}

StatusCode AnalysisParticleFlux::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all tracks, calculate all angles and fill histograms
    auto tracks = clipboard->getData<Track>();
    for(auto& track : tracks) {
        calculateAngles(track.get());
        number_of_tracks_++;
    }

    // Increment event counter
    event_number_++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisParticleFlux::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(INFO) << "Got " << number_of_tracks_ / solidAngle(zenith_low_, zenith_high_, azimuth_low_, zenith_high_)
              << " Tracks / sr in the ROI";

    // Filling flux histograms
    double zenith_width = (zenith_high_ - zenith_low_) / zenith_granularity_;
    double azimuth_width = (azimuth_high_ - azimuth_low_) / azimuth_granularity_;
    for(int i = 0; i < zenith_granularity_; i++) {
        // Zenith loop
        for(int j = 0; j < azimuth_granularity_; j++) {
            // Azimuth loop
            if(i == 0) {
                // Fill azimuth flux histogram
                int azimuth_bin_number = azimuth_histogram_->FindBin(Units::convert((j + 0.5) * azimuth_width, angle_unit_));
                double azimuthTracks = azimuth_histogram_->GetBinContent(azimuth_bin_number);
                double sA = solidAngle(zenith_low_, zenith_high_, j * azimuth_width, (j + 1) * azimuth_width);
                azimuth_flux_->SetBinContent(azimuth_bin_number, azimuthTracks / sA);
            }
            // Fill combined flux histogram
            int combined_bin_number = combined_histogram_->FindBin(Units::convert((j + 0.5) * azimuth_width, angle_unit_),
                                                                   Units::convert((i + 0.5) * zenith_width, angle_unit_));
            double combined_tracks = combined_histogram_->GetBinContent(combined_bin_number);
            double sA = solidAngle(i * zenith_width, (i + 1) * zenith_width, j * azimuth_width, (j + 1) * azimuth_width);
            combined_flux_->SetBinContent(combined_bin_number, combined_tracks / sA);
        }
        // Fill zenith flux histogram
        int zenith_bin_number = zenith_histogram_->FindBin(Units::convert((i + 0.5) * zenith_width, angle_unit_));
        double zenith_tracks = zenith_histogram_->GetBinContent(zenith_bin_number);
        double sA = solidAngle(i * zenith_width, (i + 1) * zenith_width, azimuth_low_, azimuth_high_);
        zenith_flux_->SetBinContent(zenith_bin_number, zenith_tracks / sA);
    }
}