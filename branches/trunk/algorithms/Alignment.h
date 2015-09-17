#ifndef ALIGNMENT_H
#define ALIGNMENT_H 1

#include "Algorithm.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

// Fitting function for minuit
void SumDistance2(Int_t &, Double_t *, Double_t &, Double_t *, Int_t );

class Alignment : public Algorithm {
  
public:
  // Constructors and destructors
  Alignment(bool);
  ~Alignment(){}

  // Functions
  void initialise(Parameters*);
  int run(Clipboard*);
  void finalise();
  
  // Member variables
  Timepix3Tracks m_alignmenttracks;
  int nIterations;

};

#endif // ALIGNMENT_H
