#ifndef GenericAlgorithm_H
#define GenericAlgorithm_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class GenericAlgorithm : public Module {

    public:
        // Constructors and destructors
        GenericAlgorithm(Configuration config, std::vector<Detector*> detectors);
        ~GenericAlgorithm() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        // Single histograms
        TH1F* singlePlot;

        // Member variables
        int m_eventNumber;
    };
} // namespace corryvreckan
#endif // GenericAlgorithm_H
