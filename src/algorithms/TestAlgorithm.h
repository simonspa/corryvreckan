#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "objects/Pixel.h"
#include "objects/Cluster.h"

class TestAlgorithm : public Algorithm {

public:
  // Constructors and destructors
  TestAlgorithm(bool);
  ~TestAlgorithm(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Pixel histograms
  map<string, TH2F*> hitmap;
  map<string, TH1F*> eventTimes;

  // Cluster histograms
  map<string, TH1F*> clusterSize;
  map<string, TH1F*> clusterTot;
  map<string, TH2F*> clusterPositionGlobal;

  // Correlation plots
  map<string, TH1F*> correlationX;
  map<string, TH1F*> correlationY;
  map<string, TH1F*> correlationTime;
  map<string, TH1F*> correlationTimeInt;

  // Parameters which can be set by user
  bool makeCorrelations;
};

#endif // TESTALGORITHM_H
