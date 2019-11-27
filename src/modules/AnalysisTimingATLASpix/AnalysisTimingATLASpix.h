/**
 * @file
 * @brief Definition of [AnalysisTimingATLASpix] module
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * *
 * Refer to the User's Manual for more details.
 */

#include <iostream>
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile2D.h"
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class AnalysisTimingATLASpix : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        AnalysisTimingATLASpix(Configuration config, std::shared_ptr<Detector> detector);
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;

        // timing correction functions:
        void correctClusterTimestamp(Cluster*, int mode);

        // 1D histograms:
        TH1F* hTrackCorrelationTime;
        TH1F* hTrackCorrelationTimeAssoc;
        TH1F* hTrackCorrelationTime_rowCorr;
        TH1F* hTrackCorrelationTime_rowAndTimeWalkCorr;
        TH1F* hTrackCorrelationTime_rowAndTimeWalkCorr_l25;
        TH1F* hTrackCorrelationTime_rowAndTimeWalkCorr_l40;
        TH1F* hTrackCorrelationTime_rowAndTimeWalkCorr_g40;
        TH1D* hTrackCorrelationTime_example;
        TH1F* hClusterTimeMinusPixelTime;

        // 2D histograms:
        TH2F* hTrackCorrelationTimeAssocVsTime;
        TH2F* hTrackCorrelationTimeVsCol; // control plot only
        TH2F* hTrackCorrelationTimeVsRow;
        TH2F* hTrackCorrelationTimeVsRow_1px;
        TH2F* hTrackCorrelationTimeVsRow_npx;
        TH2F* hTrackCorrelationTimeVsRow_rowCorr;
        TH2F* hTrackCorrelationTimeVsTot;
        TH2F* hTrackCorrelationTimeVsTot_1px;
        TH2F* hTrackCorrelationTimeVsTot_npx;
        TH2F* hTrackCorrelationTimeVsTot_rowCorr;
        TH2F* hTrackCorrelationTimeVsTot_rowCorr_1px;
        TH2F* hTrackCorrelationTimeVsTot_rowCorr_npx;
        TH2F* hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr;
        TH2F* hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr;

        TH2F* hClusterSizeVsTot_Assoc;

        TH2F* hHitMapAssoc;
        TH2F* hHitMapAssoc_highCharge;
        TH2F* hHitMapAssoc_inPixel;
        TH2F* hHitMapAssoc_inPixel_highCharge;
        TH2F* hClusterMapAssoc;

        TH2F* hTotVsTime;
        TH2F* hTotVsTime_high;

        // Control Plots for "left tail" and "main peak" of time correlation
        TH2F* hClusterMap_leftTail;
        TH2F* hClusterMap_mainPeak;
        TH1F* hTot_leftTail;
        TH1F* hTot_mainPeak;
        TH1F* hPixelTimestamp_leftTail;
        TH1F* hPixelTimestamp_mainPeak;
        TH1F* hClusterSize_leftTail;
        TH1F* hClusterSize_mainPeak;

        // TGraphErrors:
        TGraphErrors* gTimeCorrelationVsRow;
        TGraphErrors* gTimeCorrelationVsTot_rowCorr;
        TGraphErrors* gTimeCorrelationVsTot_rowCorr_1px;
        TGraphErrors* gTimeCorrelationVsTot_rowCorr_npx;

        TGraphErrors* gRowCorr;
        TGraphErrors* gTimeWalkCorr;

        // Member Variables:
        std::string m_DUT;
        double m_timeCut;
        double m_chi2ndofCut;
        double m_timeCutFrameEdge;
        double m_clusterChargeCut;
        size_t m_clusterSizeCut;
        int m_highTotCut;       // for pixel->tot()
        double m_highChargeCut; // for cluster->charge()
        double m_leftTailCut;

        std::string m_correctionFile_row;
        std::string m_correctionGraph_row;
        std::string m_correctionFile_timewalk;
        std::string m_correctionGraph_timewalk;
        bool m_calcCorrections;
        bool m_pointwise_correction_row;
        bool m_pointwise_correction_timewalk;
        int m_totBinExample;

        int total_tracks_uncut;
        int tracks_afterChi2Cut;
        int tracks_hasIntercept;
        int tracks_isWithinROI;
        int tracks_afterMasking;
        int total_tracks;
        int matched_tracks;
        int tracks_afterClusterChargeCut;
        int tracks_afterClusterSizeCut;
    };

} // namespace corryvreckan
