#ifndef VETRANOISE_H 
#define VETRANOISE_H 1


#include <vector>

// Include files

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "VetraMapping.h"


/** @class VetraNoise VetraNoise.h
 *  
 *
 *  @author Pablo Rodriguez
 *  @date   08-06-2010
 */

class VetraNoise : public TestBeamObject {
public: 
  /// Standard constructor
  VetraNoise();

  void readNoise(std::string);

  static float getNoiseFromTell1Ch[2048];
  static float getNoiseFromStrip[2048];

  //  VetraNoise(Parameters*,bool); 
  virtual ~VetraNoise( ); ///< Destructor

  
protected:

private:

};


#endif // VETRANOISE_H
