#ifndef KDTREE__H
#define KDTREE__H 1

#include <map>
#include "Cluster.h"
#include "TKDTree.h"
#include "TestBeamObject.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"

/*

 This class is effectively just a wrapper for the root TKDTree class that
 handles  clusters and converts them into the format needed by
 ROOT.

*/

using namespace corryvreckan;

class KDTree : public TestBeamObject {

public:
    // Constructors and destructors
    KDTree() {
        timeKdtree = NULL;
        positionKdtree = NULL;
    }
    ~KDTree() {
        delete timeKdtree;
        delete positionKdtree;
    }

    // Build a tree sorted by cluster times
    void buildTimeTree(Clusters inputClusters) {

        // Store the vector of cluster pointers
        clusters = inputClusters;

        // Create the data for the ROOT KDTree
        int npoints = clusters.size();
        times = new double[npoints];

        // Fill the timing data from the clusters
        for(int cluster = 0; cluster < npoints; cluster++) {
            times[cluster] = clusters[cluster]->timestamp();
            iteratorNumber[clusters[cluster]] = cluster;
        }

        // Place the data into the tree and build the structure
        timeKdtree = new TKDTreeID(npoints, 1, 1);
        timeKdtree->SetData(0, times);
        timeKdtree->Build();
        timeKdtree->SetOwner(kTRUE);
    }

    // Build a tree sorted by cluster xy positions
    void buildSpatialTree(Clusters inputClusters) {

        // Store the vector of cluster pointers
        clusters = inputClusters;

        // Create the data for the ROOT KDTree
        int npoints = clusters.size();
        xpositions = new double[npoints];
        ypositions = new double[npoints];

        // Fill the x and y data from the clusters
        for(int cluster = 0; cluster < npoints; cluster++) {
            xpositions[cluster] = clusters[cluster]->globalX();
            ypositions[cluster] = clusters[cluster]->globalY();
            iteratorNumber[clusters[cluster]] = cluster;
        }

        // Place the data into the tree and build the structure
        positionKdtree = new TKDTreeID(npoints, 2, 1);
        positionKdtree->SetData(0, xpositions);
        positionKdtree->SetData(1, ypositions);
        positionKdtree->Build();
        positionKdtree->SetOwner(kTRUE);
    }

    // Function to get back all clusters within a given time period
    Clusters getAllClustersInTimeWindow(Cluster* cluster, double timeWindow) {

        LOG(TRACE) << "Getting all clusters in time window " << Units::display(timeWindow, {"ns", "us", "s"});
        // Find out which iterator number this cluster corresponds to
        //    int iterator = iteratorNumber[cluster];

        // Get iterators of all clusters within the time window
        std::vector<int> results;

        double time = cluster->timestamp();
        timeKdtree->FindInRange(&time, timeWindow, results);

        LOG(TRACE) << " -- found: " << results.size();

        // Turn this into a vector of clusters
        Clusters resultClusters;
        //    delete time;
        for(int res = 0; res < results.size(); res++)
            resultClusters.push_back(clusters[results[res]]);

        // Return the vector of clusters
        return resultClusters;
    }

    // Function to get back all clusters within a given spatial window
    Clusters getAllClustersInWindow(Cluster* cluster, double window) {

        // Get iterators of all clusters within the time window
        std::vector<int> results;
        double* position;
        position[0] = cluster->globalX();
        position[1] = cluster->globalY();
        positionKdtree->FindInRange(position, window, results);

        // Turn this into a vector of clusters
        Clusters resultClusters;
        for(int res = 0; res < results.size(); res++)
            resultClusters.push_back(clusters[results[res]]);

        // Return the vector of clusters
        return resultClusters;
    }

    // Function to get back the nearest cluster in space
    Cluster* getClosestNeighbour(Cluster* cluster) {

        // Get the closest cluster to this one
        int result;
        double distance;
        double position[2];
        position[0] = cluster->globalX();
        position[1] = cluster->globalY();
        positionKdtree->FindNearestNeighbors(position, 1, &result, &distance);

        // Return the cluster
        return clusters[result];
    }

    // Member variables
    double* xpositions; //!
    double* ypositions; //!
    double* times;      //!
    TKDTreeID* positionKdtree;
    TKDTreeID* timeKdtree;
    Clusters clusters;
    std::map<Cluster*, int> iteratorNumber;

    // ROOT I/O class definition - update version number when you change this
    // class!
    ClassDef(KDTree, 2)
};

#endif // KDTREE__H
