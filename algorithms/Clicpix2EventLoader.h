#ifndef Clicpix2EventLoader_H
#define Clicpix2EventLoader_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

class Clicpix2EventLoader : public Algorithm {
  
public:
  // Constructors and destructors
  Clicpix2EventLoader(bool);
  ~Clicpix2EventLoader(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Histograms for several devices
  map<string, TH2F*> plotPerDevice;
  
  // Single histograms
  TH1F* singlePlot;
  
  // Member variables
  int m_eventNumber;
  string m_filename;
  ifstream m_file;
  
  TH2F* hHitMap;
  TH1F* hPixelToT;
  TH1F* hPixelsPerFrame;
  
};

#endif // Clicpix2EventLoader_H
