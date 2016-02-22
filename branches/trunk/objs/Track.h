#ifndef TRACK_H
#define TRACK_H 1

#include "Cluster.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"

/*
 
 This class is a simple track class which knows how to fit itself. 
 It holds a collection of clusters, which may or may not be included
 in the track fit.
 
 */

class Track : public TestBeamObject {
  
public:
  
  // Constructors and destructors
  Track(){
    m_direction.SetZ(1.);
    m_state.SetZ(0.);
  }
  virtual ~Track(){}
  
  // Copy constructor (also copies clusters from the original track)
  Track(Track* track){
    Clusters trackClusters = track->clusters();
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Cluster* trackCluster = trackClusters[iTrackCluster];
      Cluster* cluster = new Cluster(trackCluster);
      m_trackClusters.push_back(cluster);
    }
    Clusters associatedClusters = track->associatedClusters();
    for(int iAssociatedCluster=0; iAssociatedCluster<associatedClusters.size(); iAssociatedCluster++){
      Cluster* associatedCluster = associatedClusters[iAssociatedCluster];
      Cluster* cluster = new Cluster(associatedCluster);
      m_associatedClusters.push_back(cluster);
    }
    m_state = track->m_state;
    m_direction = track->m_direction;
  }
  
  // -----------
  // Functions
  // -----------
  
  // Add a new cluster to the track
  void addCluster(Cluster* cluster){
    m_trackClusters.push_back(cluster);
  }
  // Add a new cluster to the track (which will not be in the fit)
  void addAssociatedCluster(Cluster* cluster){
    m_associatedClusters.push_back(cluster);
  }
  
  // Calculate the 2D distance^2 between the fitted track and a cluster
  double distance2(Cluster* cluster){
  	
    // Get the track X and Y at the cluster z position
    double trackX = m_state.X() + m_direction.X()*cluster->globalZ();
    double trackY = m_state.Y() + m_direction.Y()*cluster->globalZ();
    
    // Calculate the 1D residuals
    double dx = (trackX - cluster->globalX());
    double dy = (trackY - cluster->globalY());
    
    // Return the distance^2
    return (dx*dx + dy*dy);
  }
  
  // Calculate the chi2 of the track
  void calculateChi2(){
    
    // Get the number of clusters
    int nClusters = m_trackClusters.size();
    m_ndof = nClusters-2.;
    m_chi2 = 0.; m_chi2ndof = 0.;
    
    // Loop over all clusters
    for(int iCluster=0;iCluster<nClusters;iCluster++){
      
      // Get the cluster
      Cluster* cluster = m_trackClusters[iCluster];
      
      // Get the distance^2 and the error^2
      double error2 = cluster->error() * cluster->error();
      m_chi2+= (this->distance2(cluster)/error2);
    }
    
    // Store also the chi2/degrees of freedom
    m_chi2ndof = m_chi2/m_ndof;
  }
  
  // Minimisation operator used by Minuit. Minuit passes the current iteration of
  // the parameters and checks if the chi2 is better or worse
  double operator()(const double *parameters){
    
    // Update the track gradient and intercept
    this->m_direction.SetX(parameters[0]);
    this->m_state.SetX(parameters[1]);
    this->m_direction.SetY(parameters[2]);
    this->m_state.SetY(parameters[3]);

    // Calculate the chi2
    this->calculateChi2();
    
    // Return this to minuit
    return (const double)m_chi2;
    
  }
  
  // Retrieve track parameters
  double chi2(){return m_chi2;}
  double chi2ndof(){return m_chi2ndof;}
  double ndof(){return m_ndof;}
  long long int timestamp(){return m_timestamp;}
  Clusters clusters(){return m_trackClusters;}
  Clusters associatedClusters(){return m_associatedClusters;}
  int nClusters(){return m_trackClusters.size();}
  ROOT::Math::XYZPoint intercept(double z){
    ROOT::Math::XYZPoint point = m_state + m_direction*z;
    return point;
  }

	// Set track parameters
  void setTimestamp(long long int timestamp){m_timestamp=timestamp;}
  
  // Member variables
  Clusters m_trackClusters;
  Clusters m_associatedClusters;
  long long int m_timestamp;
  double m_chi2;
  double m_ndof;
  double m_chi2ndof;
  ROOT::Math::XYZPoint m_state;
  ROOT::Math::XYZVector m_direction;

  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(Track,1)

};

// Vector type declaration
typedef std::vector<Track*> Tracks;

#endif // TRACK_H
