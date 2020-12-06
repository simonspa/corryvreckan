/**
 * @file
 * @brief Definition of module AnalysisDUT
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

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
        AnalysisDUT(Configuration& config, std::shared_ptr<Detector> detector);
        ~AnalysisDUT() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;

        // Histograms
        TH2F *hClusterMapAssoc, *hHitMapAssoc, *hHitMapROI;
        TProfile2D *hClusterSizeMapAssoc, *hClusterChargeMapAssoc;

        TH1F* hPixelRawValueAssoc;
        TProfile2D* hPixelRawValueMapAssoc;

        TH1F* associatedTracksVersusTime;
        TH1F *residualsX, *residualsY, *residualsPos;
        TH2F* residualsPosVsresidualsTime;

        TH1F *residualsX1pix, *residualsY1pix;
        TH1F *residualsX2pix, *residualsY2pix;
        TH1F *residualsX3pix, *residualsY3pix;
        TH1F *residualsX4pix, *residualsY4pix;
        TH1F *residualsXatLeast5pix, *residualsYatLeast5pix;

        TH1F* clusterChargeAssoc;
        TH1F* clusterSizeAssoc;
        TH1F* clusterSizeAssocNorm;
        TH1F* clusterWidthRowAssoc;
        TH1F* clusterWidthColAssoc;

        TH1F* hCutHisto;

        TProfile2D *rmsxvsxmym, *rmsyvsxmym, *rmsxyvsxmym;
        TProfile2D *qvsxmym, *qMoyalvsxmym, *pxqvsxmym;
        TProfile2D* npxvsxmym;
        TH2F *npx1vsxmym, *npx2vsxmym, *npx3vsxmym, *npx4vsxmym;

        TH1F* hTrackCorrelationX;
        TH1F* hTrackCorrelationY;
        TH1F* hTrackCorrelationPos;
        TH2F* hTrackCorrelationPosVsCorrelationTime;
        TH1F* hTrackCorrelationTime;
        TH1F* hTrackZPosDUT;
        TH1F* residualsTime;
        TH2F* residualsTimeVsTot;
        TH2F* residualsTimeVsTime;
        TH2F* residualsTimeVsSignal;
        TH2F* hAssociatedTracksGlobalPosition;
        TH2F* hAssociatedTracksLocalPosition;
        TH2F* hUnassociatedTracksGlobalPosition;
        // Member variables
        double m_timeCutFrameEdge;
        double chi2ndofCut;
        bool useClosestCluster;
        int num_tracks;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DUT_ANALYSIS_H
