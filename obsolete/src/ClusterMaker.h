#ifndef CLUSTERMAKER_H 
#define CLUSTERMAKER_H 1

// Include files
#include <map>
#include <vector>
#include <math.h>
#include <utility>

#include "TH2.h"
#include "TH1.h"

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamChessboardCluster.h"

/** @class ClusterMaker ClusterMaker.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class TimepixCalibration;

class ClusterMaker : public Algorithm {
public: 
  /// Constructors
  ClusterMaker(); 
  ClusterMaker(Parameters*, bool); 
  /// Destructor
  virtual ~ClusterMaker();
  void initial();
  void run(TestBeamEvent*, Clipboard*);
  void end();

private:

  int addHitsToCluster(TestBeamCluster*, bool);
  int addHitsToChessboardCluster(TestBeamChessboardCluster*, int);
  void setClusterCenter(TestBeamCluster*, TestBeamChessboardCluster*, bool);
  void setClusterGlobalPosition(TestBeamCluster*);
  void setClusterWidth(TestBeamCluster*);
  bool adjacent(RowColumnEntry*,RowColumnEntry*);
  bool isTOA(RowColumnEntry*, int);
  void etacorrection(const int corr, const double colwidth, const double rowwidth,
                     const double x, const double y,
                     double& xcorr, double& ycorr);
  int nlines;
  int m_reference;
  TestBeamDataSummary* summary;
  Parameters* parameters;
  std::map<std::string, TimepixCalibration*> calibration;

  bool display;
  bool m_writeClusterToTs;
  std::string m_clusterFile;
  // For the clustering we need a local copy of the hits  
  std::vector <std::pair < RowColumnEntry*,int > > m_tempData;
  
  // Histograms for monitoring
  std::map<std::string, TH1F*> hClusterSize;
  std::map<std::string, TH1F*> hClustersPerEvent;
  std::map<std::string, TH1F*> hClustersPerEventTHH;
  std::map<std::string, TH1F*> hPixelHitsPerEvent;
  std::map<std::string, TH2F*> hAdcWeightedHitPos;
  std::map<std::string, TH2F*> hHitPosition; 
  std::map<std::string, TH2F*> hLocalClusterPosition; 
  std::map<std::string, TH2F*> hGlobalClusterPosition; 
  std::map<std::string, TH2F*> hClusterWidth;
  std::map<std::string, TH1F*> hClusterWidthRatio; 
  std::map<std::string, TH1F*> hClusterWidthTwoFrac; 
  std::map<std::string, TH1F*> hSinglePixelTOTValues;
  std::map<std::string, TH1F*> hClusterTOTValues;
  std::map<std::string, TH1F*> hOnePixelClusterTOTValues;
  std::map<std::string, TH1F*> hTwoPixelClusterTOTValues;
  std::map<std::string, TH1F*> hThreePixelClusterTOTValues;
  std::map<std::string, TH1F*> hFourPixelClusterTOTValues;
  std::map<std::string, TH1F*> hThresholdValuePerEvent;
  int m_nZeroedHits;
  bool m_debug;
};

#endif // CLUSTERMAKER_H
