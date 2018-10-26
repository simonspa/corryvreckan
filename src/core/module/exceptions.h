/**
 * @file
 * @brief Collection of all module exceptions
 *
 * @copyright Copyright (c) 2017 CERN and the Corryvreckan Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_MODULE_EXCEPTIONS_H
#define CORRYVRECKAN_MODULE_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Exceptions
     * @brief Notifies of an error with dynamically loading a module
     *
     * module library can either not be found, is outdated or invalid in general.
     */
    class DynamicLibraryError : public RuntimeError {
    public:
        /**
         * @brief Constructs loading error for given module
         * @param module Name of the module that cannot be loaded
         */
        explicit DynamicLibraryError(const std::string& module) {
            error_message_ = "Dynamic library loading failed for module " + module;
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Informs that a module executes an action is it not allowed to do in particular state
     *
     * A module for example tries to accesses special methods as Module::getOutputPath which are not allowed in the
     * constructors, or sends a message outside the Module::run method.
     */
    class InvalidModuleActionException : public LogicError {
    public:
        /**
         * @brief Constructs error with a description
         * @param message Text explaining the problem
         */
        // TODO [doc] the module itself is missing
        explicit InvalidModuleActionException(std::string message) { error_message_ = std::move(message); }
    };

    /**
     * @ingroup Exceptions
     * @brief General exception for modules if something goes wrong
     * @note Only runtime error that should be raised directly by modules
     *
     * This error can be raised by modules if a problem comes up that cannot be foreseen by the framework itself.
     */
    class ModuleError : public RuntimeError {
    public:
        /**
         * @brief Constructs error with a description
         * @param reason Text explaining the reason of the error
         */
        // TODO [doc] the module itself is missing
        explicit ModuleError(std::string reason) { error_message_ = std::move(reason); }
    };
} // namespace corryvreckan

#endif /* CORRYVRECKAN_MODULE_EXCEPTIONS_H */
