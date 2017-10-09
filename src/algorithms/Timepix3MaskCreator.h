#ifndef TIMEPIX3MASKCREATOR_H
#define TIMEPIX3MASKCREATOR_H 1

#include "objects/Pixel.h"
#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"

namespace corryvreckan {
  class Timepix3MaskCreator : public Algorithm {

  public:
    // Constructors and destructors
    Timepix3MaskCreator(Configuration config, Clipboard* clipboard);
    ~Timepix3MaskCreator(){}

    // Functions
    void initialise(Parameters*);
    StatusCode run(Clipboard*);
    void finalise();

    // Member variables
    map<string, map<int, int> > pixelhits;

  };
}
#endif // TIMEPIX3MASKCREATOR_H
