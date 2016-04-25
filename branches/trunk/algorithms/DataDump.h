#ifndef DataDump_H
#define DataDump_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"
#include <dirent.h>
#include <sstream>
#include <fstream>

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
