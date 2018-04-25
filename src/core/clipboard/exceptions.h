/**
 * @file
 * @brief Collection of all configuration exceptions
 *
 * @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_CLIPBOARD_EXCEPTIONS_H
#define CORRYVRECKAN_CLIPOARD_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Exceptions
     * @brief Base class for all clipboard exceptions in the framework.
     */
    class ClipboardError : public Exception {};

    /**
     * @ingroup Exceptions
     * @brief Informs of missing data that has been requested
     */
    class MissingDataError : public ClipboardError {
    public:
        /**
         * @brief Construct an error for a missing key
         * @param key Name of the missing key
         * @param section Section where the key should have been defined
         */
        MissingDataError(const std::string& name) {
            error_message_ = "No data with key '" + name + "' exists on the clipboard";
        }
    };

} // namespace corryvreckan

#endif /* CORRYVRECKAN_CONFIG_EXCEPTIONS_H */
