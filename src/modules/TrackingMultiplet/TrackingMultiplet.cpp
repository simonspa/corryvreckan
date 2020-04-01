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

    std::string title = "Multiplet multiplicity;multiplets;events";
    multipletMultiplicity = new TH1F("multipletMultiplicity", title.c_str(), 40, 0, 40);

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

    for(auto stream : {upstream, downstream}) {
        std::string stream_name = stream == upstream ? "upstream" : "downstream";
        std::string stream_name_caps = stream == upstream ? "Upstream" : "Downstream";

        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(stream_name.c_str());

        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();

        title = "";
        std::string hist_name = "";

        // FIXME: Add cluster per track
        title = stream_name_caps + " track multiplicity;" + stream_name + " tracks;events";
        hist_name = stream_name + "Multiplicity";
        streamMultiplicity[stream] = new TH1F(hist_name.c_str(), title.c_str(), 40, 0, 40);

        title = stream_name_caps + " track angle X;angle x [mrad];" + stream_name + " tracks";
        hist_name = stream_name + "AngleX";
        streamAngleX[stream] = new TH1F(hist_name.c_str(), title.c_str(), 250, -25., 25.);
        title = stream_name_caps + " track angle Y;angle y [mrad];" + stream_name + " tracks";
        hist_name = stream_name + "AngleY";
        streamAngleY[stream] = new TH1F(hist_name.c_str(), title.c_str(), 250, -25., 25.);

        title = stream_name_caps + " track X at scatterer;position x [mm];" + stream_name + " tracks";
        hist_name = stream_name + "PositionAtScattererX";
        streamPositionAtScattererX[stream] = new TH1F(hist_name.c_str(), title.c_str(), 200, -10., 10.);
        title = stream_name_caps + " track Y at scatterer;position y [mm];" + stream_name + " tracks";
        hist_name = stream_name_caps + "PositionAtScattererY";
        streamPositionAtScattererY[stream] = new TH1F(hist_name.c_str(), title.c_str(), 200, -10., 10.);
    }

    // Loop over all up- and downstream planes
    std::vector<std::string> all_detectors;
    all_detectors.insert(all_detectors.end(), m_upstream_detectors.begin(), m_upstream_detectors.end());
    all_detectors.insert(all_detectors.end(), m_downstream_detectors.begin(), m_downstream_detectors.end());
    for(auto& detectorID : all_detectors) {

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
}

TrackVector TrackingMultiplet::findMultipletArm(streams stream, std::map<std::string, KDTree*>& cluster_tree) {

    // Define upstream/downstream dependent variables
    std::vector<std::string> stream_detectors = stream == upstream ? m_upstream_detectors : m_downstream_detectors;
    size_t min_hits = stream == upstream ? min_hits_upstream_ : min_hits_downstream_;
    std::string stream_name = stream == upstream ? "upstream" : "downstream";

    std::string reference_first = "";
    std::string reference_last = "";

    // Choose reference detectors (first and last hit detector in the list)
    LOG(DEBUG) << "Start finding " + stream_name + " tracks";
    for(auto& detector_ID : stream_detectors) {
        if(cluster_tree.count(detector_ID) == 0) {
            LOG(DEBUG) << "No clusters to be found in " << detector_ID;
            continue;
        }

        if(reference_first == "") {
            reference_first = detector_ID;
        }
        reference_last = detector_ID;
    }

    TrackVector tracks;

    if(reference_first == "" || reference_last == "" || reference_first == reference_last) {
        LOG(DEBUG) << "No " + stream_name + " tracks found in this event";
        return tracks;
    }

    LOG(DEBUG) << "Reference detectors for " << stream_name << " track: " << reference_first << " & " << reference_last;

    // Start track finding
    for(auto& clusterFirst : cluster_tree[reference_first]->getAllClusters()) {
        for(auto& clusterLast : cluster_tree[reference_last]->getAllClusters()) {
            auto trackCandidate = new StraightLineTrack();
            trackCandidate->addCluster(clusterFirst);
            trackCandidate->addCluster(clusterLast);
            trackCandidate->setTimestamp((clusterFirst->timestamp() + clusterLast->timestamp()) / 2.);

            for(auto& detectorID : stream_detectors) {
                if(detectorID == reference_first || detectorID == reference_last) {
                    continue;
                }

                if(cluster_tree.count(detectorID) == 0) {
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
                auto neighbours = cluster_tree[detectorID]->getAllClustersInTimeWindow(clusterFirst, time_cuts_[detector]);

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
                    LOG(DEBUG) << "No cluster within spatial cut";
                    continue;
                }

                // Add the cluster to the track
                trackCandidate->addCluster(closestCluster);
                LOG(DEBUG) << "Added good cluster to track candidate";
            }

            if(trackCandidate->nClusters() < min_hits) {
                LOG(DEBUG) << "Not enough clusters on the track, found " << trackCandidate->nClusters() << " but "
                           << min_hits << " required";
                delete trackCandidate;
                continue;
            }

            LOG(DEBUG) << "Found good track. Keeping this one." trackCandidate->fit();
            tracks.push_back(trackCandidate);
        }
    }
    return tracks;
}

void TrackingMultiplet::fillMultipletArmHistograms(streams stream, TrackVector tracks) {

    std::string stream_name = stream == upstream ? "upstream" : "downstream";

    streamMultiplicity[stream]->Fill(static_cast<double>(tracks.size()));

    if(tracks.size() > 0) {
        LOG(DEBUG) << "Filling plots for " << stream_name << " tracks";

        for(auto& track : tracks) {
            streamAngleX[stream]->Fill(
                static_cast<double>(Units::convert(track->direction("").X() / track->direction("").Z(), "mrad")));
            streamAngleY[stream]->Fill(
                static_cast<double>(Units::convert(track->direction("").Y() / track->direction("").Z(), "mrad")));

            streamPositionAtScattererX[stream]->Fill(track->intercept(scatterer_position_).X());
            streamPositionAtScattererY[stream]->Fill(track->intercept(scatterer_position_).Y());

            auto trackClusters = track->clusters();
            for(auto& trackCluster : trackClusters) {
                std::string detectorID = trackCluster->detectorID();
                residualsX[detectorID]->Fill(track->residual(detectorID).X());
                residualsY[detectorID]->Fill(track->residual(detectorID).Y());
            }
        }
    }
}

StatusCode TrackingMultiplet::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "Start of event";

    std::map<std::string, KDTree*> upstream_trees;
    std::map<std::string, KDTree*> downstream_trees;

    // Store upstream data in KDTrees
    for(auto& upstream_detector_ID : m_upstream_detectors) {
        LOG(DEBUG) << "Store data for upstream detector " << upstream_detector_ID;

        auto clusters = clipboard->getData<Cluster>(upstream_detector_ID);
        if(clusters == nullptr || clusters->size() == 0) {
            continue;
        }
        LOG(DEBUG) << "Cluster count: " << clusters->size();

        // Store all clusters in KDTrees
        KDTree* clusterTree = new KDTree();
        clusterTree->buildTimeTree(*clusters);
        upstream_trees[upstream_detector_ID] = clusterTree;
    }

    // Store downstream data in KDTrees
    for(auto& downstream_detector_ID : m_downstream_detectors) {
        LOG(DEBUG) << "Store data for downstream detector " << downstream_detector_ID;

        auto clusters = clipboard->getData<Cluster>(downstream_detector_ID);
        if(clusters == nullptr || clusters->size() == 0) {
            continue;
        }
        LOG(DEBUG) << "Nr. of clusters: " << clusters->size();

        // Store all clusters in KDTrees
        KDTree* clusterTree = new KDTree();
        clusterTree->buildTimeTree(*clusters);
        downstream_trees[downstream_detector_ID] = clusterTree;
    }

    TrackVector upstream_tracks = findMultipletArm(upstream, upstream_trees);
    TrackVector downstream_tracks = findMultipletArm(downstream, downstream_trees);

    LOG(DEBUG) << "Found " << upstream_tracks.size() << " upstream tracks";
    LOG(DEBUG) << "Found " << downstream_tracks.size() << " downstream tracks";

    // Fill histograms for up- and downstream tracks
    fillMultipletArmHistograms(upstream, upstream_tracks);
    fillMultipletArmHistograms(downstream, downstream_tracks);

    // Multiplet merging
    // FIXME: Check for matching criterion in time

    MultipletVector multiplets;
    for(auto& uptrack : upstream_tracks) {
        Multiplet* multiplet = nullptr;
        double closestMatchingDistance = scatterer_matching_cut_;

        for(auto& downtrack : downstream_tracks) {
            auto multipletCandidate = new Multiplet(uptrack, downtrack);
            multipletCandidate->setScattererPosition(scatterer_position_);
            multipletCandidate->fit();

            double distanceX = multipletCandidate->getOffsetAtScatterer().X();
            double distanceY = multipletCandidate->getOffsetAtScatterer().Y();
            double distance = sqrt(distanceX * distanceX + distanceY * distanceY);

            LOG(DEBUG) << "Multiplet candidate distance (x, y, abs): " << Units::display(distanceX, {"um"}) << "  "
                       << Units::display(distanceY, {"um"}) << "  " << Units::display(distance, {"um"});

            matchingDistanceAtScattererX->Fill(distanceX);
            matchingDistanceAtScattererY->Fill(distanceY);

            if(distance > scatterer_matching_cut_) {
                LOG(DEBUG) << "Multiplet candidate discarded due to high distance at scatterer";
                continue;
            }

            if(distance > closestMatchingDistance) {
                LOG(DEBUG) << "Multiplet candidate discarded - there's a closer match";
                continue;
            }

            LOG(DEBUG) << "Closest multiplet match so far. Proceed as candidate.";
            closestMatchingDistance = distance;
            multiplet = multipletCandidate;
        }

        if(multiplet == nullptr) {
            LOG(DEBUG) << "No matching downstream track found";
            continue;
        }

        LOG(DEBUG) << "Multiplet found";
        multiplets.push_back(multiplet);

        double distanceX = multiplet->getOffsetAtScatterer().X();
        double distanceY = multiplet->getOffsetAtScatterer().Y();

        double kinkX = multiplet->getKinkAtScatterer().X();
        double kinkY = multiplet->getKinkAtScatterer().Y();

        multipletOffsetAtScattererX->Fill(static_cast<double>(Units::convert(distanceX, "um")));
        multipletOffsetAtScattererY->Fill(static_cast<double>(Units::convert(distanceY, "um")));

        multipletKinkAtScattererX->Fill(static_cast<double>(Units::convert(kinkX, "mrad")));
        multipletKinkAtScattererY->Fill(static_cast<double>(Units::convert(kinkY, "mrad")));
    }

    LOG(DEBUG) << "Found " << multiplets.size() << " multiplets";
    multipletMultiplicity->Fill(static_cast<double>(multiplets.size()));

    // Clean up tree objects
    LOG(DEBUG) << "Cleaning up";
    for(auto tree = upstream_trees.cbegin(); tree != upstream_trees.cend();) {
        delete tree->second;
        tree = upstream_trees.erase(tree);
    }
    for(auto tree = downstream_trees.cbegin(); tree != downstream_trees.cend();) {
        delete tree->second;
        tree = downstream_trees.erase(tree);
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void TrackingMultiplet::finalise() {}
