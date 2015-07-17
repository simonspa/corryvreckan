#ifndef NEWVETRACLUSTERMAKER_H 
#define NEWVETRACLUSTERMAKER_H 1

// Include files
#include <vector>

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamTransform.h"
#include "VetraCluster.h"
#include "VetraMapping.h"
#include "VetraNoise.h"
#include "TTree.h"

/** @class NewVetraClusterMaker NewVetraClusterMaker.h
 *  
 *
 *  @author Pablo Rodriguez
 *  @date   2010-06-02
 */

struct branchObjects{
  int eventIdx;
  int clusterSize;
  int stripsInCluster[4];
  int tell1ChInCluster[4];
  int adcsInCluster[4];
  float clusterResiduals;
  float distTrackToStripCenter;
  float etaDistribution;
  float weightedEtaDistribution;
  float track_x;
  float track_y;
  float cluster_x;
} ;

class NewVetraClusterMaker : public Algorithm{
 public: 
  /// Standard constructor
  NewVetraClusterMaker(); 
  NewVetraClusterMaker(Parameters*,bool); 
  virtual ~NewVetraClusterMaker( ); ///< Destructor


  static int nValidTriggers;
  static int nTotalFrames;
  static int nFramesWithTDCinfo;
  static int nTotalTriggers;
  static int nTDCElements;
  static int nMatchedTDCElements;
  static int removeChannelsAffectedXTalk;

  static float globalAccumulatedCharge[200];
  static float globalAccumulatedEntries[200];
  static float accumulatedCharge[200];
  static float accumulatedEntries[200];
  static float globalAccumulatedChargeMod25[25];
  static float globalAccumulatedEntriesMod25[25];
  static float accumulatedChargeMod25[25];
  static float accumulatedEntriesMod25[25];
  static float globalChargePerSyncDelay[200];
  static float globalEntriesPerSyncDelay[200];
  static std::vector <float> accumulatedTDC1StripADC;
  static std::vector <float> accumulatedTDC1StripTDC;
  static VetraMapping vetraReorder;
  static VetraNoise vetraNoise;

  static std::vector <int> stripsStartRange;
  static std::vector <int> stripsEndRange;

  void initClusterCuts();
  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();

  void fillRejectedStripsArray();
  void defineHistograms();
  void removeBorderClusters(VetraCluster* );
  void removeSpecialClustersForAlignment(std::vector<VetraCluster*>* );
  void fillEtaDistribution(std::vector<VetraCluster*>* );

  int getClusterSeedCut(){return clusterSeedCut;}
  int getClusterInclusionCut(){return clusterInclusionCut;}
  
 protected:

 private:

  TestBeamTransform* m_transform;
  PositionVector3D< Cartesian3D<double> > m_planePointLocalCoords;
  PositionVector3D< Cartesian3D<double> > m_planePointGlobalCoords;
  PositionVector3D< Cartesian3D<double> > m_planePoint2LocalCoords;
  PositionVector3D< Cartesian3D<double> > m_planePoint2GlobalCoords;

  int eventIndex;

  float m_dut_normal_x;
  float m_dut_normal_y;
  float m_dut_normal_z;

  float efficiency_trackNr;
  float efficiency_clusterNr_1000um;
  float efficiency_clusterNr_500um;
  float efficiency_clusterNr_200um;

  int ratioCounter_m2;
  int ratioCounter_m1;
  int ratioCounter_p1;
  int ratioCounter_p2;
  int ratioCounter_m2_15;
  int ratioCounter_m1_15;
  int ratioCounter_p1_15;
  int ratioCounter_p2_15;
  int ratioCounter_m2_30;
  int ratioCounter_m1_30;
  int ratioCounter_p1_30;
  int ratioCounter_p2_30;

  string etaDistributionFile;

  std::vector<int> rejectedStrips;

  bool display;
  Parameters *parameters;

  float clusterSeedCut;
  float clusterInclusionCut;
  float clusterTotalChargeCut;

  float syncDelayLowCutRegion;
  float syncDelayHighCutRegion;

  int timeWindowVetraTrack; //nanosecs
  float spatialWindowVetraTrack; //nanosecs

  int stripWindowLowCut;
  int stripWindowHighCut;

  bool m_debug;

  TH2F* initialAdcVsStrip;

  TH2F* telescopesHitmap;
  TH2F* telescopesHitmapTimeMatchedWithTrigger;
  TH2F* telescopesHitmapTimeMatchedWithVetraCluster;
  TH2F* telescopesHitmapTimeAndSpatialMatchedWithVetraCluster;

  TH1F* residualsVetraPositionTrackX;
  TH1F* residualsClusterSize1;
  TH1F* residualsClusterSize2;

  TH1F* diffTdcTrackTdcVetraH;
  TH1F* diffToaTrackTdcTrackH;
  TH1F* validTracksInFrameH;
  TH1F* onlyToaTracksInFrameH;
  TH1F* emptyTracksInFrameH;
  TH1F* totalTracksInFrameH;
  TH1F* vetraClustersInFrameH;
  TH1F* triggersInFrameH;
  TH1F* nTDCElementsH;

  TH1F* vetraHitsPerTriggerH;
  TH1F* vetraChannelH;
  TH1F* vetraStripH;

  TH1F* correlationDUTTrack;

  TH1F* vetraADCspectrumH;
  TH1F* vetraPositionClusterH;
  TH1F* vetraPositionCluster1H;
  TH1F* vetraPositionCluster2H;
  TH1F* vetraPositionMatchedClusterH;
  TH1F* vetraPositionMatchedCluster1H;
  TH1F* vetraPositionMatchedCluster2H;
  TH1F* vetraClusterSizeH;
  TH1F* vetraClusterSeedH;
  TH1F* tell1ChNoiseDistributionH;
  TH1F* sensorNoiseDistributionH;
  TH1F* sensorNoiseResiduals;
  TH1F* numberClustersPerFrameH;
  TH1F* landauBeforeClustering;
  TH1F* landauAfterClustering;
  TH1F* landauAfterClustering1;
  TH1F* landauAfterClustering2;
  TH1F* landauAfterClustering3;
  TH1F* landauAfterClustering4;
  TH1F* landauBeforeClusteringWithoutCut;
  TH1F* landauAfterClusteringWithoutCut;

  TH1F* syncDelayBeforeClustering;
  TH1F* syncDelayHitsAfterClustering;
  TH1F* syncDelayTriggerGlobal;
  TH1F* syncDelayTriggerAfterClustering;

  TH1F* globalChargeVsBeetleClock;
  TH1F* chargeVsBeetleClock;
  TH1F* globalChargeVsBeetleClockMod25;
  TH1F* chargeVsBeetleClockMod25;
  TH1F* globalChargeVsSyncDelay;

  TH1F** TDCsyncDelay1Strip;
  TH1F** TDCsyncDelay2Strip;

  TH1F** landauVsStripsFor1stripCluster;
  TH1F** landauVsStripsFor2stripCluster;

  TH1F* cluster1stripLowADC;
  TH1F* cluster1stripHighADC;

  TH1F* etaDistribution;
  TH2F* etaVsPosition;

  TH1F* adcAdjacentStrips;

  TH2F* averageChargeVsBeetleClock;
  TProfile* averageChargeVsBeetleClockProf;

  TH1F* vetraHitsInEvent;
  TH1F* vetraHitsInMatchedTriggerInEvent;
  TH1F* vetraSeedsInMatchedTriggerInEvent;
  TH1F* vetraClustersInEvent;
  TH1F* vetraMatchedClustersInEvent;
  TH1F* tracksInEvent;
  TH1F* validTracksInEvent;

  TH1F* checkImpulseADC_central;
  TH1F* checkImpulseADC_p1;
  TH1F* checkImpulseADC_p2;
  TH1F* checkImpulseADC_m1;
  TH1F* checkImpulseADC_m2;


  TH1F* checkImpulseCenter;
  TH1F* ratioAdcNeighbor_m2;
  TH1F* ratioAdcNeighbor_m1;
  TH1F* ratioAdcNeighbor_p1;
  TH1F* ratioAdcNeighbor_p2;

  TH1F* checkImpulseCenter_15;
  TH1F* ratioAdcNeighbor_m2_15;
  TH1F* ratioAdcNeighbor_m1_15;
  TH1F* ratioAdcNeighbor_p1_15;
  TH1F* ratioAdcNeighbor_p2_15;

  TH1F* checkImpulseCenter_30;
  TH1F* ratioAdcNeighbor_m2_30;
  TH1F* ratioAdcNeighbor_m1_30;
  TH1F* ratioAdcNeighbor_p1_30;
  TH1F* ratioAdcNeighbor_p2_30;

  TH2F* etaVsTrackx;
  TH2F* cogVsTrackx;

  TH2F* totalChargeVsTrackx;
  TH2F* chargeLeftVsTrackx;
  TH2F* chargeRightVsTrackx;
  TH2F* chargeLeftVsChargeRight_0_05;
  TH2F* chargeLeftVsChargeRight_05_1;

  TH1F* etaInverse;
  TH1F* tell1ConsecutiveCluster2;

  TH1F* efficiencyPlot;

  TTree* tbtree;
  branchObjects myBranch;

  TH1F* test;
};

#endif // NEWVETRACLUSTERMAKER_H
