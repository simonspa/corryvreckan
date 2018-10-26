#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class TestAlgorithm : public Module {

    public:
        // Constructors and destructors
        TestAlgorithm(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~TestAlgorithm() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Pixel histograms
        std::map<std::string, TH2F*> hitmap;
        std::map<std::string, TH1F*> eventTimes;

        // Correlation plots
        std::map<std::string, TH1F*> correlationX;
        std::map<std::string, TH1F*> correlationY;
        std::map<std::string, TH2F*> correlationX2Dlocal;
        std::map<std::string, TH2F*> correlationY2Dlocal;
        std::map<std::string, TH2F*> correlationX2D;
        std::map<std::string, TH2F*> correlationY2D;
        std::map<std::string, TH1F*> correlationTime;
        std::map<std::string, TH1F*> correlationTimeInt;

        // Parameters which can be set by user
        bool makeCorrelations;
        double timingCut;

        // parameters
        double m_eventLength;
    };
} // namespace corryvreckan
#endif // TESTALGORITHM_H
