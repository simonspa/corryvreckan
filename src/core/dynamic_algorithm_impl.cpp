/**
 * @file
 * @brief Special file automatically included in the algorithm for the dynamic
 * loading
 *
 * Needs the following names to be defined by the build system
 * - CORRYVRECKAN_ALGORITHM_NAME: name of the module
 * - CORRYVRECKAN_ALGORITHM_HEADER: name of the header defining the module
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_ALGORITHM_NAME
#error "This header should only be automatically included during the build"
#endif

#include <memory>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/utils/log.h"

#include CORRYVRECKAN_ALGORITHM_HEADER

namespace corryvreckan {
    extern "C" {
    Algorithm* corryvreckan_algorithm_generator(Configuration config, Clipboard* clipboard);
    Algorithm* corryvreckan_algorithm_generator(Configuration config, Clipboard* clipboard) {
        auto algorithm = new CORRYVRECKAN_ALGORITHM_NAME(std::move(config), clipboard); // NOLINT
        return static_cast<Algorithm*>(algorithm);
    }
    }
}
