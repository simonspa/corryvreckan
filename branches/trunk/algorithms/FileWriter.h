#ifndef FileWriter_H
#define FileWriter_H 1

#include "Algorithm.h"
#include <iostream>
#include "TTree.h"
#include "TFile.h"
#include "Timepix1Cluster.h"
#include "Timepix3Cluster.h"
#include "Timepix1Pixel.h"
#include "Timepix3Pixel.h"

class FileWriter : public Algorithm {
  
public:
  // Constructors and destructors
  FileWriter(bool);
  ~FileWriter(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();
  
  // Member variables
  int m_eventNumber;
  string m_fileName;
  TFile* m_outputFile;
  
  // Flags for which data types to write out
  bool m_writeClusters;
  bool m_writePixels;
  bool m_writeTracks;
  bool m_onlyDUT;
  
  // Map of trees which holds the output objects
  map<string, TTree*> m_outputTrees;
  
  // Objects which the trees will point to (when
  // the branch address is set
  long long int m_time;
  map<string, TestBeamObject*> m_objects;
  
  // List of objects to write out
  vector<string> m_objectList;
  
};

#endif // FileWriter_H
