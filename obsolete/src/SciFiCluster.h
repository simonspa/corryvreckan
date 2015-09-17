#ifndef SCIFICLUSTER_H 
#define SCIFICLUSTER_H 1

// Include files
#include <vector>

#include "TestBeamEvent.h"
#include "TestBeamObject.h"

/** @class SciFiCluster SciFiCluster.h
 *  
 *
 */

class SciFiCluster : public TestBeamObject, public TObject  {
public: 
  /// Standard constructor
  SciFiCluster();
  SciFiCluster(SciFiCluster&,bool=COPY);
  virtual ~SciFiCluster( );
  void addHit(int,double);
  
  std::vector<int> getChannels() {return chans;}
  std::vector<double> getADCs(){return adcs;}
	double getADC(){return m_ADC;}
	int getClusterSize(){return clusterSize;}
	double getPosition(){return m_position;}
	void setPosition(double position){m_position=position;}
  
private:
  std::vector<int> chans;
  std::vector<double> adcs;
  int clusterSize;
	double m_ADC;
	double m_position;
  
public:
  //ClassDef(SciFiCluster,1);  //Event structure
};

inline SciFiCluster::SciFiCluster(SciFiCluster& c,bool action) : TestBeamObject(), TObject()
{
  if(action==COPY) return;
  chans = c.getChannels();
  adcs = c.getADCs();
  m_position = c.getPosition();
} 

typedef std::vector <SciFiCluster*> SciFiClusters;

#endif // SCIFICLUSTER_H
