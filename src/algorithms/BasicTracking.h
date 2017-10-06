#ifndef BASICTRACKING_H
#define BASICTRACKING_H 1

#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "objects/Pixel.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

class BasicTracking : public Algorithm {

public:
  // Constructors and destructors
  BasicTracking(bool);
  ~BasicTracking(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

//  Cluster* getNearestCluster(Cluster*, map<Cluster*, bool>, Clusters*);
  Cluster* getNearestCluster(long long int, Clusters);

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
  double nTracksTotal;

};

#endif // BASICTRACKING_H
