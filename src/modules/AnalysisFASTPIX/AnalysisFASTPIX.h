/**
 * @file
 * @brief Definition of module AnalysisFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH2Poly.h>
#include <TProfile2D.h>
#include <TProfile2Poly.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class AnalysisFASTPIX : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        AnalysisFASTPIX(Configuration& config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

        /**
         * @brief [Finalise module]
         */
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        bool inRoi(PositionVector3D<Cartesian3D<double>>);
        int getFlags(std::shared_ptr<Event> event, size_t trigger);
        static void printEfficiency(int total_tracks, int matched_tracks);

        Int_t fillTriangle(TH2* hist, double x, double y, double val = 1);

        TH2F *hitmap, *hitmapIntercept, *hitmapNoIntercept, *hitmapTrigger, *hitmapNoTrigger, *hitmapTimecuts, *hitmapAssoc;
        TH2F *hitmapTriggerAssoc, *hitmapNoTriggerAssoc, *hitmapNoAssoc, *hitmapNoAssocMissing, *hitmapDeadtime,
            *hitmapDeadtimeTrigger, *hitmapDeadtimeNoTrigger;

        TProfile2D *clusterSizeMap, *clusterChargeMap, *seedChargeMap;
        TProfile2D* clusterSizeMapROI;
        TProfile2Poly *clusterSizeMap_inpix, *clusterChargeMap_inpix, *seedChargeMap_inpix;
        TH2Poly *hitmapTriggerAssoc_inpix, *hitmapNoTriggerAssoc_inpix, *hitmapNoAssoc_inpix;

        TH2Poly *hitmapTrigger_inpix, *hitmapTimecuts_inpix, *hitmapAssoc_inpix, *hitmapNoTrigger_inpix;
        TH2Poly *hitmapDeadtime_inpix, *hitmapDeadtimeTrigger_inpix, *hitmapDeadtimeNoTrigger_inpix;

        TH1F *clusterSize, *clusterSizeROI, *inefficientAssocEventStatus, *inefficientAssocDt, *inefficientTriggerAssocDt;
        TH1F *inefficientAssocDist, *inefficientTriggerAssocTime;
        TH1F *clusterCharge, *clusterChargeROI;

        double chi2_ndof_cut_, time_cut_frameedge_, time_cut_deadtime_, time_cut_trigger_;
        double bin_size_, hist_scale_;
        bool use_closest_cluster_;

        ROOT::Math::XYVector roi_min, roi_max;
        double roi_margin_x_;
        double roi_margin_y_;
        size_t triangle_bins_;
        bool roi_inner_;

        double last_timestamp = 0;
        double pitch, height;
        size_t m_currentEvent = 0;

        std::shared_ptr<Detector> m_detector;
    };

} // namespace corryvreckan
