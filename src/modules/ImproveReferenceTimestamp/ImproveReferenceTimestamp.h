#ifndef ImproveReferenceTimestamp_H
#define ImproveReferenceTimestamp_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <cmath>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.h"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class ImproveReferenceTimestamp : public Module {

    public:
        // Constructors and destructors
        ImproveReferenceTimestamp(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~ImproveReferenceTimestamp() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
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
        double m_triggerLatency;
    };
} // namespace corryvreckan
#endif // ImproveReferenceTimestamp_H
