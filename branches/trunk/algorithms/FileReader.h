#ifndef FileReader_H
#define FileReader_H 1

#include "Algorithm.h"
#include <iostream>
#include "TTree.h"
#include "TFile.h"
#include "Timepix1Cluster.h"
#include "Timepix3Cluster.h"
#include "Timepix1Pixel.h"
#include "Timepix3Pixel.h"

class FileReader : public Algorithm {
  
public:
  // Constructors and destructors
  FileReader(bool);
  ~FileReader(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();
  
  // Member variables
  int m_eventNumber;
  string m_fileName;
  TFile* m_inputFile;

  // Flags for which data types to write out
  bool m_readClusters;
  bool m_readPixels;
  bool m_readTracks;
  bool m_onlyDUT;
  
  // Map of trees which holds the output objects
  map<string, TTree*> m_inputTrees;
  
  // Objects which the trees will point to (when
  // the branch address is set
  long long int m_time;
  map<string, TestBeamObject*> m_objects;
  
  // List of objects to write out
  vector<string> m_objectList;
  
  // Variables to keep track of time and file reading
  long long int m_currentTime;
  map<string, long long int> m_currentPosition;
  double m_timeWindow;
  
};

#endif // FileReader_H
