/**
 * @file
 * @brief Definition of [AnalysisEfficiency] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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
        AnalysisEfficiency(Configuration config, std::shared_ptr<Detector> detector);
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;

        TProfile2D* hPixelEfficiencyMap;
        TProfile2D* hChipEfficiencyMap;
        TProfile2D* hGlobalEfficiencyMap;

        double m_chi2ndofCut, m_timeCutFrameEdge;
        int total_tracks, matched_tracks;
    };

} // namespace corryvreckan
