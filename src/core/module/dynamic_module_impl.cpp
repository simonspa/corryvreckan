/**
 * @file
 * @brief Special file automatically included in the module for the dynamic
 * loading
 *
 * Needs the following names to be defined by the build system
 * - CORRYVRECKAN_MODULE_NAME: name of the module
 * - CORRYVRECKAN_MODULE_HEADER: name of the header defining the module
 * - CORRYVRECKAN_MODULE_GLOBAL: true if the module is unique, false otherwise
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_MODULE_NAME
#error "This header should only be automatically included during the build"
#endif

#include <memory>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/utils/log.h"

#include CORRYVRECKAN_MODULE_HEADER

namespace corryvreckan {

    extern "C" {
    /**
     * @brief Returns the type of the Module it is linked to
     *
     * Used by the ModuleManager to determine if it should instantiate a single module or modules per detector instead.
     */
    bool corryvreckan_module_is_global();

    /**
     * @brief Returns the type of the Module it is linked to
     *
     * Used by the ModuleManager to determine if it should instantiate a modules per detector or only per DUT.
     */
    bool corryvreckan_module_is_dut();

#if CORRYVRECKAN_MODULE_GLOBAL || defined(DOXYGEN)
    /**
     * @brief Instantiates an unique module
     * @param config Configuration for this module
     * @param detectors Vector of pointers to all detectors to be processed by this module
     * @return Instantiation of the module
     *
     * Internal method for the dynamic loading in the central Analysis class. Forwards the supplied arguments to the
     * constructor and returns an instantiation.
     */
    Module* corryvreckan_module_generator(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
    Module* corryvreckan_module_generator(Configuration config, std::vector<std::shared_ptr<Detector>> detectors) {
        auto module = new CORRYVRECKAN_MODULE_NAME(std::move(config), std::move(detectors)); // NOLINT
        return static_cast<Module*>(module);
    }
    // Returns that is a unique module
    bool corryvreckan_module_is_global() { return true; }
    // Globale modules cannot be DUT modules
    bool corryvreckan_module_is_dut() { return false; }

#endif

#if(!CORRYVRECKAN_MODULE_GLOBAL && !CORRYVRECKAN_MODULE_DUT) || defined(DOXYGEN)
    /**
     * @brief Instantiates a detector module
     * @param config Configuration for this module
     * @param detector Pointer to the Detector object this module is bound to
     * @return Instantiation of the module
     *
     * Internal method for the dynamic loading in the central Analysis class. Forwards the supplied arguments to the
     * constructor and returns an instantiation
     */
    Module* corryvreckan_module_generator(Configuration config, std::shared_ptr<Detector> detector);
    Module* corryvreckan_module_generator(Configuration config, std::shared_ptr<Detector> detector) {
        auto module = new CORRYVRECKAN_MODULE_NAME(std::move(config), std::move(detector)); // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a detector module
    bool corryvreckan_module_is_global() { return false; }
    // Return that this module is a generic detector module
    bool corryvreckan_module_is_dut() { return false; }
#endif

#if(!CORRYVRECKAN_MODULE_GLOBAL && CORRYVRECKAN_MODULE_DUT) || defined(DOXYGEN)
    /**
     * @brief Instantiates a DUT module
     * @param config Configuration for this module
     * @param detector Pointer to the Detector object this module is bound to
     * @return Instantiation of the module
     *
     * Internal method for the dynamic loading in the central Analysis class. Forwards the supplied arguments to the
     * constructor and returns an instantiation
     */
    Module* corryvreckan_module_generator(Configuration config, std::shared_ptr<Detector> detector);
    Module* corryvreckan_module_generator(Configuration config, std::shared_ptr<Detector> detector) {
        auto module = new CORRYVRECKAN_MODULE_NAME(std::move(config), std::move(detector)); // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a detector module
    bool corryvreckan_module_is_global() { return false; }
    // Return that this module is a generic detector module
    bool corryvreckan_module_is_dut() { return true; }
#endif
    }
} // namespace corryvreckan
