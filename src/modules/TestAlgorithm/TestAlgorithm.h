#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"

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
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        std::shared_ptr<Detector> m_detector;

        // Pixel histograms
        TH2F* hitmap;
        TH2F* hitmap_clusters;
        TH1F* eventTimes;

        // Correlation plots
        TH1F* correlationX;
        TH1F* correlationXY;
        TH1F* correlationY;
        TH1F* correlationYX;
        TH2F* correlationX2Dlocal;
        TH2F* correlationY2Dlocal;
        TH2F* correlationColCol_px;
        TH2F* correlationColRow_px;
        TH2F* correlationRowCol_px;
        TH2F* correlationRowRow_px;
        TH2F* correlationX2D;
        TH2F* correlationY2D;
        TH1F* correlationTime;
        TH2F* correlationTimeOverTime;
        TH1F* correlationTime_px;
        TH1F* correlationTimeInt;

        // Parameters which can be set by user
        bool makeCorrelations;
        double timeCutFactor;
        double timeCut;
        bool doTimeCut;
        bool m_time_vs_time;
    };
} // namespace corryvreckan
#endif // TESTALGORITHM_H
