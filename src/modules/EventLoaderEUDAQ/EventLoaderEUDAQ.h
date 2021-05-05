/**
 * @file
 * @brief Definition of module EventLoaderEUDAQ
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EventLoaderEUDAQ_H
#define EventLoaderEUDAQ_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "eudaq/FileReader.hh"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderEUDAQ : public Module {

    public:
        // Constructors and destructors
        EventLoaderEUDAQ(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EventLoaderEUDAQ() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

        // EUDAQ file reader instance:
        eudaq::FileReader* reader;

        // Member variables
        std::string m_filename{};
        bool m_longID;

        // Bunch crossing time of the LV1 trigger (BCID) histogram
        std::map<std::string, TH1F*> lv1;
        std::map<std::string, TH2F*> hitmap;
    };
} // namespace corryvreckan
#endif // EventLoaderEUDAQ_H
