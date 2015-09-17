#ifndef ANALYSIS_H
#define ANALYSIS_H 1

#include <vector>
#include <map>

#include "TFile.h"
#include "TDirectory.h"

#include "Algorithm.h"
#include "Clipboard.h"
#include "Parameters.h"

//-------------------------------------------------------------------------------
// The analysis class is the core class which allows the event processing to
// run. It basically contains a vector of algorithms, each of which is initialised,
// run on each event and finalised. It does not define what an event is, merely
// runs each algorithm sequentially and passes the clipboard between them (erasing
// it at the end of each run sequence). When an algorithm returns a 0, the event
// processing will stop.
//-------------------------------------------------------------------------------

class Analysis {

public:
  
  // Constructors and destructors
  Analysis(Parameters*);
  virtual ~Analysis(){};

  // Member functions
  void add(Algorithm*);
  void run();
  void timing();
  void initialiseAll();
  void finaliseAll();

protected:
  
  // Member variables
  Parameters* m_parameters;
  Clipboard* m_clipboard;
  vector<Algorithm*> m_algorithms;
  TFile* m_histogramFile;
  TDirectory* m_directory;
  int m_events;
};
#endif // ANALYSIS_H
