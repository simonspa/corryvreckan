#ifndef OnlineMonitor_H
#define OnlineMonitor_H 1

#include "core/algorithm/Algorithm.h"
#include <iostream>
#include "objects/Pixel.h"
#include "objects/Cluster.h"
#include "objects/Track.h"
#include "objects/GuiDisplay.h"

// ROOT includes
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TGCanvas.h"
#include "TGFrame.h"
#include "TGTextEntry.h"
#include "TGDockableFrame.h"
#include "TGMenu.h"
#include "TApplication.h"
#include <RQ_OBJECT.h>
#include "TROOT.h"
#include "TSystem.h"
#include "TRootEmbeddedCanvas.h"

class OnlineMonitor : public Algorithm {

public:
  // Constructors and destructors
  OnlineMonitor(bool);
  ~OnlineMonitor(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Application to allow display persistancy
  TApplication* app;
  GuiDisplay* gui;

  void AddHisto(string, string, string style="");
  void AddButton(string, string);

  // Member variables
  int eventNumber;
  int updateNumber;

};

#endif // OnlineMonitor_H
