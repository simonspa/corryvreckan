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
    config_.setDefault<int>("azimuth_granularity", 36);
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
    azimuth_granularity_ = config_.get<int>("azimuth_granularity");
    zenith_granularity_ = config_.get<int>("zenith_granularity");
    // Histogram bounds
    azimuth_low_ = config_.get<double>("azimuth_low");
    azimuth_high_ = config_.get<double>("azimuth_high");
    zenith_low_ = config_.get<double>("zenith_low");
    zenith_high_ = config_.get<double>("zenith_high");
    // Handle angle unit conversion
    output_plots_in_degrees_ = config_.get<bool>("output_plots_in_degrees");
}

void AnalysisParticleFlux::initialize() {

    std::string label = (output_plots_in_degrees_ ? "#circ" : "rad");
    angle_unit_ = (output_plots_in_degrees_ ? "deg" : "rad");

    // Initialise histograms
    azimuth_histogram_ = new TH1D("azimuth",
                                  ("Azimuthal distribution of tracks;#varphi [" + label + "];# tracks").c_str(),
                                  azimuth_granularity_,
                                  static_cast<double>(Units::convert(azimuth_low_, angle_unit_)),
                                  static_cast<double>(Units::convert(azimuth_high_, angle_unit_)));
    zenith_histogram_ = new TH1D("zenith",
                                 ("Zenith angle distribution of tracks;#vartheta [" + label + "];# tracks").c_str(),
                                 zenith_granularity_,
                                 static_cast<double>(Units::convert(zenith_low_, angle_unit_)),
                                 static_cast<double>(Units::convert(zenith_high_, angle_unit_)));
    combined_histogram_ = new TH2D("zenith_vs_azimuth",
                                   ("Zenith angle vs azimuth;#varphi [" + label + "];#vartheta [" + label + "]").c_str(),
                                   azimuth_granularity_,
                                   static_cast<double>(Units::convert(azimuth_low_, angle_unit_)),
                                   static_cast<double>(Units::convert(azimuth_high_, angle_unit_)),
                                   zenith_granularity_,
                                   static_cast<double>(Units::convert(zenith_low_, angle_unit_)),
                                   static_cast<double>(Units::convert(zenith_high_, angle_unit_)));
    combined_histogram_->SetOption("COLZ");

    // Initialise flux histograms
    azimuth_flux_ = static_cast<TH1D*>(azimuth_histogram_->Clone());
    azimuth_flux_->SetNameTitle("azimuth_flux",
                                ("Azimuthal flux distribution;#varphi [" + label + "];# tracks / sr").c_str());
    zenith_flux_ = static_cast<TH1D*>(zenith_histogram_->Clone());
    zenith_flux_->SetNameTitle("zenith_flux",
                               ("Zenith angle flux distribution;#vartheta [" + label + "];# tracks / sr").c_str());
    combined_flux_ = static_cast<TH2D*>(combined_histogram_->Clone());
    combined_flux_->SetNameTitle(
        "zenith_vs_azimuth_flux",
        ("Zenith angle vs azimuth angle flux;#varphi [" + label + "];#vartheta [" + label + "]").c_str());

    auto detectors = get_detectors();
    if(detectors.size() > 2) {
        // We can only calculate efficiencies if there are more than 2 detector layers
        for(auto& detector : detectors) {
            auto detectorID = detector->getName();
            // New directory for every detector
            TDirectory* directory = getROOTDirectory();
            TDirectory* local_directory = directory->mkdir(detectorID.c_str());
            local_directory->cd();
            // Initiliazing histos
            azimuth_efficiency_[detectorID] = static_cast<TH1D*>(azimuth_histogram_->Clone());
            azimuth_efficiency_[detectorID]->SetNameTitle(
                "azimuth_efficiency",
                ("Azimuthal efficiency in " + detectorID + ";#varphi [" + label + "];Efficiency").c_str());

            zenith_efficiency_[detectorID] = static_cast<TH1D*>(zenith_histogram_->Clone());
            zenith_efficiency_[detectorID]->SetNameTitle(
                "zenith_efficiency",
                ("Zenith efficiency in " + detectorID + ";#vartheta [" + label + "];Efficiency").c_str());

            combined_efficiency_[detectorID] = static_cast<TH2D*>(combined_histogram_->Clone());
            combined_efficiency_[detectorID]->SetNameTitle("zenith_vs_azimuth_efficiency",
                                                           ("Zenith angle vs azimuth efficiency in " + detectorID +
                                                            ";#varphi [" + label + "];#vartheta [" + label + "]")
                                                               .c_str());
        }
    }
}

/**
 * @brief [Calculate zenith and azimuth, fill histograms]
 */
void AnalysisParticleFlux::calculateAngles(Track* track) {
    // Calculate angles
    ROOT::Math::XYZVector track_direction = track->getDirection(track_intercept_);
    double phi = static_cast<double>(Units::convert((track_direction.Phi() + ROOT::Math::Pi()), angle_unit_));
    double theta = static_cast<double>(Units::convert(track_direction.theta(), angle_unit_));
    // Fill track histos
    azimuth_histogram_->Fill(phi);
    zenith_histogram_->Fill(theta);
    combined_histogram_->Fill(phi, theta);

    auto detectors = get_detectors();
    if(detectors.size() > 2) {
        for(auto& detector : detectors) {
            auto detectorID = detector->getName();
            if(track->hasDetector(detectorID)) {
                // Fill tracks in efficiency histos
                azimuth_efficiency_[detectorID]->Fill(phi);
                zenith_efficiency_[detectorID]->Fill(theta);
                combined_efficiency_[detectorID]->Fill(phi, theta);
            }
        };
    };
}
/**
 * @brief [Get solid angle for given bin (inputs in rad, output in sr)]
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

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisParticleFlux::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(INFO) << "Got " << number_of_tracks_ / solidAngle(zenith_low_, zenith_high_, azimuth_low_, zenith_high_)
              << " Tracks / sr in the ROI";

    // Filling flux histograms
    double zenith_width = (zenith_high_ - zenith_low_) / zenith_granularity_;
    double azimuth_width = (azimuth_high_ - azimuth_low_) / azimuth_granularity_;

    auto detectors = get_detectors();

    for(int i = 0; i < zenith_granularity_; i++) {
        // Zenith loop
        for(int j = 0; j < azimuth_granularity_; j++) {
            // Azimuth loop
            // Fill combined flux histogram
            int combined_bin_number =
                combined_histogram_->FindBin(static_cast<double>(Units::convert((j + 0.5) * azimuth_width, angle_unit_)),
                                             static_cast<double>(Units::convert((i + 0.5) * zenith_width, angle_unit_)));
            double combined_tracks = combined_histogram_->GetBinContent(combined_bin_number);
            double sA = solidAngle(i * zenith_width, (i + 1) * zenith_width, j * azimuth_width, (j + 1) * azimuth_width);
            combined_flux_->SetBinContent(combined_bin_number, combined_tracks / sA);
            // Fill combined efficiency
            if(detectors.size() > 2) {
                for(auto& detector : detectors) {
                    auto detectorID = detector->getName();
                    double detector_tracks = combined_efficiency_[detectorID]->GetBinContent(combined_bin_number);
                    if(detector_tracks < 1. || combined_tracks < 1.)
                        combined_efficiency_[detectorID]->SetBinContent(combined_bin_number, 0);
                    else
                        combined_efficiency_[detectorID]->SetBinContent(combined_bin_number,
                                                                        detector_tracks / combined_tracks);
                }
            };
        };
    }

    for(int i = 0; i < zenith_granularity_; i++) {
        // Fill zenith flux histogram
        int zenith_bin_number =
            zenith_histogram_->FindBin(static_cast<double>(Units::convert((i + 0.5) * zenith_width, angle_unit_)));
        double zenith_tracks = zenith_histogram_->GetBinContent(zenith_bin_number);
        double sA = solidAngle(i * zenith_width, (i + 1) * zenith_width, azimuth_low_, azimuth_high_);
        zenith_flux_->SetBinContent(zenith_bin_number, zenith_tracks / sA);
        // Fill zenith efficiency
        if(detectors.size() > 2) {
            for(auto& detector : detectors) {
                auto detectorID = detector->getName();
                double detector_tracks = zenith_efficiency_[detectorID]->GetBinContent(zenith_bin_number);
                if(detector_tracks < 1. || zenith_tracks < 1.)
                    zenith_efficiency_[detectorID]->SetBinContent(zenith_bin_number, 0);
                else
                    zenith_efficiency_[detectorID]->SetBinContent(zenith_bin_number, detector_tracks / zenith_tracks);
            }
        };
    }

    for(int j = 0; j < azimuth_granularity_; j++) {
        // Fill azimuth flux histogram
        int azimuth_bin_number =
            azimuth_histogram_->FindBin(static_cast<double>(Units::convert((j + 0.5) * azimuth_width, angle_unit_)));
        double azimuth_tracks = azimuth_histogram_->GetBinContent(azimuth_bin_number);
        double sA = solidAngle(zenith_low_, zenith_high_, j * azimuth_width, (j + 1) * azimuth_width);
        azimuth_flux_->SetBinContent(azimuth_bin_number, azimuth_tracks / sA);
        // Fill azimuth efficiency
        if(detectors.size() > 2) {
            for(auto& detector : detectors) {
                auto detectorID = detector->getName();
                double detector_tracks = azimuth_efficiency_[detectorID]->GetBinContent(azimuth_bin_number);
                if(detector_tracks < 1. || azimuth_tracks < 1.)
                    azimuth_efficiency_[detectorID]->SetBinContent(azimuth_bin_number, 0);
                else
                    azimuth_efficiency_[detectorID]->SetBinContent(azimuth_bin_number, detector_tracks / azimuth_tracks);
            }
        };
    }
}