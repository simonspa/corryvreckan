#ifndef ImproveTimestamp_H
#define ImproveTimestamp_H 1

#include <cmath>
#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class ImproveTimestamp : public Algorithm {

    public:
        // Constructors and destructors
        ImproveTimestamp(Configuration config, std::vector<Detector*> detectors);
        ~ImproveTimestamp() {}

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
        int m_method;
    };
} // namespace corryvreckan
#endif // ImproveTimestamp_H
