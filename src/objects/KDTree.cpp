/**
 * @file
 * @brief Implementation of KDTree
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "KDTree.hpp"

using namespace corryvreckan;

KDTree::KDTree(const KDTree& kd) : Object(kd.detectorID(), kd.timestamp()) {
    *xpositions = *kd.xpositions;
    *ypositions = *kd.ypositions;
    *times = *kd.times;
    positionKdtree = kd.positionKdtree;
    timeKdtree = kd.timeKdtree;
    clusters = kd.clusters;
    iteratorNumber = kd.iteratorNumber;
}

void KDTree::buildTimeTree(ClusterVector inputClusters) {
    // Store the vector of cluster pointers
    clusters = inputClusters;

    // Create the data for the ROOT KDTree
    size_t npoints = clusters.size();
    times = new double[npoints];

    // Fill the timing data from the clusters
    for(size_t cluster = 0; cluster < npoints; cluster++) {
        times[cluster] = clusters[cluster]->timestamp();
        iteratorNumber[clusters[cluster]] = cluster;
    }

    // Place the data into the tree and build the structure
    timeKdtree = new TKDTreeID(static_cast<int>(npoints), 1, 1);
    timeKdtree->SetData(0, times);
    timeKdtree->Build();
    timeKdtree->SetOwner(kTRUE);
}

void KDTree::buildSpatialTree(ClusterVector inputClusters) {

    // Store the vector of cluster pointers
    clusters = inputClusters;

    // Create the data for the ROOT KDTree
    size_t npoints = clusters.size();
    xpositions = new double[npoints];
    ypositions = new double[npoints];

    // Fill the x and y data from the clusters
    for(size_t cluster = 0; cluster < npoints; cluster++) {
        xpositions[cluster] = clusters[cluster]->global().x();
        ypositions[cluster] = clusters[cluster]->global().y();
        iteratorNumber[clusters[cluster]] = cluster;
    }

    // Place the data into the tree and build the structure
    positionKdtree = new TKDTreeID(static_cast<int>(npoints), 2, 1);
    positionKdtree->SetData(0, xpositions);
    positionKdtree->SetData(1, ypositions);
    positionKdtree->Build();
    positionKdtree->SetOwner(kTRUE);
}

ClusterVector KDTree::getAllClustersInTimeWindow(Cluster* cluster, double timeWindow) {

    // Get iterators of all clusters within the time window
    std::vector<int> results;

    double time = cluster->timestamp();
    timeKdtree->FindInRange(&time, timeWindow, results);

    // Turn this into a vector of clusters
    ClusterVector resultClusters;
    //    delete time;
    for(size_t res = 0; res < results.size(); res++)
        resultClusters.push_back(clusters[static_cast<size_t>(results[res])]);

    // Return the vector of clusters
    return resultClusters;
}

ClusterVector KDTree::getAllClustersInWindow(Cluster* cluster, double window) {

    // Get iterators of all clusters within the time window
    std::vector<int> results;
    double position[2];
    position[0] = cluster->global().x();
    position[1] = cluster->global().y();
    positionKdtree->FindInRange(position, window, results);

    // Turn this into a vector of clusters
    ClusterVector resultClusters;
    for(size_t res = 0; res < results.size(); res++)
        resultClusters.push_back(clusters[static_cast<size_t>(results[res])]);

    // Return the vector of clusters
    return resultClusters;
}

Cluster* KDTree::getClosestNeighbour(Cluster* cluster) {

    // Get the closest cluster to this one
    int result;
    double distance;
    double position[2];
    position[0] = cluster->global().x();
    position[1] = cluster->global().y();
    positionKdtree->FindNearestNeighbors(position, 1, &result, &distance);

    // Return the cluster
    return clusters[static_cast<size_t>(result)];
}
