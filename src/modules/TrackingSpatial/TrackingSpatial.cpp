/**
 * @file
 * @brief Implementation of module TrackingSpatial
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TrackingSpatial.h"
#include <TDirectory.h>

#include "tools/cuts.h"
#include "tools/kdtree.h"

using namespace corryvreckan;
using namespace std;

/*

This algorithm performs the track finding using only spatial information
(no timing). It is based on a linear extrapolation along the z axis, followed
by a nearest neighbor search, and should be well adapted to testbeam
reconstruction with a mostly colinear beam.

*/

TrackingSpatial::TrackingSpatial(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    // Backwards compatibility: also allow spatial_cut to be used for spatial_cut_abs
    config_.setAlias("spatial_cut_abs", "spatial_cut", true);

    config_.setDefault<size_t>("min_hits_on_track", 6);
    config_.setDefault<bool>("exclude_dut", true);
    config_.setDefault<std::string>("track_model", "straightline");
    config_.setDefault<bool>("reject_by_roi", false);

    if(config_.count({"spatial_cut_rel", "spatial_cut_abs"}) == 0) {
        config_.setDefault("spatial_cut_rel", 3.0);
    }

    minHitsOnTrack = config_.get<size_t>("min_hits_on_track");
    excludeDUT = config_.get<bool>("exclude_dut");
    trackModel = config_.get<std::string>("track_model");
    rejectByROI = config_.get<bool>("reject_by_roi");

    // spatial cut, relative (x * spatial_resolution) or absolute:
    spatial_cuts_ = corryvreckan::calculate_cut<XYVector>("spatial_cut", config_, get_regular_detectors(true));
}

void TrackingSpatial::initialize() {

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
    for(auto& detector : get_regular_detectors(!excludeDUT)) {
        auto detectorID = detector->getName();

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

        directory->cd();
    }
}

StatusCode TrackingSpatial::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Container for all clusters, and detectors in tracking
    map<string, KDTree<Cluster>> trees;
    vector<std::shared_ptr<Detector>> detectors;
    ClusterVector referenceClusters;

    // Loop over all detectors and get clusters
    double minZ = std::numeric_limits<double>::max();
    std::string seedPlane;
    for(auto& detector : get_regular_detectors(excludeDUT)) {
        string detectorID = detector->getName();

        // Get the clusters
        auto tempClusters = clipboard->getData<Cluster>(detectorID);
        if(!tempClusters.empty()) {
            // Store the clusters of the first plane in Z as the reference
            if(detector->displacement().Z() < minZ) {
                referenceClusters = tempClusters;
                seedPlane = detector->getName();
                minZ = detector->displacement().Z();
                LOG(TRACE) << "minZ = " << minZ;
            }
            if(tempClusters.size() == 0) {
                LOG(TRACE) << "tempClusters->size() == 0";
                continue;
            }

            // Make trees of the clusters on each plane
            trees.emplace(std::piecewise_construct, std::make_tuple(detectorID), std::make_tuple());
            trees[detectorID].buildTrees(tempClusters);

            detectors.push_back(detector);
            LOG(DEBUG) << "Picked up " << tempClusters.size() << " clusters on device " << detectorID;
        }
    }

    // If there are no detectors then stop trying to track
    if(detectors.empty() || referenceClusters.empty()) {
        LOG(DEBUG) << "There are no detectors, reference clusters are empty.";
        return StatusCode::NoData;
    }

    // Output track container
    LOG(TRACE) << "Initialise tracks object";
    TrackVector tracks;

    LOG(DEBUG) << "referenceClusters: " << referenceClusters.size();
    // Loop over all clusters
    for(auto& cluster : referenceClusters) {

        LOG(DEBUG) << "Looping over clusters.";
        // Make a new track
        auto track = Track::Factory(trackModel);

        // Add the cluster to the track
        track->addCluster(cluster.get());

        // Loop over each subsequent planes. For each plane, if extrapolate
        // the hit from the previous plane along the z axis, and look for
        // a neighbor on the new plane. We started on the most upstream
        // plane, so first detector is 1 (not 0)
        for(auto& detector : detectors) {
            auto detectorID = detector->getName();
            if(trees.count(detectorID) == 0) {
                LOG(TRACE) << "Skipping detector " << detector->getName() << " as it has 0 clusters.";
                continue;
            }
            if(detectorID == seedPlane) {
                LOG(TRACE) << "Skipping seed plane.";
                continue;
            }

            // Check if the DUT should be excluded and obey:
            if(excludeDUT && detector->isDUT()) {
                LOG(TRACE) << "Exclude DUT.";
                continue;
            }

            // Get the closest neighbor
            LOG(DEBUG) << "Searching for nearest cluster on device " << detectorID;
            Cluster* closestCluster = trees[detectorID].getClosestSpaceNeighbor(cluster).get();

            double distanceX = (cluster->global().x() - closestCluster->global().x());
            double distanceY = (cluster->global().y() - closestCluster->global().y());
            double distance = sqrt(distanceX * distanceX + distanceY * distanceY);

            // Check if closestCluster lies within ellipse defined by spatial cuts,
            // following this example:
            // https://www.geeksforgeeks.org/check-if-a-point-is-inside-outside-or-on-the-ellipse/
            //
            // ellipse defined by: x^2/a^2 + y^2/b^2 = 1: on ellipse,
            //                                       > 1: outside,
            //                                       < 1: inside
            // Continue if on or outside of ellipse:

            double norm = (distanceX * distanceX) / (spatial_cuts_[detector].x() * spatial_cuts_[detector].x()) +
                          (distanceY * distanceY) / (spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

            if(norm > 1) {
                continue;
            }

            // Add the cluster to the track
            track->addCluster(closestCluster);
            LOG(DEBUG) << "- added cluster to track. Distance is " << distance;
        }

        // Now should have a track with one cluster from each plane
        if(track->getNClusters() < minHitsOnTrack) {
            LOG(DEBUG) << "Not enough clusters on the track, found " << track->getNClusters() << " but " << minHitsOnTrack
                       << " required.";
            continue;
        }

        // Fit the track
        LOG(TRACE) << "Fitting the track.";
        track->fit();

        if(rejectByROI && track->isFitted()) {
            // check if the track is within ROI for all detectors
            auto ds = get_regular_detectors(excludeDUT);
            auto out_of_roi =
                std::find_if(ds.begin(), ds.end(), [track](const auto& d) { return !d->isWithinROI(track.get()); });
            if(out_of_roi != ds.end()) {
                LOG(DEBUG) << "Rejecting track outside of ROI of detetctor " << out_of_roi->get()->getName();
                continue;
            }
        }
        // save the track
        if(track->isFitted()) {
            tracks.push_back(track);
        } else {
            LOG_N(WARNING, 100) << "Rejected a track due to failure in fitting";
            continue;
        }

        // Fill histograms
        trackChi2->Fill(track->getChi2());
        clustersPerTrack->Fill(static_cast<double>(track->getNClusters()));
        trackChi2ndof->Fill(track->getChi2ndof());
        trackAngleX->Fill(atan(track->getDirection(track->getClusters().front()->detectorID()).X()));
        trackAngleY->Fill(atan(track->getDirection(track->getClusters().front()->detectorID()).Y()));

        // Make residuals
        for(auto& trackCluster : track->getClusters()) {
            LOG(TRACE) << "Loop over track clusters.";
            string detectorID = trackCluster->detectorID();
            ROOT::Math::XYZPoint intercept = track->getIntercept(trackCluster->global().z());
            residualsX[detectorID]->Fill(intercept.X() - trackCluster->global().x());
            residualsY[detectorID]->Fill(intercept.Y() - trackCluster->global().y());
        }
    }

    // Save the tracks on the clipboard
    tracksPerEvent->Fill(static_cast<double>(tracks.size()));
    if(tracks.size() > 0) {
        clipboard->putData(tracks);
    }

    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}
