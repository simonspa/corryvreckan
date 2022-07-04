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
#include <TDirectory.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
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

    protected:
        virtual void fillClusterHistograms(const std::shared_ptr<Cluster>& cluster);
        virtual bool acceptTrackDUT(const std::shared_ptr<Track>& track);

    public:
        enum ETrackSelection {
            kAllTrack,
            kHighChi2Ndf,
            kOutsideDUT,
            kOutsideROI,
            kCloseToMask,
            kTimeLimit,
            kPass,
            kAssociated,
            kNSelection
        };

    private:
        std::shared_ptr<Detector> m_detector;

        // Histograms
        TH1F *trackCorrelationX_beforeCuts, *trackCorrelationY_beforeCuts, *trackCorrelationTime_beforeCuts;
        TH1F *trackCorrelationX_afterCuts, *trackCorrelationY_afterCuts, *trackCorrelationTime_afterCuts;

        TH2F *hClusterMapAssoc, *hHitMapAssoc;
        TProfile2D *hClusterSizeMapAssoc, *hClusterChargeMapAssoc;
        TProfile *hClusterSizeVsColAssoc, *hClusterSizeVsRowAssoc;
        TProfile *hClusterWidthColVsRowAssoc, *hClusterWidthRowVsRowAssoc;
        TProfile *hClusterChargeVsColAssoc, *hClusterChargeVsRowAssoc;
        TProfile *hSeedChargeVsColAssoc, *hSeedChargeVsRowAssoc;

        TH2F *hClusterChargeVsRowAssoc_2D, *hSeedChargeVsRowAssoc_2D;

        TH1F* hPixelRawValueAssoc;
        TProfile2D* hPixelRawValueMapAssoc;

        TH1F* associatedTracksVersusTime;

        // local
        TH2F *residualsPosVsresidualsTime_local, *resX_vs_col, *resY_vs_col, *resX_vs_row, *resY_vs_row;
        TH1F *residualsX_local, *residualsY_local, *residualsPos_local;
        std::vector<TH1F*> residualsXclusterColLocal, residualsYclusterRowLocal;

        // global
        TH2F* residualsPosVsresidualsTime_global;
        TH1F *residualsX_global, *residualsY_global, *residualsPos_global;
        std::vector<TH1F*> residualsXclusterColGlobal, residualsYclusterRowGlobal;
        std::vector<TH1F*> residualsYclusterColGlobal, residualsXclusterRowGlobal;

        TH1F* clusterChargeAssoc;
        TH1F* seedChargeAssoc;
        TH1F* clusterSizeAssoc;
        TH1F* clusterSizeAssocNorm;
        TH1F* clusterWidthRowAssoc;
        TH1F* clusterWidthColAssoc;

        TH1F* hCutHisto;

        TProfile2D *rmsxvsxmym, *rmsyvsxmym, *rmsxyvsxmym;
        TProfile2D *qvsxmym, *qMoyalvsxmym, *pxqvsxmym;
        TProfile2D *qvsxmym_1px, *qvsxmym_2px, *qvsxmym_3px, *qvsxmym_4px;
        TProfile2D* npxvsxmym;
        TH2F *npx1vsxmym, *npx2vsxmym, *npx3vsxmym, *npx4vsxmym;

        TH1F* hTrackZPosDUT;
        TH1F* residualsTime;
        TH2F* residualsTimeVsTot;
        TH2F* residualsTimeVsCol;
        TH2F* residualsTimeVsRow;
        TH2F* residualsTimeVsTime;
        TH2F* residualsTimeVsSignal;
        TH2F* hAssociatedTracksGlobalPosition;
        TH2F* hAssociatedTracksLocalPosition;
        TH2F* hUnassociatedTracksLocalPosition;
        TH2F* hUnassociatedTracksGlobalPosition;

        TH1F* pxTimeMinusSeedTime;
        TH2F* pxTimeMinusSeedTime_vs_pxCharge;
        TH2F* pxTimeMinusSeedTime_vs_pxCharge_2px;
        TH2F* pxTimeMinusSeedTime_vs_pxCharge_3px;
        TH2F* pxTimeMinusSeedTime_vs_pxCharge_4px;

        TH2F* track_trackDistance;

        TProfile2D* htimeDelay_trackPos_TProfile;
        TH2D* htimeRes_trackPos_TProfile;
        TProfile2D* hclusterSize_trackPos_TProfile;

        // Member variables
        double inpixelBinSize_;
        double time_cut_frameedge_;
        double spatial_cut_sensoredge_;
        double chi2_ndof_cut_;
        bool use_closest_cluster_;
        int n_timebins_;
        double time_binning_;
        int n_chargebins_;
        double charge_histo_range_;
        int n_rawbins_;
        double raw_histo_range_;
        bool correlations_;
        int num_tracks_;

        void createGlobalResidualPlots();
        void createLocalResidualPlots();
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DUT_ANALYSIS_H
