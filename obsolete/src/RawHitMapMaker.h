#ifndef RAWHITMAPMAKER_H 
#define RAWHITMAPMAKER_H 1

// Include files
#include <map>
#include <vector>

#include "TH3.h"
#include "TH2.h"
#include "TH1.h"

#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "Analysis.h"

/** @class RawHitMapMaker RawHitMapMaker.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class RawHitMapMaker : public Algorithm {
public: 
  /// Standard constructor
  RawHitMapMaker(); 
  RawHitMapMaker(Parameters*,bool); 
  /// Destructor
  virtual ~RawHitMapMaker( ); 
  void initial();
  void run(TestBeamEvent*, Clipboard*);
  void end();
  bool correlationRequested(std::string, std::string);
  void suppressNoisyPixels(TH2*, int);
  float colEff(float, float);
  float rowEff(float, float);

private:
  bool display;
  TestBeamDataSummary* summary;
  Parameters* parameters;
  std::map< std::string, TH2F*> hitmap;
  std::map< std::string, TH2F*> corr_hitmap;
  std::map< std::string, TH2F*> overlap_hitmap;
  std::map< std::string, std::map< std::string, TH2F*> > row_correlation;
  std::map< std::string, std::map< std::string, TH2F*> > col_correlation;
  std::map< std::string, std::map< std::string, TH1F*> > row_difference;
  std::map< std::string, std::map< std::string, TH1F*> > col_difference;
  std::map< std::string, double> alignrow;
  std::map< std::string, double> aligncol;
  std::map< std::string, double> dutalignrow;
  std::map< std::string, double> dutaligncol;

};
#endif
 
