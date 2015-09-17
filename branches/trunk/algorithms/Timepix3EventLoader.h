#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include "Algorithm.h"
#include "Timepix3Pixel.h"
#include <stdio.h>

class Timepix3EventLoader : public Algorithm {
  
public:
  // Constructors and destructors
  Timepix3EventLoader(bool);
  ~Timepix3EventLoader(){}

  // Standard algorithm functions
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  
  // Extra functions
  bool loadData(string, Timepix3Pixels*);
  void maskPixels(string, string);

  // Member variables
  string m_inputDirectory;
  map<string, vector<string> > m_datafiles;
  map<string, int> m_nFiles;
  map<string, int> m_fileNumber;
  map<string, FILE*> m_currentFile;
  bool applyTimingCut;
  long long int m_currentTime;
  double m_timingCut;
  
};

#endif // TIMEPIX3EVENTLOADER_H
