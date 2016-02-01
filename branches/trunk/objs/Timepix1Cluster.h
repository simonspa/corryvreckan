#ifndef TIMEPIX1CLUSTER_H
#define TIMEPIX1CLUSTER_H 1

#include "Timepix1Pixel.h"
#include <iostream>

class Timepix1Cluster : public TestBeamObject {
  
public:
  
  // Constructors and destructors
  Timepix1Cluster(){}
  virtual ~Timepix1Cluster(){}
  // Copy constructor
  Timepix1Cluster(Timepix1Cluster* cluster){
    m_globalX = cluster->globalX();
    m_globalY = cluster->globalY();
    m_globalZ = cluster->globalZ();
    m_localX = cluster->localX();
    m_localY = cluster->localY();
    m_localZ = cluster->localZ();
    m_error = cluster->error();
    m_detectorID = cluster->detectorID();
  }

  // Functions
  // Add a new pixel to the cluster
  void addPixel(Timepix1Pixel* pixel){
    m_pixels.push_back(pixel);
  }
  // Retrieve cluster parameters
  double row(){return m_row;}
  double column(){return m_column;}
  double tot(){return m_tot;}
  double error(){return m_error;}
  double globalX(){return m_globalX;}
  double globalY(){return m_globalY;}
  double globalZ(){return m_globalZ;}
  double localX(){return m_localX;}
  double localY(){return m_localY;}
  double localZ(){return m_localZ;}
  int size(){return m_pixels.size();}
  std::string detectorID(){return m_detectorID;}
  Timepix1Pixels pixels(){return m_pixels;}

	// Set cluster parameters
  void setRow(double row){m_row = row;}
  void setColumn(double col){m_column = col;}
  void setTot(double tot){m_tot = tot;}
  void setClusterCentre(double x, double y, double z){
    m_globalX = x;
    m_globalY = y;
    m_globalZ = z;
  }
  void setClusterCentreLocal(double x, double y, double z){
    m_localX = x;
    m_localY = y;
    m_localZ = z;
  }
  void setError(double error){m_error=error;}
  void setDetectorID(std::string detectorID){m_detectorID=detectorID;}
  
  // Member variables
  Timepix1Pixels m_pixels;
  double m_row;
  double m_column;
  double m_tot;
  double m_error;
  double m_globalX;
  double m_globalY;
  double m_globalZ;
  double m_localX;
  double m_localY;
  double m_localZ;
  std::string m_detectorID;
  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(Timepix1Cluster,1)

};

// Vector type declaration
typedef std::vector<Timepix1Cluster*> Timepix1Clusters;

#endif // TIMEPIX1CLUSTER_H
