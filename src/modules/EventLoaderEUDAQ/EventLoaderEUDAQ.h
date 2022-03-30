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

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/module/Module.hpp"

#include "SequentialReader.h"

class TH2F;

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

        // EUDAQ-based file reader instance:
        std::unique_ptr<SequentialReader> reader;

        // Member variables
        std::vector<std::filesystem::path> m_filenames{};
        bool m_longID;

        std::map<std::string, TH2F*> hitmap;
    };
} // namespace corryvreckan
#endif // EventLoaderEUDAQ_H
