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
     * @brief Errors related to Object
     *
     * Problems that could also have been detected at compile time by specialized
     * software
     */
    class ObjectError : public Exception {
        /**
         * @brief Creates exception with the given logical problem
         * @param what_arg Text describing the problem
         */
        explicit ObjectError(std::string what_arg) : Exception(std::move(what_arg)) {}

    protected:
        /**
         * @brief Internal constructor for exceptions setting the error message
         * indirectly
         */
        ObjectError() = default;
    };

    /**
     * @ingroup Exceptions
     * @brief Indicates an object that does not contain a reference fetched
     */
    class MissingReferenceException : public ObjectError {
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

    class TrackError : public ObjectError {
    public:
        /**
         * @brief TrackError
         * @param source
         */

        explicit TrackError(const std::type_info& source, const std::string msg = "") {
            error_message_ += " Track Object ";
            error_message_ += corryvreckan::demangle((source.name()));
            if(msg != "") {
                error_message_ += " shows error ";
                error_message_ += msg;
            }
        }
    };

    class UnknownTrackModel : public TrackError {
    public:
        explicit UnknownTrackModel(const std::type_info& source, std::string model) : TrackError(source) {
            error_message_ += " is requesting non exiting track model ";
            error_message_ += model;
        }
    };
    class TrackModelChanged : public TrackError {
    public:
        explicit TrackModelChanged(const std::type_info& source, std::string modelSet, std::string model)
            : TrackError(source) {
            error_message_ += " is defined as ";
            error_message_ += model;
            error_message_ += " but a ";
            error_message_ += modelSet;
            error_message_ += " is passed ";
        }
    };

    class TrackFitError : public TrackError {
    public:
        explicit TrackFitError(const std::type_info& source, std::string error) : TrackError(source) {
            error_message_ += " fitting procedure fails with message: ";
            error_message_ += error;
        }
    };

    class RequestParameterBeforeFitError : public TrackError {
    public:
        template <typename T>
        RequestParameterBeforeFitError(T* source, std::string requestedParameter) : TrackError(typeid(source)) {
            error_message_ += "  request parameter \"";
            error_message_ += requestedParameter;
            error_message_ += "  \" before fitting";
        }
    };

} // namespace corryvreckan

#endif /* CORRYVRECKAN_OBJECT_EXCEPTIONS_H */
