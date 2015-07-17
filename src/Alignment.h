// $Id: Alignment.h,v 1.11 2010/07/27 12:23:10 paula Exp $
#ifndef ALIGNMENT_H 
#define ALIGNMENT_H 1

// Include files
#include <vector>

#include "TH1.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"
#include "TestBeamTransform.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamCluster.h"
#include "TMinuit.h"
#include "TVirtualFitter.h"

/** @class Alignment Alignment.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

//The fcn function is what Minuit uses to minimize the chi2 of the alignment
//It must be declared as "static void" outside the class, in fact exactly in
//this way. 
void fcn(Int_t&, Double_t*, Double_t&, Double_t*, Int_t);

class Alignment : public Algorithm {
public: 
  /// Constructors
  Alignment(); 
  Alignment(Parameters*, bool);
  /// Destructor
  virtual ~Alignment( ); 
  void initial();
  void run(TestBeamEvent*, Clipboard*);
  void end();
  std::vector<TestBeamTrack*>* getTrackStore(){return trackStore;} 

private:
  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
  std::string m_outputfile;

  std::vector<TestBeamTrack*>* trackStoreAngled;
  std::vector<TestBeamTrack*>* trackStore;
  std::vector<TestBeamTrack*>* vetratrackStore;
  std::vector<TestBeamProtoTrack*>* prototrackStore;

  std::map<std::string,TH1F*> hResiduals;

  bool m_debug;

};
#endif // ALIGNMENT_H
