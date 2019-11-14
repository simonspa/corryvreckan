/**
 * @file
 * @brief Collection of all object exceptions
 *
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_OBJECT_EXCEPTIONS_H
#define CORRYVRECKAN_OBJECT_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Exceptions
     * @brief Indicates an object that does not contain a reference fetched
     */
    class MissingReferenceException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a object with missing reference
         * @param source Type of the object from which the reference was requested
         * @param reference Type of the non-existing reference
         */
        explicit MissingReferenceException(const std::type_info& source, const std::type_info& reference) {
            error_message_ = "Object ";
            error_message_ += corryvreckan::demangle(source.name());
            error_message_ += " is missing reference to ";
            error_message_ += corryvreckan::demangle(reference.name());
        }
    };
    class MissingTrackModelException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a object with missing reference
         * @param source Type of the object from which the reference was requested
         * @param reference Type of the non-existing reference
         */
        explicit MissingTrackModelException(const std::type_info& source, const std::string reference) {
            error_message_ = "Object ";
            error_message_ += corryvreckan::demangle(source.name());
            error_message_ += " is requesting non exiting track model ";
            error_message_ += reference;
        }
    };

    class GblException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a object with missing reference
         * @param source Type of the object from which the reference was requested
         * @param reference Type of the non-existing reference
         */
        explicit GblException(const std::type_info& source, const std::string reference) {
            error_message_ = "Object ";
            error_message_ += corryvreckan::demangle(source.name());
            error_message_ += "  is failing due to ";
            error_message_ += reference;
        }
    };
} // namespace corryvreckan

#endif /* CORRYVRECKAN_OBJECT_EXCEPTIONS_H */
