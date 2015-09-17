#ifndef EVENTLOADER_H
#define EVENTLOADER_H 1

#include "Algorithm.h"
#include <iostream>
#include <fstream>

class EventLoader : public Algorithm {
  
public:
  // Constructors and destructors
  EventLoader(bool);
  ~EventLoader(){}

  //
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  void readHeader(string, string&, long int&);

  string m_inputDirectory;
  vector<string> m_files;
  int m_nFiles;
  int m_fileNumber;
  ifstream m_currentFile;
  
};

#endif // EVENTLOADER_H
