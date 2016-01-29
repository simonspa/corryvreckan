#ifndef BASICTRACKING_H
#define BASICTRACKING_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

class BasicTracking : public Algorithm {
  
public:
  // Constructors and destructors
  BasicTracking(bool);
  ~BasicTracking(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();
  
//  Timepix3Cluster* getNearestCluster(Timepix3Cluster*, map<Timepix3Cluster*, bool>, Timepix3Clusters*);
  Timepix3Cluster* getNearestCluster(long long int, Timepix3Clusters);

  // Member variables
  
  // Histograms
  TH1F* trackChi2;
  TH1F* clustersPerTrack;
  TH1F* trackChi2ndof;
  TH1F* tracksPerEvent;
  TH1F* trackAngleX;
  TH1F* trackAngleY;
  map<string,TH1F*> residualsX;
  map<string,TH1F*> residualsY;
  
  // Cuts for tracking
  double timingCut;
  double spatialCut;
  int minHitsOnTrack;
  
};

#endif // BASICTRACKING_H
