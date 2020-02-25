/**
 * @file
 * @brief Definition of module EventLoaderCLICpix
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EventLoaderCLICpix_H
#define EventLoaderCLICpix_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderCLICpix : public Module {

    public:
        // Constructors and destructors
        EventLoaderCLICpix(Configuration config, std::shared_ptr<Detector> detector);
        ~EventLoaderCLICpix() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;
        // Member variables
        int m_eventNumber;
        std::string m_filename;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hShutterLength;
        TH1F* hPixelMultiplicity;
    };
} // namespace corryvreckan
#endif // EventLoaderCLICpix_H
