#ifndef GenericAlgorithm_H
#define GenericAlgorithm_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"

class GenericAlgorithm : public Algorithm {
  
public:
  // Constructors and destructors
  GenericAlgorithm(bool);
  ~GenericAlgorithm(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Histograms for several devices
  map<string, TH2F*> plotPerDevice;
  
  // Single histograms
  TH1F* singlePlot;
  
  // Member variables
  int m_eventNumber;
  
};

#endif // GenericAlgorithm_H
