#include "BasicTracking.h"
#include "TCanvas.h"
#include "objects/KDTree.h"

using namespace corryvreckan;
using namespace std;

BasicTracking::BasicTracking(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    // Default values for cuts
    timingCut = m_config.get<double>("timingCut", 200. / 1000000000.); // 200 ns
    spatialCut = m_config.get<double>("spatialCut", 0.2);              // 200 um
    minHitsOnTrack = m_config.get<int>("minHitsOnTrack", 6);
    excludeDUT = m_config.get<bool>("excludeDUT", true);
}

void BasicTracking::initialise() {

    // Set up histograms
    trackChi2 = new TH1F("trackChi2", "trackChi2", 150, 0, 150);
    trackChi2ndof = new TH1F("trackChi2ndof", "trackChi2ndof", 100, 0, 50);
    clustersPerTrack = new TH1F("clustersPerTrack", "clustersPerTrack", 10, 0, 10);
    tracksPerEvent = new TH1F("tracksPerEvent", "tracksPerEvent", 100, 0, 100);
    trackAngleX = new TH1F("trackAngleX", "trackAngleX", 2000, -0.01, 0.01);
    trackAngleY = new TH1F("trackAngleY", "trackAngleY", 2000, -0.01, 0.01);

    // Loop over all Timepix3
    for(auto& detector : get_detectors()) {
        // Check if they are a Timepix3
        string detectorID = detector->name();
        if(detector->type() != "Timepix3")
            continue;
        string name = "residualsX_" + detectorID;
        residualsX[detectorID] = new TH1F(name.c_str(), name.c_str(), 100, -0.02, 0.02);
        name = "residualsY_" + detectorID;
        residualsY[detectorID] = new TH1F(name.c_str(), name.c_str(), 100, -0.02, 0.02);
    }
}

StatusCode BasicTracking::run(Clipboard* clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<string, KDTree*> trees;
    vector<string> detectors;
    Clusters* referenceClusters;

    // Output track container
    Tracks* tracks = new Tracks();

    // Loop over all Timepix3 and get clusters
    bool firstDetector = true;
    std::string seedPlane;
    for(auto& detector : get_detectors()) {

        // Check if they are a Timepix3
        string detectorID = detector->name();
        if(detector->type() != "Timepix3")
            continue;

        // Get the clusters
        Clusters* tempClusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(tempClusters == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
        } else {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters->size() << " clusters from " << detectorID;
            if(firstDetector) {
                referenceClusters = tempClusters;
                seedPlane = detector->name();
            }
            firstDetector = false;
            if(tempClusters->size() == 0)
                continue;
            KDTree* clusterTree = new KDTree();
            clusterTree->buildTimeTree(*tempClusters);
            trees[detectorID] = clusterTree;
            detectors.push_back(detectorID);
        }
    }

    // If there are no detectors then stop trying to track
    if(detectors.size() == 0)
        return Success;

    // Loop over all clusters
    int nSeedClusters = referenceClusters->size();
    map<Cluster*, bool> used;
    for(auto& cluster : (*referenceClusters)) {

        // Make a new track
        LOG(DEBUG) << "Looking next seed cluster";

        Track* track = new Track();
        // Add the cluster to the track
        track->addCluster(cluster);
        track->setTimestamp(cluster->timestamp());
        used[cluster] = true;
        // Get the cluster time
        long long int timestamp = cluster->timestamp();

        // Loop over each subsequent plane and look for a cluster within 100 ns
        for(auto& detectorID : detectors) {
            // Check if the DUT should be excluded and obey:
            if(excludeDUT && detectorID == m_config.get<std::string>("DUT"))
                continue;
            if(detectorID == seedPlane)
                continue;
            if(trees.count(detectorID) == 0)
                continue;

            // Get all neighbours within 200 ns
            LOG(DEBUG) << "Searching for neighbouring cluster on " << detectorID;
            LOG(DEBUG) << "- cluster time is " << cluster->timestamp();
            Cluster* closestCluster = NULL;
            double closestClusterDistance = spatialCut;
            Clusters neighbours = trees[detectorID]->getAllClustersInTimeWindow(cluster, timingCut);

            LOG(DEBUG) << "- found " << neighbours.size() << " neighbours";

            // Now look for the spatially closest cluster on the next plane
            double interceptX, interceptY;
            if(track->nClusters() > 1) {
                track->fit();

                // Get the detector
                auto det = get_detector(detectorID);
                PositionVector3D<Cartesian3D<double>> interceptPoint = det->getIntercept(track);
                interceptX = interceptPoint.X();
                interceptY = interceptPoint.Y();
            } else {
                interceptX = cluster->globalX();
                interceptY = cluster->globalY();
            }

            // Loop over each neighbour in time
            for(int ne = 0; ne < neighbours.size(); ne++) {
                Cluster* newCluster = neighbours[ne];

                // Calculate the distance to the previous plane's cluster/intercept
                double distance = sqrt((interceptX - newCluster->globalX()) * (interceptX - newCluster->globalX()) +
                                       (interceptY - newCluster->globalY()) * (interceptY - newCluster->globalY()));

                // If this is the closest keep it
                if(distance < closestClusterDistance) {
                    closestClusterDistance = distance;
                    closestCluster = newCluster;
                }
            }

            if(closestCluster == NULL)
                continue;

            // Add the cluster to the track
            LOG(DEBUG) << "- added cluster to track";
            track->addCluster(closestCluster);
        } //*/

        // Now should have a track with one cluster from each plane
        if(track->nClusters() < minHitsOnTrack) {
            delete track;
            continue;
        }

        // Fit the track and save it
        track->fit();
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
    }

    // Save the tracks on the clipboard
    if(tracks->size() > 0) {
        clipboard->put("tracks", (TestBeamObjects*)tracks);
        tracksPerEvent->Fill(tracks->size());
    }

    // Clean up tree objects
    for(auto& detector : get_detectors()) {
        if(trees.count(detector->name()) != 0)
            delete trees[detector->name()];
    }

    LOG(DEBUG) << "End of event";
    return Success;
}

Cluster* BasicTracking::getNearestCluster(long long int timestamp, Clusters clusters) {

    Cluster* bestCluster = NULL;
    // Loop over all clusters and return the one with the closest timestamp
    for(int iCluster = 0; iCluster < clusters.size(); iCluster++) {
        Cluster* cluster = clusters[iCluster];
        if(bestCluster == NULL)
            bestCluster = cluster;
        if(abs(cluster->timestamp() - timestamp) < abs(bestCluster->timestamp() - timestamp))
            bestCluster = cluster;
    }

    return bestCluster;
}

// Timepix3Cluster* BasicTracking::getNearestCluster(Timepix3Cluster* cluster,
// map<Timepix3Cluster*, bool> used, Timepix3Clusters* clusters){
//
//  // Loop over all clusters and return the closest in space with an acceptable
//  time cut
//  Timepix3Cluster* bestCluster = NULL;
//  double closestApproach = 1000000.;
//
//  // Loop over all clusters
//  for(int iCluster=0;iCluster<clusters->size();iCluster++){
//    Timepix3Cluster* candidate = (*clusters)[iCluster];
//    // Check if within time window
//    if( abs((double)((candidate->timestamp() - cluster->timestamp()) /
//    (4096.*40000000.))) > timinigCut ) continue;
//    // Check how close it is (2D - assumes z-axis parallel to beam)
//    if(bestCluster == NULL){ bestCluster = candidate; continue; }
//    double distanceX = candidate->globalX() - cluster->globalX();
//    double distanceY = candidate->globalY() - cluster->globalY();
//    double approach = sqrt(distanceX*distanceX + distanceY*distanceY);
//    // Check if it is closer than previous clusters, and apply spatial cut
//    if( approach < closestApproach && approach < spatialCut){
//      bestCluster = candidate;
//      closestApproach = approach;
//    }
//  }
//
//  return bestCluster;
//}

void BasicTracking::finalise() {}
