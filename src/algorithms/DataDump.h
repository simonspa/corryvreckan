#ifndef DataDump_H
#define DataDump_H 1

#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "objects/Pixel.h"
#include "objects/Cluster.h"
#include "objects/Track.h"
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <stdint.h>

class DataDump : public Algorithm {

public:
  // Constructors and destructors
  DataDump(bool);
  ~DataDump(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Member variables
  int m_eventNumber;
  string m_detector;

};

#endif // DataDump_H
