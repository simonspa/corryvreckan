/** @file
 *  @brief Base class for Corryvreckan modules
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_MODULE_H
#define CORRYVRECKAN_MODULE_H

#include <string>

#include "ModuleIdentifier.hpp"
#include "core/clipboard/Clipboard.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/detector/Detector.hpp"
#include "exceptions.h"

namespace corryvreckan {
    /**
     * @brief Flags to signal the status of a module processing a given event
     *
     * These flags are to be used as return values for the Module::run() function to signal the framework the outcome of the
     * event processing.
     */
    enum class StatusCode {
        Success,  // < Module finished processing data successfully
        NoData,   // < Module did not receive any data to process
        DeadTime, // < Module indicated that the respective detector is in deadtime
        EndRun,   // < Module requested pemature end of the run
        Failure,  // < Processing of data failed
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
     * - Module::run(Clipboard* Clipboard): for doing the job of every module for every event
     * - Module::finalize(): for finalising the module at the end
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
        explicit Module(Configuration& config, std::shared_ptr<Detector> detector);

        /**
         * @brief Base constructor for unique modules
         * @param config Configuration for this module
         * @param detectors List of detectors this module should be aware of
         */
        explicit Module(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);

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
         * @brief Create and return an absolute path to be used for output from a relative path
         * @param path Relative path to add after the main output directory
         * @param global True if the global output directory should be used instead of the module-specific version
         * @return Canonical path to an output file
         */
        std::string createOutputFile(const std::string& path, bool global = false);

        /**
         * @brief Initialise the module before the event sequence
         *
         * Does nothing if not overloaded.
         */
        virtual void initialize() {}

        /**
         * @brief Execute the function of the module for every event
         * @param clipboard Pointer to the central clipboard to store and fetch information
         * @return Status code of the event processing
         *
         * Does nothing if not overloaded.
         */
        virtual StatusCode run(const std::shared_ptr<Clipboard>&) { return StatusCode::Success; }

        /**
         * @brief Finalise the module after the event sequence
         * @note Useful to have before destruction to allow for raising exceptions
         *
         * Does nothing if not overloaded.
         */
        virtual void finalize(const std::shared_ptr<ReadonlyClipboard>&){};

        /**
         * @brief Get the config manager object to allow to read the global and other module configurations
         * @return Pointer to the config manager
         */
        ConfigManager* getConfigManager();

        /**
         * @brief Get ROOT directory which should be used to output histograms et cetera
         * @return ROOT directory for storage
         */
        TDirectory* getROOTDirectory() const;

    protected:
        /**
         * @brief Get the module configuration for internal use
         * @return Configuration of the module
         */
        Configuration& get_configuration();
        Configuration& config_;

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
         * @brief Get list of dut detectors this module acts on
         * @return List of dut detectors for this module
         */
        std::vector<std::shared_ptr<Detector>> get_duts();

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
         * @brief Set the link to the config manager
         * @param conf_manager ConfigManager holding all relevant configurations
         */
        void set_config_manager(ConfigManager* config);
        ConfigManager* conf_manager_{nullptr};

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
