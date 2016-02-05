#ifndef SpatialTracking_H
#define SpatialTracking_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Cluster.h"
#include "Track.h"

class SpatialTracking : public Algorithm {
  
public:
  // Constructors and destructors
  SpatialTracking(bool);
  ~SpatialTracking(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Histograms
  TH1F* trackChi2;
  TH1F* clustersPerTrack;
  TH1F* trackChi2ndof;
  TH1F* tracksPerEvent;
  TH1F* trackAngleX;
  TH1F* trackAngleY;
  map<string,TH1F*> residualsX;
  map<string,TH1F*> residualsY;
  
  // Member variables
  int m_eventNumber;
  double spatialCut;
  int minHitsOnTrack;
  double nTracksTotal;
  
};

#endif // SpatialTracking_H
