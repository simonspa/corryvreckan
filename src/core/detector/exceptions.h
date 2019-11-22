/**
 * @file
 * @brief Collection of all detector exceptions
 *
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_DETECTOR_EXCEPTIONS_H
#define CORRYVRECKAN_DETECTOR_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Exceptions
     * @brief Base class for all detector exceptions in the framework.
     */
    class DetectorError : public Exception {};

    class Detector;
    /**
     * @ingroup Exceptions
     * @brief Indicates an error with the setting of the detector
     *
     * Should be raised if the setting of the detector is not compatible with the action attempted
     */
    class InvalidSettingError : public DetectorError {
    public:
        /**
         * @brief Construct an error for an invalid setting
         * @param detector Detector object containing the problematic setting
         * @param key Name of the problematic setting
         * @param reason Reason why the value is invalid (empty if no explicit reason)
         */
        InvalidSettingError(const Detector* detector, const std::string& key, const std::string& reason = "");
    };
} // namespace corryvreckan

#endif /* CORRYVRECKAN_DETECTOR_EXCEPTIONS_H */
