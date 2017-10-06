#ifndef Clicpix2Correlator_H
#define Clicpix2Correlator_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"
#include <sstream>

class Clicpix2Correlator : public Algorithm {
  
public:
  // Constructors and destructors
  Clicpix2Correlator(bool);
  ~Clicpix2Correlator(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Member variables
  int m_eventNumber;
  string dutID;
  map<int, Clusters> m_eventClusters;
  map<int, Tracks> m_eventTracks;
  double angleStart, angleStop, angleStep;
  
  // Histograms
  map<string,TH1F*> hTrackDiffX;
  map<string,TH1F*> hTrackDiffY;
  
};

#endif // Clicpix2Correlator_H
