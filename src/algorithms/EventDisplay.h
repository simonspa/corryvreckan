#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H 1

#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TCanvas.h"
#include "objects/Timepix3Pixel.h"
#include "objects/Timepix3Cluster.h"
#include "objects/Timepix3Track.h"

class EventDisplay : public Algorithm {

public:
  // Constructors and destructors
  EventDisplay(bool);
  ~EventDisplay(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  TH3F* eventMap;

};

#endif // EVENTDISPLAY_H
