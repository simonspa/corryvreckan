#ifndef NCLUSTERSMONITOR_H 
#define NCLUSTERSMONITOR_H 1

// Include files
#include <map>

#include "TH1.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"

/** @class NClustersMonitor NClustersMonitor.h
 *  
 *
 *
 */

class NClustersMonitor : public Algorithm {
public: 
  /// Constructors
  NClustersMonitor(); 
  NClustersMonitor(Parameters*, bool);
  // Destructor
  virtual ~NClustersMonitor(); 
  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();

private:
  std::map<std::string, int> numHits;
  std::map<std::string, int> numClusters;
  std::map<std::string, TH1F*> nHits;
  std::map<std::string, TH1F*> nClusters;
  std::map<std::string, TH1F*> clusterTimes;

  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
};
#endif // NCLUSTERSMONITOR_H
