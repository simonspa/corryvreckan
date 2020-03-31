/**
 * @file
 * @brief Implementation of module TrackingMultiplet
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TrackingMultiplet.h"

using namespace corryvreckan;

TrackingMultiplet::TrackingMultiplet(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    // Backwards compatibilty: also allow timing_cut to be used for time_cut_abs and spatial_cut for spatial_cut_abs
    m_config.setAlias("time_cut_abs", "timing_cut", true);
    m_config.setAlias("spatial_cut_abs", "spatial_cut", true);

    // timing cut, relative (x * time_resolution) or absolute:
    if(m_config.count({"time_cut_rel", "time_cut_abs"}) > 1) {
        throw InvalidCombinationError(
            m_config, {"time_cut_rel", "time_cut_abs"}, "Absolute and relative time cuts are mutually exclusive.");
    } else if(m_config.has("time_cut_abs")) {
        double time_cut_abs_ = m_config.get<double>("time_cut_abs");
        for(auto& detector : get_detectors()) {
            time_cuts_[detector] = time_cut_abs_;
        }
    } else {
        double time_cut_rel_ = m_config.get<double>("time_cut_rel", 3.0);
        for(auto& detector : get_detectors()) {
            time_cuts_[detector] = detector->getTimeResolution() * time_cut_rel_;
        }
    }

    // spatial cut, relative (x * spatial_resolution) or absolute:
    if(m_config.count({"spatial_cut_rel", "spatial_cut_abs"}) > 1) {
        throw InvalidCombinationError(
            m_config, {"spatial_cut_rel", "spatial_cut_abs"}, "Absolute and relative spatial cuts are mutually exclusive.");
    } else if(m_config.has("spatial_cut_abs")) {
        auto spatial_cut_abs_ = m_config.get<XYVector>("spatial_cut_abs");
        for(auto& detector : get_detectors()) {
            spatial_cuts_[detector] = spatial_cut_abs_;
        }
    } else {
        // default is 3.0 * spatial_resolution
        auto spatial_cut_rel_ = m_config.get<double>("spatial_cut_rel", 3.0);
        for(auto& detector : get_detectors()) {
            spatial_cuts_[detector] = detector->getSpatialResolution() * spatial_cut_rel_;
        }
    }

    m_upstream_detectors = m_config.getArray<std::string>("upstream_detectors");
    m_downstream_detectors = m_config.getArray<std::string>("downstream_detectors");
    if(m_upstream_detectors.size() < 2) {
        throw InvalidValueError(
            m_config, "upstream_detectors", "You have to provide at least two upstream detectors for tracking.");
    }
    if(m_downstream_detectors.size() < 2) {
        throw InvalidValueError(
            m_config, "downstream_detectors", "You have to provide at least two downstream detectors for tracking.");
    }

    min_hits_upstream_ = m_config.get<size_t>("min_hits_upstream", m_upstream_detectors.size());
    min_hits_downstream_ = m_config.get<size_t>("min_hits_downstream", m_downstream_detectors.size());
    if(min_hits_upstream_ > m_upstream_detectors.size()) {
        throw InvalidValueError(m_config,
                                "min_hits_upstream",
                                "Number of required upstream hits is larger than the amount of upstream detectors.");
    }
    if(min_hits_downstream_ > m_downstream_detectors.size()) {
        throw InvalidValueError(m_config,
                                "min_hits_downstream",
                                "Number of required downstream hits is larger than the amount of downstream detectors.");
    }

    scatterer_position_ = m_config.get<double>("scatterer_position");
    scatterer_matching_cut_ = m_config.get<double>("scatterer_matching_cut");
}

void TrackingMultiplet::initialise() {

    std::string title = "Upstream track multiplicity;upstream tracks;events";
    upstreamMultiplicity = new TH1F("upstreamMultiplicity", title.c_str(), 40, 0, 40);
    title = "Downstream track multiplicity;downstream tracks;events";
    downstreamMultiplicity = new TH1F("downstreamMultiplicity", title.c_str(), 40, 0, 40);
    title = "Multiplet multiplicity;multiplets;events";
    multipletMultiplicity = new TH1F("multipletMultiplicity", title.c_str(), 40, 0, 40);

    title = "Upstream track angle X;angle x [mrad];upstream tracks";
    upstreamAngleX = new TH1F("upstreamAngleX", title.c_str(), 250, -25., 25.);
    title = "Upstream track angle Y;angle y [mrad];upstream tracks";
    upstreamAngleY = new TH1F("upstreamAngleY", title.c_str(), 250, -25., 25.);
    title = "Downstream track angle X;angle x [mrad];downstream tracks";
    downstreamAngleX = new TH1F("downstreamAngleX", title.c_str(), 250, -25., 25.);
    title = "Downstream track angle Y;angle y [mrad];downstream tracks";
    downstreamAngleY = new TH1F("downstreamAngleY", title.c_str(), 250, -25., 25.);

    title = "Upstream track X at scatterer;position x [mm];upstream tracks";
    upstreamPositionAtScattererX = new TH1F("upstreamPositionAtScattererX", title.c_str(), 200, -10., 10.);
    title = "Upstream track Y at scatterer;position y [mm];upstream tracks";
    upstreamPositionAtScattererY = new TH1F("upstreamPositionAtScattererY", title.c_str(), 200, -10., 10.);
    title = "Downstream track X at scatterer;position x [mm];downstream tracks";
    downstreamPositionAtScattererX = new TH1F("downstreamPositionAtScattererX", title.c_str(), 200, -10., 10.);
    title = "Downstream track Y at scatterer;position y [mm];downstream tracks";
    downstreamPositionAtScattererY = new TH1F("downstreamPositionAtScattererY", title.c_str(), 200, -10., 10.);

    title = "Matching distance X at scatterer;distance x [mm];multiplet candidates";
    matchingDistanceAtScattererX = new TH1F("matchingDistanceAtScattererX", title.c_str(), 200, -10., 10.);
    title = "Matching distance Y at scatterer;distance y [mm];multiplet candidates";
    matchingDistanceAtScattererY = new TH1F("matchingDistanceAtScattererY", title.c_str(), 200, -10., 10.);

    title = "Multiplet offset X at scatterer;offset x [um];multiplets";
    multipletOffsetAtScattererX = new TH1F("multipletOffsetAtScattererX", title.c_str(), 200, -300., 300.);
    title = "Multiplet offset Y at scatterer;offset y [um];multiplets";
    multipletOffsetAtScattererY = new TH1F("multipletOffsetAtScattererY", title.c_str(), 200, -300., 300.);

    title = "Multiplet kink X at scatterer;kink x [mrad];multiplets";
    multipletKinkAtScattererX = new TH1F("multipletKinkAtScattererX", title.c_str(), 200, -20., 20.);
    title = "Multiplet kink Y at scatterer;kink y [mrad];multiplets";
    multipletKinkAtScattererY = new TH1F("multipletKinkAtScattererY", title.c_str(), 200, -20., 20.);

    // Loop over all planes
    for(auto& detector : get_detectors()) {
        auto detectorID = detector->name();

        // Do not created plots for auxiliary detectors:
        if(detector->isAuxiliary()) {
            continue;
        }

        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(detectorID.c_str());

        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();

        title = detectorID + " Residual X;x_{track}-x [mm];events";
        residualsX[detectorID] = new TH1F("residualsX", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y;y_{track}-y [mm];events";
        residualsY[detectorID] = new TH1F("residualsY", title.c_str(), 500, -0.1, 0.1);
    }

    LOG(DEBUG) << "Initialised all histograms.";
}

StatusCode TrackingMultiplet::run(std::shared_ptr<Clipboard>) {

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void TrackingMultiplet::finalise() {}
