#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TCanvas.h"
#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

class EventDisplay : public Algorithm {
  
public:
  // Constructors and destructors
  EventDisplay(bool);
  ~EventDisplay(){}

  // Functions
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  
  TH3F* eventMap;
  
};

#endif // EVENTDISPLAY_H
