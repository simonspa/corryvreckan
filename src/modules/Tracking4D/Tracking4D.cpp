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

#include "tools/cuts.h"
#include "tools/kdtree.h"

using namespace corryvreckan;
using namespace std;

Tracking4D::Tracking4D(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    // Backwards compatibilty: also allow timing_cut to be used for time_cut_abs and spatial_cut for spatial_cut_abs
    m_config.setAlias("time_cut_abs", "timing_cut", true);
    m_config.setAlias("spatial_cut_abs", "spatial_cut", true);

    // timing cut, relative (x * time_resolution) or absolute:
    time_cuts_ = corryvreckan::calculate_cut<double>("time_cut", 3.0, m_config, get_detectors());

    minHitsOnTrack = m_config.get<size_t>("min_hits_on_track", 6);
    excludeDUT = m_config.get<bool>("exclude_dut", true);
    requireDetectors = m_config.getArray<std::string>("require_detectors", {""});
    timestampFrom = m_config.get<std::string>("timestamp_from", {});
    if(!timestampFrom.empty() &&
       std::find(requireDetectors.begin(), requireDetectors.end(), timestampFrom) == requireDetectors.end()) {
        LOG(WARNING) << "Adding detector " << timestampFrom << " to list of required detectors as it provides the timestamp";
        requireDetectors.push_back(timestampFrom);
    }

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
    spatial_cuts_ = corryvreckan::calculate_cut<XYVector>("spatial_cut", 3.0, m_config, get_detectors());
}

void Tracking4D::initialise() {

    // Set up histograms
    std::string title = "Track #chi^{2};#chi^{2};events";
    trackChi2 = new TH1F("trackChi2", title.c_str(), 300, 0, 150);
    title = "Track #chi^{2}/ndof;#chi^{2}/ndof;events";
    trackChi2ndof = new TH1F("trackChi2ndof", title.c_str(), 500, 0, 50);
    title = "Clusters per track;clusters;tracks";
    clustersPerTrack = new TH1F("clustersPerTrack", title.c_str(), 10, -0.5, 9.5);
    title = "Track multiplicity;tracks;events";
    tracksPerEvent = new TH1F("tracksPerEvent", title.c_str(), 100, -0.5, 99.5);
    title = "Track angle X;angle_{x} [rad];events";
    trackAngleX = new TH1F("trackAngleX", title.c_str(), 2000, -0.01, 0.01);
    title = "Track angle Y;angle_{y} [rad];events";
    trackAngleY = new TH1F("trackAngleY", title.c_str(), 2000, -0.01, 0.01);

    // Loop over all planes
    for(auto& detector : get_detectors()) {
        auto detectorID = detector->getName();

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

double Tracking4D::calculate_average_timestamp(const Track* track) {
    double sum_weighted_time = 0;
    double sum_weights = 0;
    for(auto& cluster : track->clusters()) {
        double weight = 1 / (time_cuts_[get_detector(cluster->getDetectorID())]);
        double time_of_flight = static_cast<double>(Units::convert(cluster->global().z(), "mm") / (299.792458));
        sum_weights += weight;
        sum_weighted_time += (static_cast<double>(Units::convert(cluster->timestamp(), "ns")) - time_of_flight) * weight;
    }
    return (sum_weighted_time / sum_weights);
}

StatusCode Tracking4D::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<std::shared_ptr<Detector>, KDTree<Cluster>> trees;

    std::shared_ptr<Detector> reference_first, reference_last;
    for(auto& detector : get_detectors()) {
        if(excludeDUT && detector->isDUT()) {
            LOG(DEBUG) << "Skipping DUT plane.";
            continue;
        }

        // Get the clusters
        auto tempClusters = clipboard->getData<Cluster>(detector->getName());
        LOG(DEBUG) << "Detector " << detector->getName() << " has " << tempClusters.size() << " clusters on the clipboard";
        if(!tempClusters.empty()) {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters.size() << " clusters from " << detector->getName();

            trees.emplace(std::piecewise_construct, std::make_tuple(detector), std::make_tuple());
            trees[detector].buildTrees(tempClusters);

            // Get first and last detectors with clusters on them:
            if(!reference_first) {
                reference_first = detector;
            }
            reference_last = detector;
        }
    }

    // If there are no detectors then stop trying to track
    if(trees.size() < 2) {
        // Fill histogram
        tracksPerEvent->Fill(0);

        LOG(DEBUG) << "Too few hit detectors for finding a track; end of event.";
        return StatusCode::Success;
    }

    // Output track container
    TrackVector tracks;

    // Time cut for combinations of reference clusters and for reference track with additional detector
    auto time_cut_ref = std::max(time_cuts_[reference_first], time_cuts_[reference_last]);
    auto time_cut_ref_track = std::min(time_cuts_[reference_first], time_cuts_[reference_last]);

    for(auto& clusterFirst : trees[reference_first].getAllElements()) {
        for(auto& clusterLast : trees[reference_last].getAllElements()) {
            LOG(DEBUG) << "Looking at next reference cluster pair";

            if(std::fabs(clusterFirst->timestamp() - clusterLast->timestamp()) > time_cut_ref) {
                LOG(DEBUG) << "Reference clusters not within time cuts.";
                continue;
            }

            // The track finding is based on a straight line. Therefore a refTrack to extrapolate to the next plane is used
            StraightLineTrack refTrack;
            refTrack.addCluster(clusterFirst.get());
            refTrack.addCluster(clusterLast.get());
            auto averageTimestamp = calculate_average_timestamp(&refTrack);
            refTrack.setTimestamp(averageTimestamp);

            // Make a new track
            auto track = Track::Factory(trackModel);
            track->addCluster(clusterFirst.get());
            track->addCluster(clusterLast.get());
            track->setTimestamp(averageTimestamp);
            if(useVolumeScatterer) {
                track->setVolumeScatter(volumeRadiationLength);
            }
            track->setParticleMomentum(momentum);

            // Loop over each subsequent plane and look for a cluster within the timing cuts
            size_t detector_nr = 2;
            for(auto& detector : get_detectors()) {
                if(detector->isAuxiliary()) {
                    continue;
                }
                // always add the material budget:
                track->addMaterial(detector->getName(), detector->materialBudget(), detector->displacement().z());
                LOG(TRACE) << "added material budget for " << detector->getName()
                           << " at z = " << detector->displacement().z();

                if(detector == reference_first || detector == reference_last) {
                    continue;
                }

                if(excludeDUT && detector->isDUT()) {
                    LOG(DEBUG) << "Skipping DUT plane.";
                    continue;
                }

                // Determine whether a track can still be assembled given the number of current hits and the number of
                // detectors to come. Reduces computing time.
                detector_nr++;
                if(refTrack.nClusters() + (trees.size() - detector_nr + 1) < minHitsOnTrack) {
                    LOG(DEBUG) << "No chance to find a track - too few detectors left: " << refTrack.nClusters() << " + "
                               << trees.size() << " - " << detector_nr << " < " << minHitsOnTrack;
                    continue;
                }

                if(trees.count(detector) == 0) {
                    LOG(TRACE) << "Skipping detector " << detector->getName() << " as it has 0 clusters.";
                    continue;
                }

                // Get all neighbors within the timing cut
                LOG(DEBUG) << "Searching for neighboring cluster on device " << detector->getName();
                LOG(DEBUG) << "- reference time is " << Units::display(refTrack.timestamp(), {"ns", "us", "s"});
                Cluster* closestCluster = nullptr;

                // Use spatial cut only as initial value (check if cluster is ellipse defined by cuts is done below):
                double closestClusterDistance = sqrt(spatial_cuts_[detector].x() * spatial_cuts_[detector].x() +
                                                     spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

                double timeCut = std::max(time_cut_ref_track, time_cuts_[detector]);
                LOG(DEBUG) << "Using timing cut of " << Units::display(timeCut, {"ns", "us", "s"});

                auto neighbors = trees[detector].getAllElementsInTimeWindow(refTrack.timestamp(), timeCut);

                LOG(DEBUG) << "- found " << neighbors.size() << " neighbors within the correct time window";

                // Now look for the spatially closest cluster on the next plane
                refTrack.fit();

                PositionVector3D<Cartesian3D<double>> interceptPoint = detector->getIntercept(&refTrack);
                double interceptX = interceptPoint.X();
                double interceptY = interceptPoint.Y();

                for(size_t ne = 0; ne < neighbors.size(); ne++) {
                    auto newCluster = neighbors[ne].get();

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

                    // If this is the closest keep it for now
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
                refTrack.addCluster(closestCluster);
                track->addCluster(closestCluster);
                averageTimestamp = calculate_average_timestamp(&refTrack);
                refTrack.setTimestamp(averageTimestamp);
                track->setTimestamp(averageTimestamp);

                LOG(DEBUG) << "- added cluster to track";
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
            if(!foundRequiredDetector(track.get())) {
                continue;
            }

            // Now should have a track with one cluster from each plane
            if(track->nClusters() < minHitsOnTrack) {
                LOG(DEBUG) << "Not enough clusters on the track, found " << track->nClusters() << " but " << minHitsOnTrack
                           << " required.";
                continue;
            }
            // Fit the track and save it
            track->fit();
            if(track->isFitted()) {
                tracks.push_back(track);
            } else {
                LOG_N(WARNING, 100) << "Rejected a track due to failure in fitting";
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

            for(auto& detector : get_detectors()) {
                if(detector->isAuxiliary()) {
                    continue;
                }
                auto det = detector->getName();
                if(!kinkX.count(det)) {
                    LOG(WARNING) << "Skipping writing kinks due to missing init of histograms for  " << det;
                    continue;
                }

                XYPoint kink = track->getKinkAt(det);
                kinkX.at(det)->Fill(kink.x());
                kinkY.at(det)->Fill(kink.y());
            }

            if(timestampFrom.empty()) {
                // Improve the track timestamp by taking the average of all planes
                auto timestamp = calculate_average_timestamp(track.get());
                track->setTimestamp(timestamp);
                LOG(DEBUG) << "Using average cluster timestamp of " << Units::display(timestamp, "us")
                           << " as track timestamp.";
            } else {
                // use timestamp of required detector:
                double track_timestamp = track->getClusterFromDetector(timestampFrom)->timestamp();
                LOG(DEBUG) << "Using timestamp of detector " << timestampFrom
                           << " as track timestamp: " << Units::display(track_timestamp, "us");
                track->setTimestamp(track_timestamp);
            }
        }
    }

    tracksPerEvent->Fill(static_cast<double>(tracks.size()));

    // Save the tracks on the clipboard
    if(tracks.size() > 0) {
        clipboard->putData(tracks);
    }

    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}
