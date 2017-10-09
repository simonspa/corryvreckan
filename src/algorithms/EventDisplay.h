#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "core/Algorithm.h"
#include "objects/Timepix3Cluster.h"
#include "objects/Timepix3Pixel.h"
#include "objects/Timepix3Track.h"

namespace corryvreckan {
    class EventDisplay : public Algorithm {

    public:
        // Constructors and destructors
        EventDisplay(Configuration config, Clipboard* clipboard);
        ~EventDisplay() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        TH3F* eventMap;
    };
}
#endif // EVENTDISPLAY_H
