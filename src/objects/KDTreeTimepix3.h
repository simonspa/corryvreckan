#ifndef KDTREE_TIMEPIX3_H
#define KDTREE_TIMEPIX3_H 1

#include <map>
#include "TKDTree.h"
#include "TestBeamObject.h"
#include "Timepix3Cluster.h"
#include "core/utils/log.h"
// This class is effectively just a wrapper for the root TKDTree class that
// handles Timepix3 clusters and converts them into the format needed by
// ROOT.

class KDTreeTimepix3 : public TestBeamObject {

public:
    // Constructors and destructors
    KDTreeTimepix3() {}
    virtual ~KDTreeTimepix3() {
        //    delete timeKdtree;
        //    delete times;
    }

    // Constructor with Timepix3Cluster data
    KDTreeTimepix3(Timepix3Clusters inputClusters) {

        // Store the vector of cluster pointers
        clusters = inputClusters;

        // Create the data for the ROOT KDTree
        int npoints = clusters.size();
        //   	xpositions = new double[npoints];
        //    ypositions = new double[npoints];
        times = new double[npoints];

        // Fill the x and y data from the global cluster positions
        for(int cluster = 0; cluster < npoints; cluster++) {
            //      xpositions[cluster] = clusters[cluster]->globalX();
            //      ypositions[cluster] = clusters[cluster]->globalY();
            times[cluster] = double(clusters[cluster]->timestamp()) / (4096. * 40000000.);
            iteratorNumber[clusters[cluster]] = cluster;
        }

        // Place the data into the trees and build the structure
        //    positionKdtree = new TKDTreeID(npoints,2,1);
        //    positionKdtree->SetData(0,xpositions);
        //    positionKdtree->SetData(1,ypositions);
        //    positionKdtree->Build();

        timeKdtree = new TKDTreeID(npoints, 1, 1);
        timeKdtree->SetData(0, times);
        timeKdtree->Build();
        timeKdtree->SetOwner(kTRUE);
    }

    // Function to get back all clusters within a given time period
    Timepix3Clusters getAllClustersInTimeWindow(Timepix3Cluster* cluster, double timeWindow) {

        LOG(TRACE) << "Getting all clusters in time window" << timeWindow;
        // Find out which iterator number this cluster corresponds to
        //    int iterator = iteratorNumber[cluster];

        // Get iterators of all clusters within the time window
        std::vector<int> results;
        double* time;
        time[0] = double(cluster->timestamp()) / (4096. * 40000000.);
        timeKdtree->FindInRange(time, timeWindow, results);

        LOG(TRACE) << " -- found: " << results.size();

        // Turn this into a vector of clusters
        Timepix3Clusters resultClusters;
        //    delete time;
        for(int res = 0; res < results.size(); res++)
            resultClusters.push_back(clusters[results[res]]);

        // Return the vector of clusters
        return resultClusters;
    }

    // Member variables
    double* xpositions; //!
    double* ypositions; //!
    double* times;      //!
    TKDTreeID* positionKdtree;
    TKDTreeID* timeKdtree;
    Timepix3Clusters clusters;
    std::map<Timepix3Cluster*, int> iteratorNumber;

    // ROOT I/O class definition - update version number when you change this
    // class!
    ClassDef(KDTreeTimepix3, 1)
};

#endif // KDTREE_TIMEPIX3_H
