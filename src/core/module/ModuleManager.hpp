/** @file
 *  @brief Interface to the core framework
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_MODULE_MANAGER_H
#define CORRYVRECKAN_MODULE_MANAGER_H

#include <fstream>
#include <map>
#include <vector>

#include <TBrowser.h>
#include <TDirectory.h>
#include <TFile.h>

#include "Module.hpp"
#include "core/clipboard/Clipboard.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/detector/Detector.hpp"
#include "core/detector/PixelDetector.hpp"

namespace corryvreckan {

    /**
     * @ingroup Managers
     * @brief Manager responsible for dynamically loading all modules and running their event sequence
     *
     * The module manager class is the core class which allows the event processing to run. It basically contains a vector of
     * modules, each of which is initialised, run on each event and finalized. It does not define what an event is, merely
     * runs each module sequentially and passes the clipboard between them (erasing it at the end of each run sequence). When
     * an module returns a Failure code, the event processing will stop.
     */
    class ModuleManager {
        using ModuleList = std::list<std::shared_ptr<Module>>;

    public:
        /**
         * @brief Construct manager
         */
        ModuleManager();
        /**
         * @brief Use default destructor
         */
        ~ModuleManager() = default;

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;
        /// @}

        /// @{
        /**
         * @brief Moving the manager is not allowed
         */
        ModuleManager(ModuleManager&&) = delete;
        ModuleManager& operator=(ModuleManager&&) = delete;
        /// @}

        // Member functions
        void load(ConfigManager* conf_mgr);

        void run();
        void initializeAll();
        void finalizeAll();
        void terminate();

        TBrowser* browser;

    protected:
        // Member variables
        std::shared_ptr<Clipboard> m_clipboard;
        std::vector<std::shared_ptr<Detector>> m_detectors;

    private:
        void timing();

        void load_detectors();
        void load_modules();

        /**
         * @brief Get a specific detector, identified by its name
         * @param  name Name of the detector to retrieve
         * @return Pointer to the requested detector, nullptr if detector with given name is not found
         */
        std::shared_ptr<Detector> get_detector(std::string name);

        std::shared_ptr<Detector> m_reference;

        // Create string vector for detector types:
        std::vector<std::string> get_type_vector(char* tokens);
        // Log file if specified
        std::ofstream log_file_;

        std::unique_ptr<TFile> m_histogramFile;
        int m_events;
        int m_tracks;
        int m_pixels;

        /**
         * @brief Create unique modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @return An unique module together with its identifier
         */
        std::pair<ModuleIdentifier, Module*> create_unique_module(void* library, Configuration config);

        /**
         * @brief Create detector modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @param dut_only Bollean signalling whether should be instantiated only for DUT detectors
         * @param types List of detector type restrictions imposed by the module itself
         * @return A list of all created detector modules and their identifiers
         */
        std::vector<std::pair<ModuleIdentifier, Module*>>
        create_detector_modules(void* library, Configuration config, bool dut_only, std::vector<std::string> types);

        using IdentifierToModuleMap = std::map<ModuleIdentifier, ModuleList::iterator>;

        ModuleList m_modules;
        IdentifierToModuleMap id_to_module_;

        std::map<std::string, void*> loaded_libraries_;

        std::atomic<bool> m_terminate;
        ConfigManager* conf_manager_;

        std::tuple<LogLevel, LogFormat> set_module_before(const std::string&, const Configuration& config);
        void set_module_after(std::tuple<LogLevel, LogFormat> prev);

        std::map<Module*, long double> module_execution_time_;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_MODULE_MANAGER_H
