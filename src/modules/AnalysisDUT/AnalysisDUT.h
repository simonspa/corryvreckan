#ifndef CORRYVRECKAN_DUT_ANALYSIS_H
#define CORRYVRECKAN_DUT_ANALYSIS_H

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <iostream>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class AnalysisDUT : public Module {

    public:
        // Constructors and destructors
        AnalysisDUT(Configuration config, std::shared_ptr<Detector> detector);
        ~AnalysisDUT() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;

        // Histograms
        TH2F *hClusterMapAssoc, *hHitMapAssoc, *hHitMapROI;
        TProfile2D* hClusterSizeMapAssoc;

        TH1F* hPixelRawValueAssoc;
        TProfile2D* hPixelRawValueMapAssoc;

        TH1F* associatedTracksVersusTime;
        TH1F *residualsX, *residualsY;

        TH1F *residualsX1pix, *residualsY1pix;
        TH1F *residualsX2pix, *residualsY2pix;

        TH1F* clusterChargeAssoc;
        TH1F* clusterSizeAssoc;
        TH1F* clusterSizeAssocNorm;

        TProfile2D *rmsxvsxmym, *rmsyvsxmym, *rmsxyvsxmym;
        TProfile2D *qvsxmym, *qMoyalvsxmym, *pxqvsxmym;
        TProfile2D* npxvsxmym;
        TH2F *npx1vsxmym, *npx2vsxmym, *npx3vsxmym, *npx4vsxmym;

        TProfile2D* hPixelEfficiencyMap;
        TProfile2D* hChipEfficiencyMap;
        TProfile2D* hGlobalEfficiencyMap;

        TH1F* hTrackCorrelationX;
        TH1F* hTrackCorrelationY;
        TH1F* hTrackCorrelationTime;
        TH1F* residualsTime;
        TH2F* residualsTimeVsTime;
        TH2F* residualsTimeVsSignal;

        TH2F* hAssociatedTracksGlobalPosition;
        TH2F* hAssociatedTracksLocalPosition;

        // Member variables
        double spatialCut, m_timeCutFrameEdge;
        double chi2ndofCut;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DUT_ANALYSIS_H
