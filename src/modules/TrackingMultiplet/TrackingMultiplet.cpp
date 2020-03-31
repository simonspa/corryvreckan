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

StatusCode TrackingMultiplet::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "Start of event";

    std::string upstream_reference_first = "";
    std::string upstream_reference_last = "";

    std::shared_ptr<ClusterVector> upstream_reference_clusters_first = nullptr;
    std::shared_ptr<ClusterVector> upstream_reference_clusters_last = nullptr;

    for(auto& upstream_detector_ID : m_upstream_detectors) {
        LOG(DEBUG) << "Upstream detector " << upstream_detector_ID;

        auto clusters = clipboard->getData<Cluster>(upstream_detector_ID);
        if(clusters == nullptr || clusters->size() == 0) {
            continue;
        }
        LOG(DEBUG) << "Clusters: " << clusters->size();
        if(upstream_reference_first == "") {
            upstream_reference_first = upstream_detector_ID;
            upstream_reference_clusters_first = clusters;
        }
        upstream_reference_last = upstream_detector_ID;
        upstream_reference_clusters_last = clusters;

        // Store them all in KDTrees
        KDTree* clusterTree = new KDTree();
        clusterTree->buildTimeTree(*clusters);
        upstream_trees[upstream_detector_ID] = clusterTree;
    }

    if(upstream_reference_clusters_first == nullptr || upstream_reference_clusters_last == nullptr ||
       upstream_reference_clusters_first == upstream_reference_clusters_last) {
        LOG(DEBUG) << "No upstream tracks to be found in this event";
        return StatusCode::Success;
    }

    LOG(DEBUG) << "Number of upstream reference clusters (first hit detector, in " << upstream_reference_first
               << "): " << upstream_reference_clusters_first->size();
    LOG(DEBUG) << "Number of upstream reference clusters (last hit detector, in " << upstream_reference_last
               << "): " << upstream_reference_clusters_last->size();

    // Start track finding

    for(auto& clusterFirst : (*upstream_reference_clusters_first)) {
        for(auto& clusterLast : (*upstream_reference_clusters_last)) {
            auto trackCandidate = new StraightLineTrack();
            trackCandidate->addCluster(clusterFirst);
            trackCandidate->addCluster(clusterLast);
            trackCandidate->setTimestamp((clusterFirst->timestamp() + clusterLast->timestamp()) / 2.);

            for(auto& detectorID : m_upstream_detectors) {
                if(detectorID == upstream_reference_first || detectorID == upstream_reference_last) {
                    LOG(DEBUG) << "Don't have to count this detector, since it's a reference detector";
                    continue;
                }

                if(upstream_trees.count(detectorID) == 0) {
                    LOG(TRACE) << "Skipping detector " << detectorID << " as it has 0 clusters.";
                    continue;
                }

                auto detector = get_detector(detectorID);

                // Now let's see if there's a cluster matching in time and space.

                Cluster* closestCluster = nullptr;

                // Use spatial cut only as initial value (check if cluster is ellipse defined by cuts is done below):
                double closestClusterDistance = sqrt(spatial_cuts_[detector].x() * spatial_cuts_[detector].x() +
                                                     spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

                LOG(DEBUG) << "Using timing cut of " << Units::display(time_cuts_[detector], {"ns", "us", "s"});
                auto neighbours = upstream_trees[detectorID]->getAllClustersInTimeWindow(clusterFirst, time_cuts_[detector]);

                LOG(DEBUG) << "- found " << neighbours.size() << " neighbours";

                // Now look for the spatially closest cluster on the next plane
                trackCandidate->fit();

                double interceptX, interceptY;
                PositionVector3D<Cartesian3D<double>> interceptPoint = detector->getIntercept(trackCandidate);
                interceptX = interceptPoint.X();
                interceptY = interceptPoint.Y();

                for(size_t ne = 0; ne < neighbours.size(); ne++) {
                    Cluster* newCluster = neighbours[ne];

                    // Calculate the distance to the previous plane's cluster/intercept
                    double distanceX = interceptX - newCluster->global().x();
                    double distanceY = interceptY - newCluster->global().y();
                    double distance = sqrt(distanceX * distanceX + distanceY * distanceY);

                    // Check if newCluster lies within ellipse defined by spatial cuts around intercept,
                    // following this example:
                    // https://www.geeksforgeeks.org/check-if-a-point-is-inside-outside-or-on-the-ellipse/
                    //
                    // ellipse defined by: x^2/a^2 + y^2/b^2 = 1: on ellipse,
                    //                                       > 1: outside,
                    //                                       < 1: inside
                    // Continue if outside of ellipse:

                    double norm = (distanceX * distanceX) / (spatial_cuts_[detector].x() * spatial_cuts_[detector].x()) +
                                  (distanceY * distanceY) / (spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

                    if(norm > 1) {
                        LOG(DEBUG) << "Cluster outside the cuts. Normalized distance: " << norm;
                        continue;
                    }

                    // If this is the closest keep it
                    if(distance < closestClusterDistance) {
                        closestClusterDistance = distance;
                        closestCluster = newCluster;
                    }
                }

                if(closestCluster == nullptr) {
                    LOG(DEBUG) << "No cluster within spatial cut.";
                    continue;
                }

                // Add the cluster to the track
                trackCandidate->addCluster(closestCluster);
                LOG(DEBUG) << "Added cluster to track candidate";
            }

            if(trackCandidate->nClusters() < min_hits_upstream_) {
                LOG(DEBUG) << "Not enough clusters on the track, found " << trackCandidate->nClusters() << " but "
                           << min_hits_upstream_ << " required.";
                delete trackCandidate;
                continue;
            }

            trackCandidate->fit();
            m_upstreamTracks.push_back(trackCandidate);
        }
    }

    // Fill some plots
    upstreamMultiplicity->Fill(static_cast<double>(m_upstreamTracks.size()));

    if(m_upstreamTracks.size() > 0) {
        LOG(DEBUG) << "Filling plots for upstream tracks.";

        for(auto& track : m_upstreamTracks) {
            upstreamAngleX->Fill(
                static_cast<double>(Units::convert(track->direction("").X() / track->direction("").Z(), "mrad")));
            upstreamAngleY->Fill(
                static_cast<double>(Units::convert(track->direction("").Y() / track->direction("").Z(), "mrad")));

            upstreamPositionAtScattererX->Fill(track->intercept(scatterer_position_).X());
            upstreamPositionAtScattererY->Fill(track->intercept(scatterer_position_).Y());

            for(std::map<std::string, TH1F*>::iterator it = residualsX.begin(); it != residualsX.end(); ++it) {
                LOG(DEBUG) << it->first;
            }
            auto trackClusters = track->clusters();
            for(auto& trackCluster : trackClusters) {
                std::string detectorID = trackCluster->detectorID();
                residualsX[detectorID]->Fill(track->residual(detectorID).X());
                residualsY[detectorID]->Fill(track->residual(detectorID).Y());
            }
        }
    }

    // Clean up tree objects
    LOG(DEBUG) << "Cleaning up.";
    for(auto tree = upstream_trees.cbegin(); tree != upstream_trees.cend();) {
        delete tree->second;
        tree = upstream_trees.erase(tree);
    }
    m_upstreamTracks.clear();

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void TrackingMultiplet::finalise() {}
