/**
 * @file
 * @brief Definition of [AnalysisEfficiency] module
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <iostream>

#include "core/module/Module.hpp"

#include "TEfficiency.h"
#include "TH2D.h"
#include "TNamed.h"
#include "TProfile2D.h"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class AnalysisEfficiency : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        AnalysisEfficiency(Configuration& config, std::shared_ptr<Detector> detector);
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;

        TH1D* hPixelEfficiency;
        TH1D* hPixelEfficiencyMatrix;

        // Profile version
        TProfile2D* hPixelEfficiencyMap_trackPos_TProfile;
        TProfile2D* hPixelEfficiencyMap_inPixelROI_trackPos_TProfile;
        TProfile2D* hChipEfficiencyMap_trackPos_TProfile;
        TProfile2D* hPixelEfficiencyMatrix_TProfile;
        TProfile2D* hGlobalEfficiencyMap_trackPos_TProfile;
        TProfile2D* hChipEfficiencyMap_clustPos_TProfile;
        TProfile2D* hGlobalEfficiencyMap_clustPos_TProfile;

        // TEfficiency version
        TEfficiency* hPixelEfficiencyMap_trackPos;
        TEfficiency* hChipEfficiencyMap_trackPos;
        TEfficiency* hGlobalEfficiencyMap_trackPos;
        TEfficiency* hChipEfficiencyMap_clustPos;
        TEfficiency* hGlobalEfficiencyMap_clustPos;

        TEfficiency* eTotalEfficiency;
        TEfficiency* eTotalEfficiency_inPixelROI;
        TEfficiency* efficiencyColumns;
        TEfficiency* efficiencyRows;
        TEfficiency* efficiencyVsTime;
        TEfficiency* efficiencyVsTimeLong;

        TH1D* hDistanceCluster;
        TH1D* hTimeDiffPrevTrack_assocCluster;
        TH1D* hTimeDiffPrevTrack_noAssocCluster;
        TH1D* hRowDiffPrevTrack_assocCluster;
        TH1D* hColDiffPrevTrack_assocCluster;
        TH1D* hRowDiffPrevTrack_noAssocCluster;
        TH1D* hColDiffPrevTrack_noAssocCluster;
        TH1D* hTrackTimeToPrevHit_matched;
        TH1D* hTrackTimeToPrevHit_notmatched;

        TH2D* hPosDiffPrevTrack_assocCluster;
        TH2D* hPosDiffPrevTrack_noAssocCluster;
        TH2D* hDistanceCluster_track;
        TH2D* htimeRes_cluster_size;
        double m_chi2ndofCut, m_timeCutFrameEdge, m_inpixelBinSize, spatial_cut_sensoredge;
        XYVector m_inpixelEdgeCut;
        int m_maskedPixelDistanceCut = 1;
        int total_tracks = 0;
        int matched_tracks = 0;

        double last_track_timestamp = 0;
        double last_track_col = 0.;
        double last_track_row = 0.;
        double n_track = 0, n_chi2 = 0, n_dut = 0, n_roi = 0, n_masked = 0, n_frameedge = 0, n_requirecluster = 0;
        std::vector<std::string> require_associated_cluster_on_;

        Matrix<double> prev_hit_ts; // matrix containing previous hit timestamp for every pixel
    };

} // namespace corryvreckan
