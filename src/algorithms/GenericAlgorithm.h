#ifndef GenericAlgorithm_H
#define GenericAlgorithm_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class GenericAlgorithm : public Algorithm {

    public:
        // Constructors and destructors
        GenericAlgorithm(Configuration config, Clipboard* clipboard);
        ~GenericAlgorithm() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        // Single histograms
        TH1F* singlePlot;

        // Member variables
        int m_eventNumber;
    };
}
#endif // GenericAlgorithm_H
