#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include "Algorithm.h"
#include "Pixel.h"
#include "SpidrSignal.h"
#include <stdio.h>

class Timepix3EventLoader : public Algorithm {
  
public:
  // Constructors and destructors
  Timepix3EventLoader(bool);
  ~Timepix3EventLoader(){}

  // Standard algorithm functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();
  
  // Extra functions
  bool loadData(string, Pixels*, SpidrSignals*);
  void maskPixels(string, string);

  // Member variables
  string m_inputDirectory;
  map<string, vector<string> > m_datafiles;
  map<string, int> m_nFiles;
  map<string, int> m_fileNumber;
  map<string, long long int> m_syncTime;
  map<string, FILE*> m_currentFile;
  map<string, bool> m_clearedHeader;
  int m_minNumberOfPlanes;
  bool applyTimingCut;
  long long int m_currentTime;
  double m_timingCut;
  long long int m_prevTime;
  bool m_shutterOpen;
  
};

#endif // TIMEPIX3EVENTLOADER_H
