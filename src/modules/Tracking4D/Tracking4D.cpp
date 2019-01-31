#include "Tracking4D.h"
#include <TCanvas.h>
#include <TDirectory.h>
#include "objects/KDTree.hpp"

using namespace corryvreckan;
using namespace std;

Tracking4D::Tracking4D(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    // Default values for cuts
    timingCut = m_config.get<double>("timing_cut", static_cast<double>(Units::convert(200, "ns")));
    spatialCut = m_config.get<double>("spatial_cut", static_cast<double>(Units::convert(0.2, "mm")));
    minHitsOnTrack = m_config.get<size_t>("min_hits_on_track", 6);
    excludeDUT = m_config.get<bool>("exclude_dut", true);
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
        auto detectorID = detector->name();

        // Do not create plots for detector snot participating in the tracking:
        if(excludeDUT && detector->isDUT()) {
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
        title = detectorID + " Residual X, size 1;x_{track}-x [mm];events";
        residualsXwidth1[detectorID] = new TH1F("residualsXwidth1", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual X, size 2;x_{track}-x [mm];events";
        residualsXwidth2[detectorID] = new TH1F("residualsXwidth2", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual X, size 3;x_{track}-x [mm];events";
        residualsXwidth3[detectorID] = new TH1F("residualsXwidth3", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y;y_{track}-y [mm];events";
        residualsY[detectorID] = new TH1F("residualsY", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, size 1;y_{track}-y [mm];events";
        residualsYwidth1[detectorID] = new TH1F("residualsYwidth1", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, size 2;y_{track}-y [mm];events";
        residualsYwidth2[detectorID] = new TH1F("residualsYwidth2", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + " Residual Y, size 3;y_{track}-y [mm];events";
        residualsYwidth3[detectorID] = new TH1F("residualsYwidth3", title.c_str(), 500, -0.1, 0.1);

        directory->cd();
    }
}

StatusCode Tracking4D::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<string, KDTree*> trees;
    vector<string> detectors;
    std::shared_ptr<Clusters> referenceClusters = nullptr;

    // Loop over all planes and get clusters
    bool firstDetector = true;
    std::string seedPlane;
    for(auto& detector : get_detectors()) {
        string detectorID = detector->name();

        // Get the clusters
        auto tempClusters = clipboard->get<Clusters>(detectorID);
        if(tempClusters == nullptr || tempClusters->size() == 0) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
        } else {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters->size() << " clusters from " << detectorID;
            if(firstDetector && !detector->isDUT()) {
                referenceClusters = tempClusters;
                seedPlane = detector->name();
                LOG(DEBUG) << "Seed plane is " << seedPlane;
                firstDetector = false;
            }

            KDTree* clusterTree = new KDTree();
            clusterTree->buildTimeTree(*tempClusters);
            trees[detectorID] = clusterTree;
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
    auto tracks = std::make_shared<Tracks>();

    // Loop over all clusters
    for(auto& cluster : (*referenceClusters)) {

        // Make a new track
        LOG(DEBUG) << "Looking at next seed cluster";

        Track* track = new Track();
        // Add the cluster to the track
        track->addCluster(cluster);
        track->setTimestamp(cluster->timestamp());

        // Loop over each subsequent plane and look for a cluster within the timing cuts
        for(auto& detectorID : detectors) {
            // Get the detector
            auto det = get_detector(detectorID);

            // Check if the DUT should be excluded and obey:
            if(excludeDUT && det->isDUT()) {
                LOG(DEBUG) << "Skipping DUT plane.";
                continue;
            }

            if(detectorID == seedPlane)
                continue;
            if(trees.count(detectorID) == 0)
                continue;

            // Get all neighbours within the timing cut
            LOG(DEBUG) << "Searching for neighbouring cluster on " << detectorID;
            LOG(DEBUG) << "- cluster time is " << Units::display(cluster->timestamp(), {"ns", "us", "s"});
            Cluster* closestCluster = nullptr;
            double closestClusterDistance = spatialCut;
            Clusters neighbours = trees[detectorID]->getAllClustersInTimeWindow(cluster, timingCut);

            LOG(DEBUG) << "- found " << neighbours.size() << " neighbours";

            // Now look for the spatially closest cluster on the next plane
            double interceptX, interceptY;
            if(track->nClusters() > 1) {
                track->fit();

                PositionVector3D<Cartesian3D<double>> interceptPoint = det->getIntercept(track);
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
                double distance = sqrt((interceptX - newCluster->global().x()) * (interceptX - newCluster->global().x()) +
                                       (interceptY - newCluster->global().y()) * (interceptY - newCluster->global().y()));

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
        } //*/

        // Now should have a track with one cluster from each plane
        if(track->nClusters() < minHitsOnTrack) {
            LOG(DEBUG) << "Not enough clusters on the track, found " << track->nClusters() << " but " << minHitsOnTrack
                       << " required.";
            delete track;
            continue;
        }

        // Fit the track and save it
        track->fit();
        tracks->push_back(track);

        // Fill histograms
        trackChi2->Fill(track->chi2());
        clustersPerTrack->Fill(static_cast<double>(track->nClusters()));
        trackChi2ndof->Fill(track->chi2ndof());
        trackAngleX->Fill(atan(track->direction().X()));
        trackAngleY->Fill(atan(track->direction().Y()));

        // Make residuals
        Clusters trackClusters = track->clusters();
        for(auto& trackCluster : trackClusters) {
            string detectorID = trackCluster->detectorID();
            ROOT::Math::XYZPoint intercept = track->intercept(trackCluster->global().z());
            residualsX[detectorID]->Fill(intercept.X() - trackCluster->global().x());
            if(trackCluster->columnWidth() == 1)
                residualsXwidth1[detectorID]->Fill(intercept.X() - trackCluster->global().x());
            if(trackCluster->columnWidth() == 2)
                residualsXwidth2[detectorID]->Fill(intercept.X() - trackCluster->global().x());
            if(trackCluster->columnWidth() == 3)
                residualsXwidth3[detectorID]->Fill(intercept.X() - trackCluster->global().x());
            residualsY[detectorID]->Fill(intercept.Y() - trackCluster->global().y());
            if(trackCluster->rowWidth() == 1)
                residualsYwidth1[detectorID]->Fill(intercept.Y() - trackCluster->global().y());
            if(trackCluster->rowWidth() == 2)
                residualsYwidth2[detectorID]->Fill(intercept.Y() - trackCluster->global().y());
            if(trackCluster->rowWidth() == 3)
                residualsYwidth3[detectorID]->Fill(intercept.Y() - trackCluster->global().y());
        }

        // Improve the track timestamp by taking the average of all planes
        double avg_track_time = 0;
        for(auto& trackCluster : trackClusters) {
            avg_track_time += static_cast<double>(Units::convert(trackCluster->timestamp(), "ns"));
            avg_track_time -= static_cast<double>(Units::convert(trackCluster->global().z(), "mm") / (299.792458));
        }
        track->setTimestamp(avg_track_time / static_cast<double>(track->nClusters()));
    }

    tracksPerEvent->Fill(static_cast<double>(tracks->size()));

    // Save the tracks on the clipboard
    if(tracks->size() > 0) {
        clipboard->put(tracks);
    }

    // Clean up tree objects
    for(auto tree = trees.cbegin(); tree != trees.cend();) {
        delete tree->second;
        tree = trees.erase(tree);
    }

    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}
