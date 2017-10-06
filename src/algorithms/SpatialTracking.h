#ifndef SpatialTracking_H
#define SpatialTracking_H 1

// Includes
#include <iostream>
// ROOT includes
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Minuit2/Minuit2Minimizer.h"
#include "Math/Functor.h"
// Local includes
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

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
