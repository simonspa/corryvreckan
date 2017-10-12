/**
 * @file
 * @brief Collection of all algorithm exceptions
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_ALGORITHM_EXCEPTIONS_H
#define CORRYVRECKAN_ALGORITHM_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Exceptions
     * @brief Notifies of an error with dynamically loading a algorithm
     *
     * algorithm library can either not be found, is outdated or invalid in general.
     */
    class DynamicLibraryError : public RuntimeError {
    public:
        /**
         * @brief Constructs loading error for given algorithm
         * @param algorithm Name of the algorithm that cannot be loaded
         */
        explicit DynamicLibraryError(const std::string& algorithm) {
            error_message_ = "Dynamic library loading failed for algorithm " + algorithm;
        }
    };

    /**
     * @ingroup Exceptions
     * @brief General exception for algorithms if something goes wrong
     * @note Only runtime error that should be raised directly by algorithms
     *
     * This error can be raised by algorithms if a problem comes up that cannot be foreseen by the framework itself.
     */
    class AlgorithmError : public RuntimeError {
    public:
        /**
         * @brief Constructs error with a description
         * @param reason Text explaining the reason of the error
         */
        // TODO [doc] the algorithm itself is missing
        explicit AlgorithmError(std::string reason) { error_message_ = std::move(reason); }
    };
} // namespace corryvreckan

#endif /* CORRYVRECKAN_ALGORITHM_EXCEPTIONS_H */
