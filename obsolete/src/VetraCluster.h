#ifndef VETRACLUSTER_H 
#define VETRACLUSTER_H 1


#include <vector>

// Include files
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamObject.h"
#include "Parameters.h"

/** @class VetraCluster VetraCluster.h
 *  
 *
 *  @author Pablo Rodriguez
 *  @date   08-06-2010
 */

class VetraCluster : public TestBeamObject, public TObject  {
 public: 
  /// Standard constructor
  VetraCluster();
  VetraCluster(VetraCluster&,bool=COPY);

  virtual ~VetraCluster(); ///< Destructor

  void clear();
  void addStripToVetraCluster(int str, int ad, double pos);
  void addStripToVetraCluster(int str, int ad, int wad, double pos);

  std::vector <int> getStripsVetraCluster() {return m_strips;}
  std::vector <int> getADCVetraCluster(){return m_adc;}
  std::vector <double> getStripPosition(){return m_stripPosition;}
  void setTimestamp(double , float );
  void setVetraTimeBeforeShutterClose(double vetraTime){m_vetraTimeBeforeShutterClose=vetraTime;}
  void setTrackPosition(double position){m_trackPosition=position;}
  void setTriggerVetraCluster(int trigger){m_triggerNr=trigger;}
  void setStripsVetraCluster(std::vector <int> str){m_strips=str;}
  void setStripPosition(std::vector <double> pos){m_stripPosition=pos;}
  void setADCVetraCluster(std::vector <int> adcs){m_adc=adcs;}
  void setSyncDelay(int sync) {m_syncDelay=sync;}
  void setEtaDistributionFile(std::string file) {m_etaDistributionFile=file;}

  int getTriggerVetraCluster() {return m_triggerNr;}
  int getSumAdcCluster();
  float getCogCluster();
  float getEtaVariable();
  float getEtaInverse();
  float getWeightedEtaVariable();
  float getVetraClusterPosition();
  double getFrameTimestamp() {return m_frameTimeStamp;}
  float getTimeAfterShutterOpen() {return m_timeAfterShutterOpen;}
  double getVetraTimeBeforeShutterClose() { return m_vetraTimeBeforeShutterClose;}
  double getTrackPosition() {return m_trackPosition;}
  int getClusterSize();
  int getSyncDelay() {return m_syncDelay;}
  std::string getEtaDistributionFile() {return m_etaDistributionFile;}
  bool is40microns();
  bool is60microns();

 protected:

 private:

  std::vector <int> m_strips;
  std::vector <int> m_adc;
  std::vector <int> m_wadc;
  std::vector <double> m_stripPosition;
  int m_triggerNr;
  double m_frameTimeStamp;
  float m_timeAfterShutterOpen;
  double m_vetraTimeBeforeShutterClose;
  int m_syncDelay; //delay between the asynchronous and the synchronous trigger. The sinchronous
  //trigger is sinchronous with the Beetle's 40MHz clock
  double m_trackPosition;
  int m_clusterSize;
  std::string m_etaDistributionFile;

  /* ClassDef(VetraCluster,1) */
};

inline VetraCluster::VetraCluster(VetraCluster& c,bool action) : TestBeamObject(), TObject()
{
  if(action==COPY) return;

  m_strips = c.getStripsVetraCluster();
  m_adc = c.getADCVetraCluster();
  m_stripPosition = c.getStripPosition();
  m_triggerNr = c.getTriggerVetraCluster();
  m_frameTimeStamp = c.getFrameTimestamp();
  m_timeAfterShutterOpen=c.getTimeAfterShutterOpen();
  m_syncDelay=c.getSyncDelay();
  m_etaDistributionFile=c.getEtaDistributionFile();
} 

typedef std::vector <VetraCluster*> VetraClusters;

#endif // VETRACLUSTER_H
