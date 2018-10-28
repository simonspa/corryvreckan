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

#include "ModuleIdentifier.hpp"
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
     */
    class Module {
        friend class ModuleManager;

    public:
        /**
         * @brief Delete default constructor (configuration required)
         */
        Module() = delete;

        /**
         * @brief Base constructor for detector modules
         * @param config Configuration for this module
         * @param detector Pointer to the detector this module is bound to
         */
        Module(Configuration config, std::shared_ptr<Detector> detector);

        /**
         * @brief Base constructor for unique modules
         * @param config Configuration for this module
         * @param detectors List of detectors this module should be aware of
         */
        Module(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

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
         * @brief Get the unique name of this module
         * @return Unique name
         * @note Can be used to interact with ROOT objects that require an unique name
         */
        std::string getUniqueName() const;

        /**
         * @brief Get the configuration for this module
         * @return Reference to the configuration of this module
         */
        Configuration& getConfig() { return m_config; }

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
        virtual StatusCode run(Clipboard*) { return Success; }

        /**
         * @brief Finalise the module after the event sequence
         * @note Useful to have before destruction to allow for raising exceptions
         *
         * Does nothing if not overloaded.
         */
        virtual void finalise(){};

        /**
         * @brief Get ROOT directory which should be used to output histograms et cetera
         * @return ROOT directory for storage
         */
        TDirectory* getROOTDirectory() const;

    protected:
        // Configuration of this module
        Configuration m_config;

        /**
         * @brief Get list of all detectors this module acts on
         * @return List of all detectors for this module
         */
        std::vector<std::shared_ptr<Detector>> get_detectors() { return m_detectors; };

        /**
         * @brief Get the number of detectors this module takes care of
         * @return Number of detectors to act on
         */
        std::size_t num_detectors() { return m_detectors.size(); };

        /**
         * @brief Get a specific detector, identified by its name
         * @param  name Name of the detector to retrieve
         * @return Pointer to the requested detector
         * @throws ModuleError if detector with given name is not found for this module
         */
        std::shared_ptr<Detector> get_detector(std::string name);

        /**
         * @brief Get the reference detector for this setup
         * @return Pointer to the reference detector
         */
        std::shared_ptr<Detector> get_reference();

        /**
         * @brief Get the device under test
         * @return Pointer to the DUT detector.  A nullptr is returned if no DUT is found.
         * FIXME This should allow retrieval of a vector of DUTs
         */
        std::shared_ptr<Detector> get_dut();

        /**
         * @brief Check if this module should act on a given detector
         * @param  name Name of the detector to check
         * @return True if detector is known to this module, false if detector is unknown.
         */
        bool has_detector(std::string name);

    private:
        /**
         * @brief Set the module identifier for internal use
         * @param identifier Identifier of the instantiation
         */
        void set_identifier(ModuleIdentifier identifier);
        /**
         * @brief Get the module identifier for internal use
         * @return Identifier of the instantiation
         */
        ModuleIdentifier get_identifier() const;
        ModuleIdentifier identifier_;

        /**
         * @brief Set the output ROOT directory for this module
         * @param directory ROOT directory for storage
         */
        void set_ROOT_directory(TDirectory* directory);
        TDirectory* directory_{nullptr};

        // Configure the reference detector:
        void setReference(std::shared_ptr<Detector> reference) { m_reference = reference; };
        std::shared_ptr<Detector> m_reference;

        // List of detectors to act on
        std::vector<std::shared_ptr<Detector>> m_detectors;
    };

} // namespace corryvreckan

#endif // CORRYVRECKAN_MODULE_H
