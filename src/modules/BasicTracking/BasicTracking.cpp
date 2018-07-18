#include "BasicTracking.h"
#include "TCanvas.h"
#include "objects/KDTree.h"

using namespace corryvreckan;
using namespace std;

BasicTracking::BasicTracking(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    // Default values for cuts
    timingCut = m_config.get<double>("timingCut", Units::convert(200, "ns"));
    spatialCut = m_config.get<double>("spatialCut", Units::convert(0.2, "mm"));
    minHitsOnTrack = m_config.get<int>("minHitsOnTrack", 6);
    // checking if DUT parameter is in the configuration file, if so then check if the the DUT should be excluded, if not
    // then set exclude the DUT
    excludeDUT = (m_config.has("DUT") ? m_config.get<bool>("excludeDUT", true) : true);
}

void BasicTracking::initialise() {

    // Set up histograms
    trackChi2 = new TH1F("trackChi2", "trackChi2", 150, 0, 150);
    trackChi2ndof = new TH1F("trackChi2ndof", "trackChi2ndof", 100, 0, 50);
    clustersPerTrack = new TH1F("clustersPerTrack", "clustersPerTrack", 10, 0, 10);
    tracksPerEvent = new TH1F("tracksPerEvent", "tracksPerEvent", 100, 0, 100);
    trackAngleX = new TH1F("trackAngleX", "trackAngleX", 2000, -0.01, 0.01);
    trackAngleY = new TH1F("trackAngleY", "trackAngleY", 2000, -0.01, 0.01);

    // Loop over all planes
    for(auto& detector : get_detectors()) {
        string detectorID = detector->name();

        string name = "residualsX_" + detectorID;
        residualsX[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsXwidth1_" + detectorID;
        residualsXwidth1[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsXwidth2_" + detectorID;
        residualsXwidth2[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsXwidth3_" + detectorID;
        residualsXwidth3[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsY_" + detectorID;
        residualsY[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsYwidth1_" + detectorID;
        residualsYwidth1[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsYwidth2_" + detectorID;
        residualsYwidth2[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
        name = "residualsYwidth3_" + detectorID;
        residualsYwidth3[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -0.1, 0.1);
    }
}

StatusCode BasicTracking::run(Clipboard* clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<string, KDTree*> trees;
    vector<string> detectors;
    Clusters* referenceClusters = nullptr;

    // Output track container
    Tracks* tracks = new Tracks();

    // Loop over all planes and get clusters
    bool firstDetector = true;
    std::string seedPlane;
    for(auto& detector : get_detectors()) {
        string detectorID = detector->name();

        // Get the clusters
        Clusters* tempClusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(tempClusters == NULL || tempClusters->size() == 0) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
        } else {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters->size() << " clusters from " << detectorID;
            if(firstDetector && (!m_config.has("DUT") || detectorID != m_config.get<std::string>("DUT"))) {
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
    if(detectors.size() == 0)
        return Success;

    // Loop over all clusters
    if(referenceClusters == nullptr)
        return Success;
    int nSeedClusters = referenceClusters->size();
    map<Cluster*, bool> used;
    for(auto& cluster : (*referenceClusters)) {

        // Make a new track
        LOG(DEBUG) << "Looking at next seed cluster";

        Track* track = new Track();
        // Add the cluster to the track
        track->addCluster(cluster);
        track->setTimestamp(cluster->timestamp());
        used[cluster] = true;
        // Get the cluster time
        long long int timestamp = cluster->timestamp();

        // Loop over each subsequent plane and look for a cluster within the timing cuts
        for(auto& detectorID : detectors) {

            // Check if the DUT should be excluded and obey:
            if(excludeDUT && detectorID == m_config.get<std::string>("DUT")) {
                continue;
            }

            if(detectorID == seedPlane)
                continue;
            if(trees.count(detectorID) == 0)
                continue;

            // Get all neighbours within the timing cut
            LOG(DEBUG) << "Searching for neighbouring cluster on " << detectorID;
            LOG(DEBUG) << "- cluster time is " << Units::display(cluster->timestamp(), {"ns", "us", "s"});
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
                LOG(DEBUG) << "No cluster within spatial cut.";
            continue;

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
            if(trackCluster->columnWidth() == 1)
                residualsXwidth1[detectorID]->Fill(intercept.X() - trackCluster->globalX());
            if(trackCluster->columnWidth() == 2)
                residualsXwidth2[detectorID]->Fill(intercept.X() - trackCluster->globalX());
            if(trackCluster->columnWidth() == 3)
                residualsXwidth3[detectorID]->Fill(intercept.X() - trackCluster->globalX());
            residualsY[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
            if(trackCluster->rowWidth() == 1)
                residualsYwidth1[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
            if(trackCluster->rowWidth() == 2)
                residualsYwidth2[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
            if(trackCluster->rowWidth() == 3)
                residualsYwidth3[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
        }
    }

    // Save the tracks on the clipboard
    if(tracks->size() > 0) {
        clipboard->put("tracks", (Objects*)tracks);
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

void BasicTracking::finalise() {}
