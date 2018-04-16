#include "SpatialTracking.h"
#include "objects/KDTree.h"

using namespace corryvreckan;
using namespace std;

SpatialTracking::SpatialTracking(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    spatialCut = m_config.get<double>("spatialCut", Units::convert(200, "um"));
    spatialCut_DUT = m_config.get<double>("spatialCutDUT", Units::convert(200, "um"));
    minHitsOnTrack = m_config.get<int>("minHitsOnTrack", 6);
    excludeDUT = m_config.get<bool>("excludeDUT", true);
}

/*

 This algorithm performs the track finding using only spatial information
 (no timing). It is based on a linear extrapolation along the z axis, followed
 by a nearest neighbour search, and should be well adapted to testbeam
 reconstruction with a mostly colinear beam.

 */

void SpatialTracking::initialise() {

    // Set up histograms
    trackChi2 = new TH1F("trackChi2", "trackChi2", 150, 0, 150);
    trackChi2ndof = new TH1F("trackChi2ndof", "trackChi2ndof", 100, 0, 50);
    clustersPerTrack = new TH1F("clustersPerTrack", "clustersPerTrack", 10, 0, 10);
    tracksPerEvent = new TH1F("tracksPerEvent", "tracksPerEvent", 100, 0, 100);
    trackAngleX = new TH1F("trackAngleX", "trackAngleX", 2000, -0.01, 0.01);
    trackAngleY = new TH1F("trackAngleY", "trackAngleY", 2000, -0.01, 0.01);

    // Loop over all Timepix1
    for(auto& detector : get_detectors()) {
        // Check if they are a Timepix3
        // if(detector->type() != "Timepix1")
        //     continue;
        string name = "residualsX_" + detector->name();
        residualsX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.05, 0.05);
        name = "residualsY_" + detector->name();
        residualsY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.05, 0.05);
    }

    // Initialise member variables
    m_eventNumber = 0;
    nTracksTotal = 0.;
}

StatusCode SpatialTracking::run(Clipboard* clipboard) {

    // Container for all clusters, and detectors in tracking
    map<string, KDTree*> trees;
    vector<string> detectors;
    Clusters* referenceClusters;
    Clusters dutClusters;

    // Output track container
    Tracks* tracks = new Tracks();

    // Loop over all Timepix1 and get clusters
    double minZ = 1000.;
    for(auto& detector : get_detectors()) {

        // Check if they are a Timepix1
        string detectorID = detector->name();
        // if(detector->type() != "Timepix1")
        //     continue;

        // Get the clusters
        Clusters* tempClusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(tempClusters == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
        } else {
            // Store the clusters of the first plane in Z as the reference
            if(detector->displacementZ() < minZ) {
                referenceClusters = tempClusters;
                minZ = detector->displacementZ();
            }
            if(tempClusters->size() == 0)
                continue;

            // Make trees of the clusters on each plane
            KDTree* clusterTree = new KDTree();
            clusterTree->buildSpatialTree(*tempClusters);
            trees[detectorID] = clusterTree;
            detectors.push_back(detectorID);
            LOG(DEBUG) << "Picked up " << tempClusters->size() << " clusters on device " << detectorID;
        }
    }

    // If there are no detectors then stop trying to track
    if(detectors.size() == 0)
        return Success;

    // Keep a note of which clusters have been used
    map<Cluster*, bool> used;

    // Loop over all clusters
    int nSeedClusters = referenceClusters->size();
    for(int iSeedCluster = 0; iSeedCluster < nSeedClusters; iSeedCluster++) {

        LOG(DEBUG) << "==> seed cluster " << iSeedCluster;

        // Make a new track
        Track* track = new Track();

        // Get the cluster
        Cluster* cluster = (*referenceClusters)[iSeedCluster];

        // Add the cluster to the track
        track->addCluster(cluster);
        used[cluster] = true;

        // Loop over each subsequent planes. For each plane, if extrapolate
        // the hit from the previous plane along the z axis, and look for
        // a neighbour on the new plane. We started on the most upstream
        // plane, so first detector is 1 (not 0)
        for(auto& detectorID : detectors) {

            if(trees.count(detectorID) == 0)
                continue;

            // Check if the DUT should be excluded and obey:
            if(excludeDUT && detectorID == m_config.get<std::string>("DUT")) {
                // Keep all DUT clusters, so we can add them as associated clusters later:
                Cluster* dutCluster = trees[detectorID]->getClosestNeighbour(cluster);
                dutClusters.push_back(dutCluster);
                continue;
            }

            // Get the closest neighbour
            LOG(DEBUG) << "- looking for nearest cluster on device " << detectorID;
            Cluster* closestCluster = trees[detectorID]->getClosestNeighbour(cluster);

            LOG(DEBUG) << "still alive";
            // If it is used do nothing
            //      if(used[closestCluster]) continue;

            // Check if it is within the spatial window
            double distance =
                sqrt((cluster->globalX() - closestCluster->globalX()) * (cluster->globalX() - closestCluster->globalX()) +
                     (cluster->globalY() - closestCluster->globalY()) * (cluster->globalY() - closestCluster->globalY()));

            if(distance > spatialCut)
                continue;

            // Add the cluster to the track
            track->addCluster(closestCluster);
            cluster = closestCluster;
            LOG(DEBUG) << "- added cluster to track. Distance is " << distance;
        }

        // Now should have a track with one cluster from each plane
        if(track->nClusters() < minHitsOnTrack) {
            delete track;
            continue;
        }

        // Fit the track
        track->fit();

        // Save the track
        tracks->push_back(track);

        // Fill histograms
        trackChi2->Fill(track->chi2());
        clustersPerTrack->Fill(track->nClusters());
        trackChi2ndof->Fill(track->chi2ndof());
        trackAngleX->Fill(atan(track->m_direction.X()));
        trackAngleY->Fill(atan(track->m_direction.Y()));

        // Make residuals
        Clusters trackClusters = track->clusters();
        for(auto& trackCluster : trackClusters) {
            string detectorID = trackCluster->detectorID();
            ROOT::Math::XYZPoint intercept = track->intercept(trackCluster->globalZ());
            residualsX[detectorID]->Fill(intercept.X() - trackCluster->globalX());
            residualsY[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
        }

        // Add potential associated clusters from the DUT:
        for(auto& dutcluster : dutClusters) {

            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(dutcluster->globalZ());
            double xdistance = intercept.X() - dutcluster->globalX();
            double ydistance = intercept.Y() - dutcluster->globalY();
            if(abs(xdistance) > spatialCut_DUT)
                continue;
            if(abs(ydistance) > spatialCut_DUT)
                continue;

            LOG(DEBUG) << "Found associated cluster";
            track->addAssociatedCluster(dutcluster);
        }
    }

    // Save the tracks on the clipboard
    tracksPerEvent->Fill(tracks->size());
    if(tracks->size() > 0) {
        clipboard->put("tracks", (Objects*)tracks);
    }

    // Clean up tree objects
    for(auto& detector : get_detectors()) {
        if(trees.count(detector->name()) != 0)
            delete trees[detector->name()];
    }

    return Success;
}

void SpatialTracking::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
