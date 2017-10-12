#ifndef TIMEPIX3MASKCREATOR_H
#define TIMEPIX3MASKCREATOR_H 1

#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    class Timepix3MaskCreator : public Algorithm {

    public:
        // Constructors and destructors
        Timepix3MaskCreator(Configuration config, std::vector<Detector*> detectors);
        ~Timepix3MaskCreator() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        std::map<std::string, std::map<int, int>> pixelhits;
    };
}
#endif // TIMEPIX3MASKCREATOR_H
