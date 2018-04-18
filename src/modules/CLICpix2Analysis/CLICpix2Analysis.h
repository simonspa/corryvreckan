#ifndef CORRYVRECKAN_CLICPIX2_ANALYSIS_H
#define CORRYVRECKAN_CLICPIX2_ANALYSIS_H

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile2D.h"
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class CLICpix2Analysis : public Module {

    public:
        // Constructors and destructors
        CLICpix2Analysis(Configuration config, std::vector<Detector*> detectors);
        ~CLICpix2Analysis() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);

    private:
        // Histograms
        TH2F *hClusterMapAssoc, *hHitMapAssoc;

        TH1F* hPixelToTAssoc;
        TProfile2D* hPixelToTMapAssoc;

        TH1F* associatedTracksVersusTime;
        TH1F *residualsX, *residualsY;

        TH1F *residualsX1pix, *residualsY1pix;
        TH1F *residualsX2pix, *residualsY2pix;

        TH1F* clusterTotAssoc;
        TH1F* clusterSizeAssoc;

        TH2F* hPixelEfficiencyMap;
        TH2F* hChipEfficiencyMap;
        TH2F* hGlobalEfficiencyMap;

        TH1F* hTrackCorrelationX;
        TH1F* hTrackCorrelationY;
        TH1F* hTrackCorrelationTime;
        TH1F* residualsTime;
        TH2F* residualsTimeVsTime;
        TH2F* residualsTimeVsSignal;

        TH2F* hAssociatedTracksGlobalPosition;
        TH2F* hUnassociatedTracksGlobalPosition;

        // Member variables
        std::string m_DUT;
        double spatialCut;
        double chi2ndofCut;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_CLICPIX2_ANALYSIS_H
