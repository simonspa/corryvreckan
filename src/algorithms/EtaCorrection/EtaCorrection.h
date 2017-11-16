#ifndef EtaCorrection_H
#define EtaCorrection_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class EtaCorrection : public Algorithm {

    public:
        // Constructors and destructors
        EtaCorrection(Configuration config, std::vector<Detector*> detectors);
        ~EtaCorrection() {}

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
#endif // EtaCorrection_H
