// $Id: PulseShapeAnalyser.h,v 1.4 2009-07-13 15:11:11 mjohn Exp $
#ifndef PULSESHAPEANALYSER_H 
#define PULSESHAPEANALYSER_H 1

// Include files
#include <map>

#include "TH1.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"

/** @class PulseShapeAnalyser PulseShapeAnalyser.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class PulseShapeAnalyser : public Algorithm {
public: 
  /// Standard constructor
  PulseShapeAnalyser(); 
  PulseShapeAnalyser(Parameters*,bool); 
  virtual ~PulseShapeAnalyser( ); ///< Destructor
  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();
protected:

private:
  std::map< std::string, TH1F*> pulseShape;
  std::map< std::string, TH1F*> pulseShapeRaw;

  TestBeamDataSummary *summary;
  Parameters *parameters;
  bool display;
};
#endif // PULSESHAPEANALYSER_H
