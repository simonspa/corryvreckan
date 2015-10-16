#ifndef GUI_H
#define GUI_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TCanvas.h"
#include "TBrowser.h"
#include "TThread.h"
#include "TApplication.h"

#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

class GUI : public Algorithm {
  
public:
  // Constructors and destructors
  GUI(bool);
  ~GUI(){}

  // Functions
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  
  // Canvases to display plots
  TCanvas* trackCanvas;
  TCanvas* hitmapCanvas;
  TCanvas* globalHitmapCanvas;
  TCanvas* residualsCanvas;
  
  // Plot holders
  map<string, TH2F*> hitmap;
  map<string, TH2F*> globalHitmap;
  map<string, TH1F*> residuals;
  
  // Thread to allow display to run in separate thread
  TThread* displayThread;
  
  // Misc. member objects
  int nDetectors;
  int eventNumber;
  int updateNumber;
//  TBrowser* browser;
  
};

#endif // GUI_H
