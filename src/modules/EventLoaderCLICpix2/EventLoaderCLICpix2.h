/**
 * @file
 * @brief Definition of module EventLoaderCLICpix2
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EventLoaderCLICpix2_H
#define EventLoaderCLICpix2_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

#include "CLICpix2/clicpix2_frameDecoder.hpp"
#include "CLICpix2/clicpix2_pixels.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderCLICpix2 : public Module {

    public:
        // Constructors and destructors
        EventLoaderCLICpix2(Configuration& config, std::shared_ptr<Detector> detector);
        ~EventLoaderCLICpix2() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;
        std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix_config;

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        bool comp, sp_comp;
        caribou::clicpix2_frameDecoder* decoder;

        // Member variables
        int m_eventNumber;
        std::string m_filename;
        std::string m_matrix;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH2F* hMaskMap;
        TH2F* hTimeWalk;
        TH2F* hHitMapDiscarded;
        TProfile2D* hPixelToTMap;
        TH1F* hPixelToT;
        TH1F* hPixelToA;
        TH1F* hPixelCnt;
        TH1F* hPixelMultiplicity;

        bool discardZeroToT;
    };
} // namespace corryvreckan
#endif // Clicpix2EventLoader_H
