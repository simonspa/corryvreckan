#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class TestAlgorithm : public Module {

    public:
        // Constructors and destructors
        TestAlgorithm(Configuration config, std::shared_ptr<Detector> detector);
        ~TestAlgorithm() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);

    private:
        std::shared_ptr<Detector> m_detector;

        // Pixel histograms
        TH2F* hitmap;
        TH1F* eventTimes;

        // Correlation plots
        TH1F* correlationX;
        TH1F* correlationY;
        TH2F* correlationX2Dlocal;
        TH2F* correlationY2Dlocal;
        TH2F* correlationX2D;
        TH2F* correlationY2D;
        TH1F* correlationTime;
        TH1F* correlationTimeInt;

        // Parameters which can be set by user
        bool makeCorrelations;
        double timingCut;
        bool do_timing_cut_;

        // parameters
        double m_eventLength;
    };
} // namespace corryvreckan
#endif // TESTALGORITHM_H
