#ifndef Timepix1Correlator_H
#define Timepix1Correlator_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"

class Timepix1Correlator : public Algorithm {
  
public:
  // Constructors and destructors
  Timepix1Correlator(bool);
  ~Timepix1Correlator(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Histograms for several devices
  map<string, TH2F*> hitmaps;
  map<string, TH2F*> hitmapsGlobal;
  map<string, TH1F*> clusterSize;
  map<string, TH1F*> clustersPerEvent;
  map<string, TH1F*> correlationPlotsX;
  map<string, TH1F*> correlationPlotsY;
  
  // Member variables
  int m_eventNumber;
  
};

#endif // Timepix1Correlator_H
