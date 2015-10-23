#ifndef DUTAnalysis_H
#define DUTAnalysis_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"

class DUTAnalysis : public Algorithm {
  
public:
  // Constructors and destructors
  DUTAnalysis(bool);
  ~DUTAnalysis(){}

  // Functions
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  
  // Histograms
  TH1F* tracksVersusTime;
  TH1F* associatedTracksVersusTime;
  TH1F* residualsX;
  TH1F* residualsY;
  TH1F* residualsTime;
  
  // Member variables
  int m_eventNumber;
  int m_nAlignmentClusters;
  
};

#endif // DUTAnalysis_H
