#ifndef TRACKFITTER_H 
#define TRACKFITTER_H 1

// Include files
#include <map>
#include <vector>

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"

/** @class TrackFitter TrackFitter.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class TrackFitter : public Algorithm {
public: 
  /// Constructors
  TrackFitter(); 
  TrackFitter(Parameters*, bool);
  /// Destructor 
  virtual ~TrackFitter() {} 
  void initial();
  void run(TestBeamEvent*, Clipboard*);
	void getTracks(TestBeamClusters*, TestBeamProtoTracks*, std::vector<TestBeamTrack*>*&, int);
  void end();

private:

  TH1F* hTracksClusterCount;
  TH1F* hTrackSlopeX;
  TH1F* hTrackSlopeY;
  TH1F* hTrackChiSquared;
  TH1F* hTrackChiSquaredNdof;
  TH1F* hTrackProb;

  std::map<std::string, TH1F*> hResX;
  std::map<std::string, TH1F*> hResLocalX;
  std::map<std::string, TH1F*> hResY;
  std::map<std::string, TH1F*> hResLocalY;
  std::map<std::string, TH1F*> hResAssociatedX;
  std::map<std::string, TH1F*> hResAssociatedY;

  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
  bool m_debug;
  
};

#endif // TRACKFITTER_H
