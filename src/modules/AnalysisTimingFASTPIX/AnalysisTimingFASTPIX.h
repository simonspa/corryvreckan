/**
 * @file
 * @brief Definition of module AnalysisTimingFASTPIX
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <ROOT/RDataFrame.hxx>
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
         * @param detectors Vector of pointers to the detectors
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
        TH2F *timewalk2d, *timewalk2d_inner, *timewalk2d_outer;
        TH1F *timewalk, *timewalk_inner, *timewalk_outer;
        TH2F *hitmapLocal, *hitmapLocalInner, *hitmapLocalOuter;
        TH2F *hitmapLocalCut, *hitmapLocalInnerCut, *hitmapLocalOuterCut;
        TTree *tree;

        std::vector<double> dt_hist, dt_inner_hist, dt_outer_hist;
    
        double chi2_ndof_cut_;

        int pixel_;
        double tot_, dt_;

        std::shared_ptr<Detector> m_detector;
    };

} // namespace corryvreckan
