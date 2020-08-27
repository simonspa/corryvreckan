/**
 * @file
 * @brief Definition of module AnalysisSensorEdge
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TProfile2D.h"

#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to analyse the sensor edge
     */
    class AnalysisSensorEdge : public Module {

    public:
        /**
         * @brief Constructor for this DUT module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        AnalysisSensorEdge(Configuration& config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;

        double m_inpixelBinSize;

        // The four edges
        TProfile2D* efficiencyFirstCol;
        TProfile2D* efficiencyLastCol;
        TProfile2D* efficiencyFirstRow;
        TProfile2D* efficiencyLastRow;
    };

} // namespace corryvreckan
