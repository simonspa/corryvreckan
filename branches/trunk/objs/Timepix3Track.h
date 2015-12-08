#ifndef TIMEPIX3TRACK_H
#define TIMEPIX3TRACK_H 1

#include "Timepix3Cluster.h"
#include "TLinearFitter.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"

class Timepix3Track : public TestBeamObject {
  
public:
  
  // Constructors and destructors
  Timepix3Track(){
    m_fitterXZ = new TLinearFitter();
    m_fitterXZ->SetFormula("pol1");
    m_fitterYZ = new TLinearFitter();
    m_fitterYZ->SetFormula("pol1");
  }
  virtual ~Timepix3Track(){
    delete m_fitterXZ;
    delete m_fitterYZ;
  }
  
  // Copy constructor (also copies clusters from the original track)
  Timepix3Track(Timepix3Track* track){
    m_fitterXZ = new TLinearFitter();
    m_fitterXZ->SetFormula("pol1");
    m_fitterYZ = new TLinearFitter();
    m_fitterYZ->SetFormula("pol1");
    Timepix3Clusters trackClusters = track->clusters();
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Timepix3Cluster* trackCluster = trackClusters[iTrackCluster];
      Timepix3Cluster* cluster = new Timepix3Cluster(trackCluster);
      m_trackClusters.push_back(cluster);
    }
    Timepix3Clusters associatedClusters = track->associatedClusters();
    for(int iAssociatedCluster=0; iAssociatedCluster<associatedClusters.size(); iAssociatedCluster++){
      Timepix3Cluster* associatedCluster = associatedClusters[iAssociatedCluster];
      Timepix3Cluster* cluster = new Timepix3Cluster(associatedCluster);
      m_associatedClusters.push_back(cluster);
    }
    m_state = track->m_state;
    m_direction = track->m_direction;
  }
  
  // -----------
  // Functions
  // -----------
  
  // Add a new cluster to the track
  void addCluster(Timepix3Cluster* cluster){
    m_trackClusters.push_back(cluster);
  }
  void addAssociatedCluster(Timepix3Cluster* cluster){
    m_associatedClusters.push_back(cluster);
  }
  
  // Fit the track
  void fit(){

    m_fitterXZ->ClearPoints();
    m_fitterYZ->ClearPoints();
    
    // Check how many clusters there are
    int nClusters = m_trackClusters.size();
    
    // Loop over all clusters
    double* z = new double[0];
    for(int iCluster=0;iCluster<nClusters;iCluster++){
      // Get the cluster
      Timepix3Cluster* cluster = m_trackClusters[iCluster];
			// Add it to the fitter
      z[0] = cluster->globalZ();
      m_fitterXZ->AddPoint(z,cluster->globalX(),cluster->error());
      m_fitterYZ->AddPoint(z,cluster->globalY(),cluster->error());
    }

    // Fit the two lines
    m_fitterXZ->Eval();
    m_fitterYZ->Eval();
    
    // Get the slope and intercept
    double interceptXZ = m_fitterXZ->GetParameter(0);
    double slopeXZ = m_fitterXZ->GetParameter(1);
    double interceptYZ = m_fitterYZ->GetParameter(0);
    double slopeYZ = m_fitterYZ->GetParameter(1);
    
    // Set the track state and direction
    m_state.SetX(interceptXZ);
    m_state.SetY(interceptYZ);
    m_state.SetZ(0.);

    m_direction.SetX(slopeXZ);
    m_direction.SetY(slopeYZ);
    m_direction.SetZ(1.);
    
    delete z;
    this->calculateChi2();
    
  }
  
  // Calculate the 2D distance^2 between the fitted track and a cluster
  double distance2(Timepix3Cluster* cluster){
  	
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
      Timepix3Cluster* cluster = m_trackClusters[iCluster];
      
      // Get the distance^2 and the error^2
      double error2 = cluster->error() * cluster->error();
      m_chi2+= (this->distance2(cluster)/error2);
    }
    
    // Store also the chi2/degrees of freedom
    m_chi2ndof = m_chi2/m_ndof;
  }
  
  // Retrieve track parameters
  double chi2(){return m_chi2;}
  double chi2ndof(){return m_chi2ndof;}
  double ndof(){return m_ndof;}
  long long int timestamp(){return m_timestamp;}
  Timepix3Clusters clusters(){return m_trackClusters;}
  Timepix3Clusters associatedClusters(){return m_associatedClusters;}
  int nClusters(){return m_trackClusters.size();}
  ROOT::Math::XYZPoint intercept(double z){
    ROOT::Math::XYZPoint point = m_state + m_direction*z;
    return point;
  }

	// Set track parameters
  void setTimestamp(long long int timestamp){m_timestamp=timestamp;}
  
  // Member variables
  Timepix3Clusters m_trackClusters;
  Timepix3Clusters m_associatedClusters;
  long long int m_timestamp;
  double m_chi2;
  double m_ndof;
  double m_chi2ndof;
  TLinearFitter* m_fitterXZ;
  TLinearFitter* m_fitterYZ;
  ROOT::Math::XYZPoint m_state;
  ROOT::Math::XYZVector m_direction;

  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(Timepix3Track,1)

};

// Vector type declaration
typedef std::vector<Timepix3Track*> Timepix3Tracks;

#endif // TIMEPIX3TRACK_H
