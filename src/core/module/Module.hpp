/** @file
 *  @brief Base class for Corryvreckan modules
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_MODULE_H
#define CORRYVRECKAN_MODULE_H

#include <string>
#include "TStopwatch.h"
#include "core/clipboard/Clipboard.hpp"
#include "core/config/Configuration.hpp"
#include "core/detector/Detector.hpp"
#include "exceptions.h"

namespace corryvreckan {
    /**
     * @brief Flags to signal the status of a module processing a given event
     *
     * These flags are to be used as return values for the Module::run() function to signal the framework the outcome of the
     * event processing. Here, three different states are supported:
     * - Success: the event processing finished successfully and the framework should continue processing this event with the
     * next module.
     * - NoData: there was no data found to process by this module. The framework should skip all following modules and
     * proceed with the next event.
     * - Failure: the module failed to process the event correctly or requests an end of the event processing for other
     * reasons. The framework should stop processing events and call Module::finalise for all modules.
     */
    enum StatusCode {
        Success,
        NoData,
        Failure,
    };

    /** Base class for all modules
     *  @defgroup Modules Modules
     */

    /**
     * @brief Base class for all modules
     *
     * The module base is the core of the modular framework. All modules should be descendants of this class. The base class
     * defines the methods the children can implement:
     * - Module::initialise(): for initializing the module at the start
     * - Module::run(Clipoard* Clipboard): for doing the job of every module for every event
     * - Module::finalise(): for finalising the module at the end
     *
     * The module class also provides a few utility methods such as a stopwatch for timing measurements.
     */
    class Module {

    public:
        /**
         * @brief Delete default constructor (configuration required)
         */
        Module() = delete;

        /**
         * @brief Base constructor for modules
         * @param config Configuration for this module
         * @param detectors List of detectors this module should be aware of
         */
        Module(Configuration config, std::vector<Detector*> detectors);

        /**
         * @brief Essential virtual destructor.
         */
        virtual ~Module();

        /**
         * @brief Copying a module is not allowed
         */
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        /**
         * @brief Initialise the module before the event sequence
         *
         * Does nothing if not overloaded.
         */
        virtual void initialise() {}

        /**
         * @brief Execute the function of the module for every event
         * @param clipboard Pointer to the central clipboard to store and fetch information
         * @return Status code of the event processing
         *
         * Does nothing if not overloaded.
         */
        virtual StatusCode run(Clipboard* clipboard) { (void)clipboard; }

        /**
         * @brief Finalise the module after the event sequence
         * @note Useful to have before destruction to allow for raising exceptions
         *
         * Does nothing if not overloaded.
         */
        virtual void finalise(){};

        // Methods to get member variables
        std::string getName() { return m_name; }
        Configuration getConfig() { return m_config; }
        TStopwatch* getStopwatch() { return m_stopwatch; }

    protected:
        // Member variables
        std::string m_name;
        Configuration m_config;

        std::vector<Detector*> get_detectors() { return m_detectors; };
        std::size_t num_detectors() { return m_detectors.size(); };
        Detector* get_detector(std::string name);
        bool has_detector(std::string name);

    private:
        TStopwatch* m_stopwatch;
        std::vector<Detector*> m_detectors;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_MODULE_H
