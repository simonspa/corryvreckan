/**
 * @file
 * @brief Definition of module TextWriter
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * This module writes specific objects into a user specified text file
 *
 * Refer to the User's Manual for more details.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to write objects into a text file
     *
     * Loops over the selected objects and writes them into a text file in ASCII format.
     */
    class TextWriter : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        TextWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

        /**
         * @brief Reads the configuration and opens the file to write to
         */
        void initialise();

        /**
         * @brief Loops over the selected objects on the clipboard and writes them to file
         * @param clipboard Pointer to the clipboard
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

        /**
         * @brief No specific actions implemented
         */
        void finalise();

    private:
        int m_eventNumber;

        // Object names to include or exclude from writing
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // Output data file to write
        std::string output_file_name_{};
        std::unique_ptr<std::ofstream> output_file_;
    };

} // namespace corryvreckan
