/**
 * @file
 * @brief Definition of [Dummy] module
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
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class Dummy : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        Dummy(Configuration config, std::vector<Detector*> detectors);

        /**
         * @brief [Initialise this module]
         */
        void initialise();

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(Clipboard* clipboard);

        /**
         * @brief [Finalise module]
         */
        void finalise();

    private:
        int m_eventNumber;
    };

} // namespace corryvreckan
