#ifndef TIMEPIX3CLUSTERING_H
#define TIMEPIX3CLUSTERING_H 1

#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"

class Timepix3Clustering : public Algorithm {
  
public:
  // Constructors and destructors
  Timepix3Clustering(bool);
  ~Timepix3Clustering(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();
  void calculateClusterCentre(Timepix3Cluster*);
  bool touching(Timepix3Pixel*,Timepix3Cluster*);
  bool closeInTime(Timepix3Pixel*,Timepix3Cluster*);
  
  double timingCut;
  long long int timingCutInt;

};

#endif // TIMEPIX3CLUSTERING_H
