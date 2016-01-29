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
  StatusCode run(Clipboard*);
  void finalise();
  
  // Histograms
  TH1F* tracksVersusTime;
  TH1F* associatedTracksVersusTime;
  TH1F* residualsX;
  TH1F* residualsY;
  TH1F* residualsTime;
  
  TH1F* tracksVersusPowerOnTime;
  TH1F* associatedTracksVersusPowerOnTime;
  
  // Member variables
  int m_eventNumber;
  int m_nAlignmentClusters;
//  bool m_powerOn;
  long long int m_powerOnTime;
  long long int m_powerOffTime;
  
  
};

#endif // DUTAnalysis_H
