/**
 * @file
 * @brief Implementation of module Tracking4D
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Tracking4D.h"
#include <TCanvas.h>
#include <TDirectory.h>
#include "objects/KDTree.hpp"

using namespace corryvreckan;
using namespace std;

Tracking4D::Tracking4D(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
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
    minHitsOnTrack = m_config.get<size_t>("min_hits_on_track", 6);
    excludeDUT = m_config.get<bool>("exclude_dut", true);
    requireDetectors = m_config.getArray<std::string>("require_detectors", {""});
    timestampFrom = m_config.get<std::string>("timestamp_from", {});
    trackModel = m_config.get<std::string>("track_model", "straightline");
    momentum = m_config.get<double>("momentum", Units::get<double>(5, "GeV"));
    volumeRadiationLength = m_config.get<double>("volume_radiation_length", Units::get<double>(304.2, "m"));
    useVolumeScatterer = m_config.get<bool>("volume_scattering", false);

    // print a warning if volumeScatterer are used as this causes fit failures
    // that are still not understood
    if(useVolumeScatterer) {
        LOG_ONCE(WARNING) << "Taking volume scattering effects into account is still WIP and causes the GBL to fail - these "
                             "tracks are rejected";
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
}

void Tracking4D::initialise() {

    // Set up histograms
    std::string title = "Track #chi^{2};#chi^{2};events";
    trackChi2 = new TH1F("trackChi2", title.c_str(), 150, 0, 150);
    title = "Track #chi^{2}/ndof;#chi^{2}/ndof;events";
    trackChi2ndof = new TH1F("trackChi2ndof", title.c_str(), 100, 0, 50);
    title = "Clusters per track;clusters;tracks";
    clustersPerTrack = new TH1F("clustersPerTrack", title.c_str(), 10, 0, 10);
    title = "Track multiplicity;tracks;events";
    tracksPerEvent = new TH1F("tracksPerEvent", title.c_str(), 100, 0, 100);
    title = "Track angle X;angle_{x} [rad];events";
    trackAngleX = new TH1F("trackAngleX", title.c_str(), 2000, -0.01, 0.01);
    title = "Track angle Y;angle_{y} [rad];events";
    trackAngleY = new TH1F("trackAngleY", title.c_str(), 2000, -0.01, 0.01);

    // Loop over all planes
    for(auto& detector : get_detectors()) {
        auto detectorID = detector->Name();

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

        title = detectorID + " kink X;kink [rad];events";
        kinkX[detectorID] = new TH1F("kinkX", title.c_str(), 500, -0.01, -0.01);
        title = detectorID + " kinkY ;kink [rad];events";
        kinkY[detectorID] = new TH1F("kinkY", title.c_str(), 500, -0.01, -0.01);

        // Do not create plots for detectors not participating in the tracking:
        if(excludeDUT && detector->isDUT()) {
            continue;
        }

        title = detectorID + " Residual X;x_{track}-x [mm];events";
        residualsX[detectorID] = new TH1F("residualsX", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual X, cluster column width 1;x_{track}-x [mm];events";
        residualsXwidth1[detectorID] = new TH1F("residualsXwidth1", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual X, cluster column width  2;x_{track}-x [mm];events";
        residualsXwidth2[detectorID] = new TH1F("residualsXwidth2", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual X, cluster column width  3;x_{track}-x [mm];events";
        residualsXwidth3[detectorID] = new TH1F("residualsXwidth3", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y;y_{track}-y [mm];events";
        residualsY[detectorID] = new TH1F("residualsY", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, cluster row width 1;y_{track}-y [mm];events";
        residualsYwidth1[detectorID] = new TH1F("residualsYwidth1", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, cluster row width 2;y_{track}-y [mm];events";
        residualsYwidth2[detectorID] = new TH1F("residualsYwidth2", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, cluster row width 3;y_{track}-y [mm];events";
        residualsYwidth3[detectorID] = new TH1F("residualsYwidth3", title.c_str(), 500, -0.1, 0.1);

        title = detectorID + " Pull X;x_{track}-x/resolution;events";
        pullX[detectorID] = new TH1F("pullX", title.c_str(), 500, -5, 5);

        title = detectorID + " Pull Y;y_{track}-y/resolution;events";
        pullY[detectorID] = new TH1F("pully", title.c_str(), 500, -5, 5);
    }
}

StatusCode Tracking4D::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<string, KDTree*> trees;
    vector<string> detectors;
    std::shared_ptr<ClusterVector> referenceClusters = nullptr;

    // Loop over all planes and get clusters
    bool firstDetector = true;
    std::string seedPlane;
    for(auto& detector : get_detectors()) {
        string detectorID = detector->Name();

        // Get the clusters
        auto tempClusters = clipboard->getData<Cluster>(detectorID);
        if(tempClusters == nullptr || tempClusters->size() == 0) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
        } else {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters->size() << " clusters from " << detectorID;
            if(firstDetector && !detector->isDUT()) {
                referenceClusters = tempClusters;
                time_cut_reference_ = time_cuts_[detector];
                seedPlane = detector->Name();
                LOG(DEBUG) << "Seed plane is " << seedPlane;
                firstDetector = false;
            }

            KDTree* clusterTree = new KDTree();
            clusterTree->buildTimeTree(*tempClusters);
            trees[detectorID] = clusterTree;
        }
        // the detector always needs to be listed as we would like to add the material budget information
        if(!detector->isAuxiliary()) {
            detectors.push_back(detectorID);
        }
    }

    // If there are no detectors then stop trying to track
    if(detectors.empty() || referenceClusters == nullptr) {
        // Clean up tree objects
        for(auto tree = trees.cbegin(); tree != trees.cend();) {
            delete tree->second;
            tree = trees.erase(tree);
        }

        return StatusCode::Success;
    }

    // Output track container
    auto tracks = std::make_shared<TrackVector>();

    // Loop over all clusters
    for(auto& cluster : (*referenceClusters)) {

        // Make a new track
        LOG(DEBUG) << "Looking at next seed cluster";

        auto track = Track::Factory(trackModel);
        // The track finding is based on a straight line. Therefore a refTrack to extrapolate to the next plane is used here
        auto refTrack = new StraightLineTrack();
        // Add the cluster to the track
        track->addCluster(cluster);
        track->setTimestamp(cluster->timestamp());
        track->setVolumeScatter(volumeRadiationLength);
        track->useVolumeScatterer(useVolumeScatterer);
        refTrack->addCluster(cluster);
        refTrack->setTimestamp(cluster->timestamp());

        track->setParticleMomentum(momentum);
        // Loop over each subsequent plane and look for a cluster within the timing cuts
        for(auto& detectorID : detectors) {
            // Get the detector
            auto det = get_detector(detectorID);

            // always add the material budget:
            track->addMaterial(detectorID, det->materialBudget(), det->displacement().z());
            LOG(TRACE) << "added material budget for " << detectorID << " at z = " << det->displacement().z();
            if(trees.count(detectorID) == 0) {
                LOG(TRACE) << "Skipping detector " << det->Name() << " as it has 0 clusters.";
                continue;
            }

            if(detectorID == seedPlane) {
                LOG(TRACE) << "Skipping seed plane " << det->Name();
                continue;
            }

            // Check if the DUT should be excluded and obey:
            if(excludeDUT && det->isDUT()) {
                LOG(DEBUG) << "Skipping DUT plane.";
                continue;
            }

            // Get all neighbours within the timing cut
            LOG(DEBUG) << "Searching for neighbouring cluster on device " << detectorID;
            LOG(DEBUG) << "- cluster time is " << Units::display(cluster->timestamp(), {"ns", "us", "s"});
            Cluster* closestCluster = nullptr;

            // Use spatial cut only as initial value (check if cluster is ellipse defined by cuts is done below):
            double closestClusterDistance =
                sqrt(spatial_cuts_[det].x() * spatial_cuts_[det].x() + spatial_cuts_[det].y() * spatial_cuts_[det].y());
            // For default configuration, comparing time cuts calculated from the time resolution of the current detector and
            // the first plane in Z,
            // and taking the maximal value as the cut in time for track-cluster association
            // If an absolute cut is to be used, then time_cut_reference_=time_cuts_[det]= time_cut_abs parameter
            double timeCut = std::max(time_cut_reference_, time_cuts_[det]);
            LOG(TRACE) << "Reference calcuated time cut = " << Units::display(time_cut_reference_, {"ns", "us", "s"})
                       << "; detector plane " << detectorID
                       << " calculated time cut = " << Units::display(time_cuts_[det], {"ns", "us", "s"});
            LOG(DEBUG) << "Using timing cut of " << Units::display(timeCut, {"ns", "us", "s"});
            auto neighbours = trees[detectorID]->getAllClustersInTimeWindow(cluster, timeCut);

            LOG(DEBUG) << "- found " << neighbours.size() << " neighbours";

            // Now look for the spatially closest cluster on the next plane
            double interceptX, interceptY;
            if(refTrack->nClusters() > 1) {
                refTrack->fit(); // fixme: this is not really a nice way to get the details

                PositionVector3D<Cartesian3D<double>> interceptPoint = det->getIntercept(refTrack);
                interceptX = interceptPoint.X();
                interceptY = interceptPoint.Y();
            } else {
                interceptX = cluster->global().x();
                interceptY = cluster->global().y();
            }

            // Loop over each neighbour in time
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

                double norm = (distanceX * distanceX) / (spatial_cuts_[det].x() * spatial_cuts_[det].x()) +
                              (distanceY * distanceY) / (spatial_cuts_[det].y() * spatial_cuts_[det].y());

                if(norm > 1) {
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
            LOG(DEBUG) << "- added cluster to track";
            track->addCluster(closestCluster);
            refTrack->addCluster(closestCluster);
        }

        // check if track has required detector(s):
        auto foundRequiredDetector = [this](Track* t) {
            for(auto& requireDet : requireDetectors) {
                if(!requireDet.empty() && !t->hasDetector(requireDet)) {
                    LOG(DEBUG) << "No cluster from required detector " << requireDet << " on the track.";
                    return false;
                }
            }
            return true;
        };
        if(!foundRequiredDetector(track)) {
            delete track;
            continue;
        }

        // Now should have a track with one cluster from each plane
        if(track->nClusters() < minHitsOnTrack) {
            LOG(DEBUG) << "Not enough clusters on the track, found " << track->nClusters() << " but " << minHitsOnTrack
                       << " required.";
            delete track;
            continue;
        }
        delete refTrack;
        // Fit the track and save it
        track->fit();
        if(track->isFitted()) {
            tracks->push_back(track);
        } else {
            LOG_N(WARNING, 100) << "Rejected a track due to failure in fitting";
            delete track;
            continue;
        }
        // Fill histograms
        trackChi2->Fill(track->chi2());
        clustersPerTrack->Fill(static_cast<double>(track->nClusters()));
        trackChi2ndof->Fill(track->chi2ndof());
        if(!(trackModel == "gbl")) {
            trackAngleX->Fill(atan(track->direction(track->clusters().front()->detectorID()).X()));
            trackAngleY->Fill(atan(track->direction(track->clusters().front()->detectorID()).Y()));
        }
        // Make residuals
        auto trackClusters = track->clusters();
        for(auto& trackCluster : trackClusters) {
            string detectorID = trackCluster->detectorID();
            residualsX[detectorID]->Fill(track->residual(detectorID).X());
            pullX[detectorID]->Fill(track->residual(detectorID).X() / track->clusters().front()->errorX());
            pullY[detectorID]->Fill(track->residual(detectorID).Y() / track->clusters().front()->errorY());
            if(trackCluster->columnWidth() == 1)
                residualsXwidth1[detectorID]->Fill(track->residual(detectorID).X());
            if(trackCluster->columnWidth() == 2)
                residualsXwidth2[detectorID]->Fill(track->residual(detectorID).X());
            if(trackCluster->columnWidth() == 3)
                residualsXwidth3[detectorID]->Fill(track->residual(detectorID).X());
            residualsY[detectorID]->Fill(track->residual(detectorID).Y());
            if(trackCluster->rowWidth() == 1)
                residualsYwidth1[detectorID]->Fill(track->residual(detectorID).Y());
            if(trackCluster->rowWidth() == 2)
                residualsYwidth2[detectorID]->Fill(track->residual(detectorID).Y());
            if(trackCluster->rowWidth() == 3)
                residualsYwidth3[detectorID]->Fill(track->residual(detectorID).Y());
        }

        for(auto& det : detectors) {
            if(!kinkX.count(det)) {
                LOG(WARNING) << "Skipping writing kinks due to missing init of histograms for  " << det;
                continue;
            }

            XYPoint kink = track->kink(det);
            kinkX.at(det)->Fill(kink.x());
            kinkY.at(det)->Fill(kink.y());
        }

        if(timestampFrom.empty()) {
            // Improve the track timestamp by taking the average of all planes
            double avg_track_time = 0;
            for(auto& trackCluster : trackClusters) {
                avg_track_time += static_cast<double>(Units::convert(trackCluster->timestamp(), "ns"));
                avg_track_time -= static_cast<double>(Units::convert(trackCluster->global().z(), "mm") / (299.792458));
            }
            track->setTimestamp(avg_track_time / static_cast<double>(track->nClusters()));
            LOG(DEBUG) << "Using average cluster timestamp of "
                       << Units::display(avg_track_time / static_cast<double>(track->nClusters()), "us")
                       << " as track timestamp.";
        } else if(track->hasDetector(timestampFrom)) {
            // use timestamp of required detector:
            double track_timestamp = track->getClusterFromDetector(timestampFrom)->timestamp();
            LOG(DEBUG) << "Found cluster for detector " << timestampFrom << ", adding timestamp "
                       << Units::display(track_timestamp, "us") << " to track.";
            track->setTimestamp(track_timestamp);
        } else {
            LOG(ERROR) << "Cannot assign timestamp to track. Use average cluster timestamp for track or set detector to set "
                          "track timestamp. Please update the configuration file.";
            return StatusCode::Failure;
        }
    }

    tracksPerEvent->Fill(static_cast<double>(tracks->size()));

    // Save the tracks on the clipboard
    if(tracks->size() > 0) {
        clipboard->putData(tracks);
    }

    // Clean up tree objects
    for(auto tree = trees.cbegin(); tree != trees.cend();) {
        delete tree->second;
        tree = trees.erase(tree);
    }

    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}
