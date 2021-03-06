#ifndef CORRELATIONS_H
#define CORRELATIONS_H 1

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
    class Correlations : public Module {

    public:
        // Constructors and destructors
        Correlations(Configuration config, std::shared_ptr<Detector> detector);
        ~Correlations() {}

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
        TH2F* correlationXVsTime;
        TH2F* correlationYVsTime;

        // Parameters which can be set by user
        double timeCut;
        bool do_time_cut_;
        bool m_corr_vs_time;
    };
} // namespace corryvreckan
#endif // CORRELATIONS_H
