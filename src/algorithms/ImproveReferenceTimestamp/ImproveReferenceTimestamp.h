#ifndef ImproveReferenceTimestamp_H
#define ImproveReferenceTimestamp_H 1

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
    class ImproveReferenceTimestamp : public Algorithm {

    public:
        // Constructors and destructors
        ImproveReferenceTimestamp(Configuration config, std::vector<Detector*> detectors);
        ~ImproveReferenceTimestamp() {}

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
        bool m_stop;
        std::string m_source;
    };
} // namespace corryvreckan
#endif // ImproveReferenceTimestamp_H
