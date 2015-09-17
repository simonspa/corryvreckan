// $Id: TrackTimestamper.h,v 1.1 2009-09-14 12:44:43 mcrossle Exp $
#ifndef TRACKTIMESTAMPER_H 
#define TRACKTIMESTAMPER_H 1

// Include files
#include <map>
#include <string>
#include "TH2.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"

/** @class TrackTimestamper TrackTimestamper.h
 *  
 *
 *  @author Hamish Gordon
 *  @date   2011-09-20
 */

class TrackTimestamper : public Algorithm {
public: 
  /// Constructors
  TrackTimestamper(); 
  TrackTimestamper(Parameters*, bool);
  /// Destructor 
  virtual ~TrackTimestamper(); 
  void initial();
  float getDelta(TestBeamTracks* track, TestBeamEvent* event, std::string dunt);
  void run(TestBeamEvent*,Clipboard*);
  void end();

private:
  int totalveto;
  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
  bool dofit;
  double centrewindow;
  int passcwin;
  int failcwin;
  int nframes;
  std::map<int, TH1F*> hMapFrameTimewalks;
  std::map<std::string, TH1F*> hTimewalk;
  std::map<std::string, TH1F*> hTimewalkTop;
  std::map<std::string, TH1F*> hTimewalkBottom;
  std::map<std::string, TH1F*> hTimewalkLeft;
  std::map<std::string, TH1F*> hTimewalkRight;
  std::map<std::string, TH1F*> hTimewalkBlack;
  std::map<std::string, TH1F*> hTimewalkWhite;
  std::map<std::string, TH1F*> hTimewalkTrack;
  std::map<std::string, TH2F*> hRowTimewalk;
  std::map<std::string, TH2F*> hColTimewalk;
  std::map<std::string, TH2F*> hCorrTdcTpx;
  std::map<std::string, TH2F*> hAssocTracks;
  std::map<std::string, TH2F*> hNonAssocTracks;
  std::map<std::string, TH2F*> hTdcToaOverlapAssoc;
  std::map<std::string, TH2F*> hTdcToaOverlapNonAssoc;
  std::map<std::string, TH1F*> hTimepixAssocToasBlack;
  std::map<std::string, TH1F*> hTimepixAssocToasWhite;
  std::map<std::string, TH1F*> hTdcToas;

  std::map<std::string, TH1F*> hTimeBetweenTriggersPostVeto;

  //TH1F* htimepixnonassoctoas;
  //TH1F* hrawtpxclustertoas;
	TH1F* hTdcToa;
  TH1F* hTpxToa;
  TH1F* hTimeBetweenTriggers;
  TH1F* hSyncDelay;
  TH1F* hTdcTriggers;
  TH1F* hTdcTrigPerFrame;
  
  TH1F* hTDCShutter;
  TH1F* hGood;
  TH1F* hZeropoint;
  TH1F* hRiseTime;
  TH1F* hMean;

  int nHists;
  std::map<int, TH1F*> tws;
  std::map<int, TH1F*> twsblack;
  std::map<int, TH1F*> twswhite;
  
  int nTracksAssocTotal;
  int nTracksNonAssocTotal;
  int nTriggersNonAssocTotal;
  int nGoodFrames;
  int nTracksAssocGood;
  int nTracksNonAssocGood;
  int nGoodClusterFrames;
  int totalTDCTriggers;
  int nlines;
  int nerror;
  int globalEfficiencyDenominator;
  int inOverlapEfficiencyDenominator;
  int globalEfficiencyNumerator;
  int inOverlapEfficiencyNumerator;
  float time;
  float clockperiod;
  int totalSaturated;
  int noTOAcluster;
  bool m_debug;
  int noTrackClusters;
  int screwedUpFrame;
  int totalSaturatedInOverlap;
  
};
#endif // TRACKTIMESTAMPER_H
