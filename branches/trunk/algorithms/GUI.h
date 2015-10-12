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
  
//  void startBrowser();
  
  TApplication* rootapp;
  TThread* browserThread;
  TThread* updateThread;
  TBrowser* browser;
  TH3F* eventMap;
  
};

#endif // GUI_H
