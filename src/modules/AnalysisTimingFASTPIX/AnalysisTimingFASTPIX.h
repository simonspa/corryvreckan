/**
 * @file
 * @brief Definition of module AnalysisTimingFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <TProfile2Poly.h>
#include <TTree.h>
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
    class AnalysisTimingFASTPIX : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector
         */
        AnalysisTimingFASTPIX(Configuration& config, std::shared_ptr<Detector> detector);

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
        template <typename T> Int_t fillTriangle(T* hist, double x, double y, double val = 1);
        bool inRoi(PositionVector3D<Cartesian3D<double>> p);

        TH2F *timewalk2d, *timewalk2d_inner, *timewalk2d_outer;
        TH1F *timewalk, *timewalk_inner, *timewalk_outer;
        TH2F *hitmapLocal, *hitmapLocalInner, *hitmapLocalOuter;
        TH2F *hitmapLocalCut, *hitmapLocalInnerCut, *hitmapLocalOuterCut;
        TH1F* seedDistance;
        TH1F* seedStatus;
        TTree* tree;

        TProfile2D* timewalkMap;
        TProfile2Poly* timewalk_inpix;

        std::vector<double> dt_hist, dt_inner_hist, dt_outer_hist;

        double chi2_ndof_cut_;

        int pixel_col_, pixel_row_;
        double tot_, dt_, seed_dist_;
        double track_x_, track_y_;
        double track_x_inpix_, track_y_inpix_;

        double pitch, height;
        size_t triangle_bins_;

        std::shared_ptr<Detector> m_detector;
    };

} // namespace corryvreckan
