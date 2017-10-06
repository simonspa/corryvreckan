#ifndef GUI_H
#define GUI_H 1

#include "core/algorithm/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TCanvas.h"
#include "TBrowser.h"
#include "TThread.h"
#include "TApplication.h"

#include "objects/Pixel.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

class GUI : public Algorithm {

public:
  // Constructors and destructors
  GUI(bool);
  ~GUI(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Plot holders
  vector<TCanvas*> canvases;
  map<TCanvas*,vector<TH1*> > histograms;
  map<TH1*,string> styles;

  // Add plots and canvases
  void addPlot(TCanvas*, string, string style = "");
  void addCanvas(TCanvas*);

  // Application to allow display of canvases
  TApplication* app;

  // Misc. member objects
  int nDetectors;
  int eventNumber;
  int updateNumber;

};

#endif // GUI_H
