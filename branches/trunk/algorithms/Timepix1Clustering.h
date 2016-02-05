#ifndef Timepix1Clustering_H
#define Timepix1Clustering_H 1

#include "Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "Cluster.h"

class Timepix1Clustering : public Algorithm {
  
public:
  // Constructors and destructors
  Timepix1Clustering(bool);
  ~Timepix1Clustering(){}

  // Functions
  void initialise(Parameters*);
  StatusCode run(Clipboard*);
  void finalise();

  void calculateClusterCentre(Cluster*);

  // Member variables
  int m_eventNumber;
  
};

#endif // Timepix1Clustering_H
