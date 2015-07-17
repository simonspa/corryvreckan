// $Id: EventDisplay.h,v 1.4 2009-07-13 15:11:11 mjohn Exp $
#ifndef EVENTDISPLAY_H 
#define EVENTDISPLAY_H 1

// Include files
#include <map>
#include <vector>

#include "TH2.h"
#include "TCanvas.h"
#include "TGraph2D.h"
#include "TApplication.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"

/** @class EventDisplay EventDisplay.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */
class TestBeamCluster;

class EventDisplay : public Algorithm {
public: 
  /// Standard constructor
  EventDisplay(); 
  EventDisplay(Parameters*,TApplication* a); 
  virtual ~EventDisplay( ); ///< Destructor
  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();
protected:

private:
  TestBeamDataSummary *summary;
  Parameters *parameters;

  TApplication *app;
  TGraph2D* graph;
  TCanvas *cCC1;
  TH2F *hhh;
};
#endif // EVENTDISPLAY_H
