// $Id: SummaryDisplayer.h,v 1.5 2009/08/13 17:06:13 mjohn Exp $
#ifndef SUMMARYDISPLAYER_H 
#define SUMMARYDISPLAYER_H 1

// Include files
#include "Parameters.h"
#include "TestBeamDataSummary.h"

/** @class SummaryDisplayer SummaryDisplayer.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-28
 */
class SummaryDisplayer {
public: 
  /// Standard constructor
  SummaryDisplayer( ){} 
  SummaryDisplayer(Parameters*); 
  void display();

  virtual ~SummaryDisplayer( ); ///< Destructor

protected:

private:
  Parameters *parameters;
  TestBeamDataSummary *summary;
  bool verbose;
};
#endif // SUMMARYDISPLAYER_H
