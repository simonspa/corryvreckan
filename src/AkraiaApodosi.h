#ifndef AKRAIAAPODOSI_H
#define AKRAIAAPODOSI_H 1

// Include files
#include <map>
#include <vector>
#include <utility>

#include "TH1F.h"
#include "TH2F.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"
#include "TestBeamProtoTrack.h"

/** @class AkraiaApodosi.h
 *  
 *
 *  @author Panagiotis Tsopelas
 *  @date   20-08-2012
 */

class AkraiaApodosi : public Algorithm {
public: 
  /// Constructors
  AkraiaApodosi(); 
  AkraiaApodosi(Parameters*, bool); 
  /// Destructor
  virtual ~AkraiaApodosi() {}
  void initial();
  void run(TestBeamEvent*, Clipboard*);
  void end();
  
private:
 
  TH2F* hProtoInter;       
  TH2F* hAllCluster;       
  TH2F* hCutCluster;       
  TH1F* hProtoRatio;       
  TH1F* hProtoPerFrame; 
  TH1F* hClustPerFrame;  
  TH1F* hLocResTestAreaX; 
  TH1F* hLocResTestAreaY;  
  TH1F* hGloResTestAreaX;   
  TH1F* hGloResTestAreaY; 
  TH2F* hNonDetected; 

  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
  bool m_debug;

};

#endif // AKRAIAAPODOSI_H
