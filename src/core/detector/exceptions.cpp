/**
 * @file
 * @brief Implementation of detector exceptions
 *
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "exceptions.h"
#include "Detector.hpp"

using namespace corryvreckan;

InvalidSettingError::InvalidSettingError(const Detector* detector, const std::string& key, const std::string& reason) {
    error_message_ = "Setting '" + key + "' of detector '" + detector->getName() + "' is not valid";
    if(!reason.empty()) {
        error_message_ += ": " + reason;
    }
}
