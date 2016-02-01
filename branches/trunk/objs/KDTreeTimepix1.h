#ifndef KDTREE_TIMEPIX1_H
#define KDTREE_TIMEPIX1_H 1

#include "TestBeamObject.h"
#include "Timepix1Cluster.h"
#include "TKDTree.h"
#include <map>

// This class is effectively just a wrapper for the root TKDTree class that
// handles Timepix1 clusters and converts them into the format needed by
// ROOT.

class KDTreeTimepix1 : public TestBeamObject {
  
public:
  // Constructors and destructors
  KDTreeTimepix1(){}
  virtual ~KDTreeTimepix1(){}

  // Constructor with Timepix1Cluster data
  KDTreeTimepix1(Timepix1Clusters inputClusters){
    
    // Store the vector of cluster pointers
    clusters = inputClusters;
    
    // Create the data for the ROOT KDTree
    int npoints = clusters.size();
   	xpositions = new double[npoints];
    ypositions = new double[npoints];
  
    // Fill the x and y data from the global cluster positions
    for(int cluster=0;cluster<npoints;cluster++){
      xpositions[cluster] = clusters[cluster]->globalX();
      ypositions[cluster] = clusters[cluster]->globalY();
      iteratorNumber[clusters[cluster]] = cluster;
    }
  
    // Place the data into the trees and build the structure
    positionKdtree = new TKDTreeID(npoints,2,1);
    positionKdtree->SetData(0,xpositions);
    positionKdtree->SetData(1,ypositions);
    positionKdtree->Build();
    positionKdtree->SetOwner(kTRUE);
    
}

  // Function to get back all clusters within a given spatial window
  Timepix1Clusters getAllClustersInWindow(Timepix1Cluster* cluster, double window){
    
    // Get iterators of all clusters within the time window
    std::vector<int> results;
    double* position;
    position[0] = cluster->globalX();
    position[1] = cluster->globalY();
    positionKdtree->FindInRange(position,window,results);
    
    // Turn this into a vector of clusters
    Timepix1Clusters resultClusters;
    for(int res=0;res<results.size();res++) resultClusters.push_back(clusters[results[res]]);
    
    // Return the vector of clusters
    return resultClusters;
  }
  
  // Function to get back all clusters within a given time period
  Timepix1Cluster* getClosestNeighbour(Timepix1Cluster* cluster){
    
    // Get the closest cluster to this one
    int result;
    double distance;
    double position[2];
    position[0] = cluster->globalX();
    position[1] = cluster->globalY();
    positionKdtree->FindNearestNeighbors(position,1,&result,&distance);
    
    // Return the cluster
    return clusters[result];
  }
  
  
  // Member variables
  double* xpositions; //!
  double* ypositions; //!
  double* times; //!
  TKDTreeID* positionKdtree;
  TKDTreeID* timeKdtree;
  Timepix1Clusters clusters;
  std::map<Timepix1Cluster*,int> iteratorNumber;
  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(KDTreeTimepix1,1)

};

#endif // KDTREE_TIMEPIX1_H
