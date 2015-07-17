#ifndef VETRAMAPPING_H 
#define VETRAMAPPING_H 1


#include <vector>

// Include files

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"

/** @class VetraMapping VetraMapping.h
 *  
 *
 *  @author Pablo Rodriguez
 *  @date   08-06-2010
 */

class VetraMapping : public TestBeamObject {
public: 
  /// Standard constructor
  VetraMapping();

  //  VetraMapping(Parameters*,bool); 
  virtual ~VetraMapping( ); ///< Destructor

  static int Tell1chToStrip[2048];
  static int StripToTell1ch[2048];
  static int StripToRegion[2048];
  static double StripToPosition[2048];
  static int RoutingLineToStrip[2048];

  void createMapping(Parameters*);

  int getStripFromTell1ch(int t1ch){return Tell1chToStrip[t1ch];}
  int getTell1chFromStrip(int str){return StripToTell1ch[str];}
  int getRegionFromStrip(int str){return StripToRegion[str];}
  float getPositionFromStrip(int str){return StripToPosition[str];}
  int getStripFromRoutingLine(int rl){return RoutingLineToStrip[rl];}

protected:

private:
  Parameters *parameters;
  void ChannelToStrip(int T1ch, bool reorder, int* routingLine, int* strip, int* region, float* position);
};


#endif // VETRAMAPPING_H
