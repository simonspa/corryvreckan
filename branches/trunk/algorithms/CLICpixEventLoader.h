#ifndef CLICpixEventLoader_H
#define CLICpixEventLoader_H 1

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

class CLICpixEventLoader : public Algorithm {
  
public:
  // Constructors and destructors
  CLICpixEventLoader(bool);
  ~CLICpixEventLoader(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  // Member variables
  int m_eventNumber;
  string m_filename;
  ifstream m_file;
  
  TH2F* hHitMap;
  TH1F* hPixelToT;
  TH1F* hShutterLength;
  TH1F* hPixelsPerFrame;
  
};

#endif // CLICpixEventLoader_H
