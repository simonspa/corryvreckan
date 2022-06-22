/** @file
 *  @brief Interface to the core framework
 *  @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

// ROOT include files
#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include <TFile.h>
#include <TSystem.h>

// Local include files
#include "ModuleManager.hpp"
#include "core/utils/log.h"
#include "exceptions.h"

#include <chrono>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iomanip>

#define CORRYVRECKAN_MODULE_PREFIX "libCorryvreckanModule"
#define CORRYVRECKAN_GENERATOR_FUNCTION "corryvreckan_module_generator"
#define CORRYVRECKAN_GLOBAL_FUNCTION "corryvreckan_module_is_global"
#define CORRYVRECKAN_DUT_FUNCTION "corryvreckan_module_is_dut"
#define CORRYVRECKAN_AUX_FUNCTION "corryvreckan_module_exclude_aux"
#define CORRYVRECKAN_TYPE_FUNCTION "corryvreckan_detector_types"

using namespace corryvreckan;

// Default constructor
ModuleManager::ModuleManager() : m_reference(nullptr), m_terminate(false) {
    // New clipboard for storage:
    m_clipboard = std::make_shared<Clipboard>();
}

void ModuleManager::load(ConfigManager* conf_mgr) {
    conf_manager_ = conf_mgr;

    load_detectors();
    load_modules();
}

void ModuleManager::load_detectors() {
    Configuration& global_config = conf_manager_->getGlobalConfiguration();

    for(auto& detector_section : conf_manager_->getDetectorConfigurations()) {

        std::string name = detector_section.getName();

        // Check if we have a duplicate:
        if(std::find_if(m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) {
               return obj->getName() == name;
           }) != m_detectors.end()) {
            throw InvalidValueError(
                global_config, "detectors_file", "Detector " + detector_section.getName() + " defined twice");
        }

        LOG_PROGRESS(STATUS, "DET_LOAD_LOOP") << "Loading detector " << name;
        // the default coordinates is cartesian, any other type is forbidden now
        // @to do: other detector types, e.g., ATLAS endcap strip detector
        auto detector = Detector::factory(detector_section);

        // Check if we already found a reference plane:
        if(m_reference != nullptr && detector->isReference()) {
            throw InvalidValueError(global_config, "detectors_file", "Found more than one reference detector");
        }

        // Switch flag if we found the reference plane:
        if(detector->isReference()) {
            m_reference = detector;
        }

        // Add the new detector to the global list:
        m_detectors.push_back(detector);
    }

    // Check that exactly one detector is marked as reference:
    if(m_reference == nullptr) {
        throw InvalidValueError(global_config, "detectors_file", "Found no detector marked as reference");
    }

    LOG_PROGRESS(STATUS, "DET_LOAD_LOOP") << "Loaded " << m_detectors.size() << " detectors";

    // Finally, sort the list of detectors by z position (from lowest to highest)
    std::sort(m_detectors.begin(), m_detectors.end(), [](std::shared_ptr<Detector> det1, std::shared_ptr<Detector> det2) {
        return det1->displacement().Z() < det2->displacement().Z();
    });
}

void ModuleManager::load_modules() {
    auto& configs = conf_manager_->getModuleConfigurations();
    Configuration& global_config = conf_manager_->getGlobalConfiguration();

    // (Re)create the main ROOT file
    global_config.setAlias("histogram_file", "histogramFile");
    auto path = std::string(gSystem->pwd()) + "/" + global_config.get<std::string>("histogram_file", "histograms");
    path = std::filesystem::path(path).replace_extension("root");

    if(std::filesystem::is_regular_file(path)) {
        if(global_config.get<bool>("deny_overwrite", false)) {
            throw RuntimeError("Overwriting of existing main ROOT file " + path + " denied");
        }
        LOG(WARNING) << "Main ROOT file " << path << " exists and will be overwritten.";
        std::filesystem::remove(path);
    }
    m_histogramFile = std::make_unique<TFile>(path.c_str(), "RECREATE");
    if(m_histogramFile->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + path);
    }
    m_histogramFile->cd();

    // Update the histogram file path:
    global_config.set("histogram_file", path);

    LOG(DEBUG) << "Start loading modules, have " << configs.size() << " configurations.";
    // Loop through all non-global configurations
    for(auto& config : configs) {
        // Load library for each module. Libraries are named (by convention + CMAKE libCorryvreckanModule Name.suffix
        std::string lib_name =
            std::string(CORRYVRECKAN_MODULE_PREFIX).append(config.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG_PROGRESS(STATUS, "MOD_LOAD_LOOP") << "Loading module " << config.getName();

        void* lib = nullptr;
        bool load_error = false;
        dlerror();
        if(loaded_libraries_.count(lib_name) == 0) {
            // If library is not loaded then try to load it first from the config
            // directories
            if(global_config.has("library_directories")) {
                auto lib_paths = global_config.getPathArray("library_directories", true);
                for(auto& lib_path : lib_paths) {
                    std::string full_lib_path = lib_path;
                    full_lib_path += "/";
                    full_lib_path += lib_name;

                    // Check if the absolute file exists and try to load if it exists
                    std::ifstream check_file(full_lib_path);
                    if(check_file.good()) {
                        lib = dlopen(full_lib_path.c_str(), RTLD_NOW);
                        if(lib != nullptr) {
                            LOG(DEBUG) << "Found library in configuration specified directory at " << full_lib_path;
                        } else {
                            load_error = true;
                        }
                        break;
                    }
                }
            }

            // Otherwise try to load from the standard paths if not found already
            if(!load_error && lib == nullptr) {
                lib = dlopen(lib_name.c_str(), RTLD_NOW);

                if(lib != nullptr) {
                    LOG(TRACE) << "Opened library";
                    Dl_info dl_info;
                    dl_info.dli_fname = "";

                    // workaround to get the location of the library
                    int ret = dladdr(dlsym(lib, CORRYVRECKAN_GENERATOR_FUNCTION), &dl_info);
                    if(ret != 0) {
                        LOG(DEBUG) << "Found library during global search in runtime paths at " << dl_info.dli_fname;
                    } else {
                        LOG(WARNING) << "Found library during global search but could not "
                                        "deduce location, likely broken library";
                    }
                } else {
                    load_error = true;
                }
            }
        } else {
            // Otherwise just fetch it from the cache
            lib = loaded_libraries_[lib_name];
        }

        // If library did not load then throw exception
        if(load_error) {
            const char* lib_error = dlerror();

            // Find the name of the loaded library if it exists
            std::string lib_error_str = lib_error;
            size_t end_pos = lib_error_str.find(':');
            std::string problem_lib;
            if(end_pos != std::string::npos) {
                problem_lib = lib_error_str.substr(0, end_pos);
            }

            // FIXME is checking the error in this way portable?
            if(lib_error != nullptr && std::strstr(lib_error, "cannot allocate memory in static TLS block") != nullptr) {
                LOG(ERROR) << "Library could not be loaded: not enough thread local storage "
                              "available"
                           << std::endl
                           << "Try one of below workarounds:" << std::endl
                           << "- Rerun library with the environmental variable LD_PRELOAD='" << problem_lib << "'"
                           << std::endl
                           << "- Recompile the library " << problem_lib << " with tls-model=global-dynamic";
            } else if(lib_error != nullptr && std::strstr(lib_error, "cannot open shared object file") != nullptr &&
                      problem_lib.find(CORRYVRECKAN_MODULE_PREFIX) == std::string::npos) {
                LOG(ERROR) << "Library could not be loaded: one of its dependencies is missing" << std::endl
                           << "The name of the missing library is " << problem_lib << std::endl
                           << "Please make sure the library is properly initialized and try "
                              "again";
            } else {
                LOG(ERROR) << "Library could not be loaded: it is not available" << std::endl
                           << " - Did you enable the library during building? " << std::endl
                           << " - Did you spell the library name correctly (case-sensitive)? ";
                if(lib_error != nullptr) {
                    LOG(DEBUG) << "Detailed error: " << lib_error;
                }
            }

            throw corryvreckan::DynamicLibraryError("Error loading " + config.getName());
        }
        // Remember that this library was loaded
        loaded_libraries_[lib_name] = lib;

        // Check if this module is produced once, or once per detector
        void* globalFunction = dlsym(loaded_libraries_[lib_name], CORRYVRECKAN_GLOBAL_FUNCTION);
        void* dutFunction = dlsym(loaded_libraries_[lib_name], CORRYVRECKAN_DUT_FUNCTION);
        void* auxFunction = dlsym(loaded_libraries_[lib_name], CORRYVRECKAN_AUX_FUNCTION);
        void* typeFunction = dlsym(loaded_libraries_[lib_name], CORRYVRECKAN_TYPE_FUNCTION);

        // If the global function was not found, throw an error
        if(globalFunction == nullptr || dutFunction == nullptr || auxFunction == nullptr || typeFunction == nullptr) {
            LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
            throw corryvreckan::DynamicLibraryError(config.getName());
        }

        bool global = reinterpret_cast<bool (*)()>(globalFunction)();      // NOLINT
        bool dut_only = reinterpret_cast<bool (*)()>(dutFunction)();       // NOLINT
        bool exclude_aux = reinterpret_cast<bool (*)()>(auxFunction)();    // NOLINT
        char* type_tokens = reinterpret_cast<char* (*)()>(typeFunction)(); // NOLINT

        std::vector<std::string> types = get_type_vector(type_tokens);

        // Add the global internal parameters to the configuration
        std::string global_dir = gSystem->pwd();
        config.set<std::string>("_global_dir", global_dir);

        // Create the modules from the library
        std::vector<std::pair<ModuleIdentifier, Module*>> mod_list;
        if(global) {
            mod_list.emplace_back(create_unique_module(loaded_libraries_[lib_name], config));
        } else {
            mod_list = create_detector_modules(loaded_libraries_[lib_name], config, dut_only, exclude_aux, types);
        }

        // Loop through all created instantiations
        for(auto& id_mod : mod_list) {
            // FIXME: This convert the module to an unique pointer. Check that this always works and we can do this earlier
            std::unique_ptr<Module> mod(id_mod.second);
            ModuleIdentifier identifier = id_mod.first;

            // Check if the unique instantiation already exists
            auto iter = id_to_module_.find(identifier);
            if(iter != id_to_module_.end()) {
                // Unique name exists, check if its needs to be replaced
                if(identifier.getPriority() < iter->first.getPriority()) {
                    // Priority of new instance is higher, replace the instance
                    LOG(TRACE) << "Replacing model instance " << iter->first.getUniqueName()
                               << " with instance with higher priority.";

                    module_execution_time_.erase(iter->second->get());
                    iter->second = m_modules.erase(iter->second);
                    iter = id_to_module_.erase(iter);
                } else {
                    // Priority is equal, raise an error
                    if(identifier.getPriority() == iter->first.getPriority()) {
                        throw AmbiguousInstantiationError(config.getName());
                    }
                    // Priority is lower, do not add this module to the run list
                    continue;
                }
            }

            // Save the identifier in the module
            mod->set_identifier(identifier);
            mod->setReference(m_reference);

            // Add the new module to the run list
            m_modules.emplace_back(std::move(mod));
            id_to_module_[identifier] = --m_modules.end();
        }
    }

    LOG_PROGRESS(STATUS, "MOD_LOAD_LOOP") << "Loaded " << m_modules.size() << " module instances";
}

std::vector<std::string> ModuleManager::get_type_vector(char* type_tokens) {
    std::vector<std::string> types;

    std::stringstream tokenstream(type_tokens);
    while(tokenstream.good()) {
        std::string token;
        getline(tokenstream, token, ',');
        if(token.empty()) {
            continue;
        }
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        types.push_back(token);
    }
    return types;
}

std::shared_ptr<Detector> ModuleManager::get_detector(std::string name) {
    auto it = find_if(
        m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) { return obj->getName() == name; });
    return (it != m_detectors.end() ? (*it) : nullptr);
}

std::pair<ModuleIdentifier, Module*> ModuleManager::create_unique_module(void* library, Configuration& config) {
    // Create the identifier
    ModuleIdentifier identifier(config.getName(), "", 0);

    // Return error if user tried to specialize the unique module:
    if(config.has("name")) {
        throw InvalidValueError(config, "name", "unique modules cannot be specialized using the \"name\" keyword.");
    }
    if(config.has("type")) {
        throw InvalidValueError(config, "type", "unique modules cannot be specialized using the \"type\" keyword.");
    }

    LOG(TRACE) << "Creating module " << identifier.getUniqueName() << ", using generator \""
               << CORRYVRECKAN_GENERATOR_FUNCTION << "\"";

    // Get the generator function for this module
    void* generator = dlsym(library, CORRYVRECKAN_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required "
                      "interface function not found!";
        throw corryvreckan::RuntimeError("Error instantiating module from " + config.getName());
    }

    // Create and add module instance config
    Configuration& instance_config = conf_manager_->addInstanceConfiguration(identifier, config);

    // Specialize instance configuration
    auto output_dir = instance_config.get<std::string>("_global_dir");
    output_dir += "/";
    std::string path_mod_name = identifier.getUniqueName();
    std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '_');
    output_dir += path_mod_name;

    // Convert to correct generator function
    auto module_generator =
        reinterpret_cast<Module* (*)(Configuration&, std::vector<std::shared_ptr<Detector>>)>(generator); // NOLINT

    // Set the log section header
    std::string old_section_name = Log::getSection();
    std::string section_name = "C:";
    section_name += identifier.getUniqueName();
    Log::setSection(section_name);
    // Set module specific log settings
    auto old_settings = set_module_before(identifier.getUniqueName(), instance_config);
    // Build module
    Module* module = module_generator(instance_config, m_detectors);
    // Reset log
    Log::setSection(old_section_name);
    set_module_after(old_settings);

    // Set the module directory afterwards to catch invalid access in constructor
    module->get_configuration().set<std::string>("_output_dir", output_dir);

    // Return the module to the ModuleManager
    return std::make_pair(identifier, module);
}

std::vector<std::pair<ModuleIdentifier, Module*>> ModuleManager::create_detector_modules(
    void* library, Configuration& config, bool dut_only, bool exclude_aux, std::vector<std::string> types) {
    LOG(TRACE) << "Creating instantiations for module " << config.getName() << ", using generator \""
               << CORRYVRECKAN_GENERATOR_FUNCTION << "\"";

    // Get the generator function for this module
    void* generator = dlsym(library, CORRYVRECKAN_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required "
                      "interface function not found!";
        throw corryvreckan::RuntimeError("Error instantiating module from " + config.getName());
    }

    // Convert to correct generator function
    auto module_generator = reinterpret_cast<Module* (*)(Configuration&, std::shared_ptr<Detector>)>(generator); // NOLINT
    auto module_base_name = config.getName();

    // Figure out which detectors should run on this module:
    bool instances_created = false;
    std::vector<std::pair<std::shared_ptr<Detector>, ModuleIdentifier>> instantiations;

    // Create all names first with highest priority
    std::set<std::string> module_names;
    if(config.has("name")) {
        std::vector<std::string> names = config.getArray<std::string>("name");
        for(auto& name : names) {
            auto det = get_detector(name);
            if(det == nullptr) {
                continue;
            }

            LOG(TRACE) << "Preparing \"name\" instance for " << det->getName();
            instantiations.emplace_back(det, ModuleIdentifier(module_base_name, det->getName(), 0));
            // Save the name (to not instantiate it again later)
            module_names.insert(name);
        }
        instances_created = !names.empty();
    }

    // Then create all types that are not yet name instantiated
    if(config.has("type")) {
        // Prepare type list from configuration:
        std::vector<std::string> ctypes = config.getArray<std::string>("type");
        for(auto& type : ctypes) {
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        }

        // Check that this is possible at all:
        std::vector<std::string> intersection;
        std::set_intersection(ctypes.begin(), ctypes.end(), types.begin(), types.end(), std::back_inserter(intersection));
        if(!types.empty() && intersection.empty()) {
            throw InvalidInstantiationError(module_base_name, "type conflict");
        }

        for(auto& det : m_detectors) {
            auto detectortype = det->getType();
            std::transform(detectortype.begin(), detectortype.end(), detectortype.begin(), ::tolower);

            // Skip all detectors marked as passive
            if(det->hasRole(DetectorRole::PASSIVE)) {
                continue;
            }

            // Skip all that were already added by name
            if(module_names.find(det->getName()) != module_names.end()) {
                continue;
            }
            for(auto& type : ctypes) {
                // Skip all with wrong type
                if(detectortype != type) {
                    continue;
                }

                LOG(TRACE) << "Preparing \"type\" instance for " << det->getName();
                instantiations.emplace_back(det, ModuleIdentifier(module_base_name, det->getName(), 1));
            }
        }
        instances_created = !ctypes.empty();
    }

    // Create for all detectors if no name / type provided
    if(!instances_created) {
        for(auto& det : m_detectors) {
            LOG(TRACE) << "Preparing \"other\" instance for " << det->getName();
            instantiations.emplace_back(det, ModuleIdentifier(module_base_name, det->getName(), 2));
        }
    }

    std::vector<std::pair<ModuleIdentifier, Module*>> modules;
    for(const auto& instance : instantiations) {
        auto detector = instance.first;
        auto identifier = instance.second;

        // If this should only be instantiated for DUTs, skip otherwise:
        if(dut_only && !detector->isDUT()) {
            LOG(TRACE) << "Skipping instantiation \"" << identifier.getUniqueName() << "\", detector is no DUT";
            continue;
        }

        if(exclude_aux && detector->isAuxiliary()) {
            LOG(TRACE) << "Skipping instantiation \"" << identifier.getUniqueName() << "\", detector is auxiliary";
            continue;
        }

        // Do not instantiate module if detector type is not mentioned as supported:
        auto detectortype = detector->getType();
        std::transform(detectortype.begin(), detectortype.end(), detectortype.begin(), ::tolower);
        if(!types.empty() && std::find(types.begin(), types.end(), detectortype) == types.end()) {
            LOG(TRACE) << "Skipping instantiation \"" << identifier.getUniqueName() << "\", detector type mismatch";
            continue;
        }
        LOG(DEBUG) << "Creating instantiation \"" << identifier.getUniqueName() << "\"";

        // Create and add module instance config
        Configuration& instance_config = conf_manager_->addInstanceConfiguration(instance.second, config);

        // Add internal module config
        auto output_dir = instance_config.get<std::string>("_global_dir");
        output_dir += "/";
        std::string path_mod_name = instance.second.getUniqueName();
        std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '/');
        output_dir += path_mod_name;

        // Set the log section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "C:";
        section_name += identifier.getUniqueName();
        Log::setSection(section_name);
        // Set module specific log settings
        auto old_settings = set_module_before(identifier.getUniqueName(), instance_config);
        // Build module
        Module* module = module_generator(instance_config, detector);
        // Reset log
        Log::setSection(old_section_name);
        set_module_after(old_settings);

        // Set the module directory afterwards to catch invalid access in constructor
        module->get_configuration().set<std::string>("_output_dir", output_dir);

        modules.emplace_back(identifier, module);
    }

    // Return the list of modules to the analysis
    return modules;
}

// Run the analysis loop - this initializes, runs and finalizes all modules
void ModuleManager::run() {
    Configuration& global_config = conf_manager_->getGlobalConfiguration();

    // Check if we have an event or track limit:
    auto number_of_events = global_config.get<int>("number_of_events", -1);
    auto number_of_tracks = global_config.get<int>("number_of_tracks", -1);
    auto eventloop_print_freq = global_config.get<int>("status_print_frequency", 100);

    auto run_time = global_config.get<double>("run_time", static_cast<double>(Units::convert(-1.0, "s")));

    // Loop over all events, running each module on each "event"
    LOG(STATUS) << "========================| Event loop |========================";
    m_events = 0;
    m_tracks = 0;
    m_pixels = 0;

    while(1) {
        bool run = true;

        // Run all modules
        for(auto& module : m_modules) {
            // Get current time
            auto start = std::chrono::steady_clock::now();

            // Set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += module->getUniqueName();
            Log::setSection(section_name);
            // Set module specific settings
            auto old_settings = set_module_before(module->getUniqueName(), module->get_configuration());
            // Change to the output file directory
            module->getROOTDirectory()->cd();

            StatusCode check = module->run(m_clipboard);

            // Reset logging
            Log::setSection(old_section_name);
            set_module_after(old_settings);

            // Update execution time
            auto end = std::chrono::steady_clock::now();
            module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();

            if(check == StatusCode::DeadTime) {
                // If status code indicates dead time, just silently continue with next event:
                break;
            } else if(check == StatusCode::Failure) {
                // If the status code indicates failure, break immediately and finish:
                run = false;
                break;
            } else if(check == StatusCode::EndRun) {
                // If the returned status code asks for end-of-run, finish module list and finish:
                run = false;
            }
        }

        // Increment event number
        m_events++;

        // Print statistics:
        m_tracks += static_cast<int>(m_clipboard->countObjects<Track>());
        m_pixels += static_cast<int>(m_clipboard->countObjects<Pixel>());

        if(m_events % eventloop_print_freq == 0) {

            auto kilo_or_mega = [](const double& input) {
                bool mega = (input > 1e6 ? true : false);
                auto value = (mega ? input * 1e-6 : input * 1e-3);
                std::stringstream output;
                output << std::fixed << std::setprecision(mega ? 2 : 1) << value << (mega ? "M" : "k");
                return output.str();
            };

            LOG_PROGRESS(STATUS, "event_loop")
                << "Ev: " << kilo_or_mega(m_events) << " "
                << "Px: " << kilo_or_mega(m_pixels) << " "
                << "Tr: " << kilo_or_mega(m_tracks) << " (" << std::setprecision(3)
                << (static_cast<double>(m_tracks) / m_events) << "/ev)"
                << (m_clipboard->isEventDefined()
                        ? " t = " + Units::display(m_clipboard->getEvent()->start(), {"ns", "us", "ms", "s"})
                        : "");
        }

        // Check if we have reached the maximum number of events
        if(number_of_events > -1 && m_events >= number_of_events) {
            break;
        }

        if(m_clipboard->isEventDefined() && run_time > 0.0 && m_clipboard->getEvent()->start() >= run_time) {
            break;
        }

        // Check if we have reached the maximum number of tracks
        if(number_of_tracks > -1 && m_tracks >= number_of_tracks) {
            break;
        }

        // Check if any of the modules return a value saying it should stop
        if(!run) {
            break;
        }

        // Check for user termination and stop the event loop:
        if(m_terminate) {
            break;
        }

        // Clear objects from this iteration from the clipboard
        m_clipboard->clear();
    }
}

void ModuleManager::terminate() {
    m_terminate = true;
}

// Initialise all modules
void ModuleManager::initializeAll() {
    // Loop over all modules and initialize them
    LOG(STATUS) << "=================| Initializing modules |==================";
    for(auto& module : m_modules) {
        // Pass the config manager to this instance
        module->set_config_manager(conf_manager_);

        // Create main ROOT directory for this module class if it does not exists yet
        LOG(TRACE) << "Creating and accessing ROOT directory";
        std::string module_name = module->get_configuration().getName();
        auto directory = m_histogramFile->GetDirectory(module_name.c_str());
        if(directory == nullptr) {
            directory = m_histogramFile->mkdir(module_name.c_str());
            if(directory == nullptr) {
                throw RuntimeError("Cannot create or access overall ROOT directory for module " + module_name);
            }
        }
        directory->cd();

        // Create local directory for this instance
        TDirectory* local_directory = nullptr;
        if(module->get_identifier().getIdentifier().empty()) {
            local_directory = directory;
        } else {
            local_directory = directory->mkdir(module->get_identifier().getIdentifier().c_str());
            if(local_directory == nullptr) {
                throw RuntimeError("Cannot create or access local ROOT directory for module " + module->getUniqueName());
            }
        }

        // Change to the directory and save it in the module
        local_directory->cd();
        module->set_ROOT_directory(local_directory);

        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += module->getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->getUniqueName(), module->get_configuration());
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();

        LOG_PROGRESS(STATUS, "MOD_INIT_LOOP") << "Initializing \"" << module->getUniqueName() << "\"";
        // Initialize the module
        module->initialize();

        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }
}

// Finalise all modules
void ModuleManager::finalizeAll() {
    Configuration& global_config = conf_manager_->getGlobalConfiguration();

    // Create read-only version of permanent storage element from event clipboard:
    auto readonly_clipboard = std::static_pointer_cast<ReadonlyClipboard>(m_clipboard);

    // Loop over all modules and finalize them
    LOG(STATUS) << "===================| Finalising modules |===================";
    for(auto& module : m_modules) {
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += module->getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->getUniqueName(), module->get_configuration());
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();

        // Finalise the module
        module->finalize(readonly_clipboard);

        // Store all ROOT objects:
        module->getROOTDirectory()->Write();

        // Remove the pointer to the ROOT directory after finalizing
        module->set_ROOT_directory(nullptr);

        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }

    // Write the output histogram file
    m_histogramFile->Close();

    LOG(STATUS) << "Wrote histogram output file to " << global_config.getPath("histogram_file");

    // Write out update detectors file:
    if(global_config.has("detectors_file_updated")) {
        std::string path = global_config.getPath("detectors_file_updated");
        // Check if the file exists
        if(std::filesystem::is_regular_file(path)) {
            if(global_config.get<bool>("deny_overwrite", false)) {
                throw RuntimeError("Overwriting of existing detectors file " + path + " denied");
            }
            LOG(WARNING) << "Detectors file " << path << " exists and will be overwritten.";
            std::filesystem::remove(path);
        }

        std::ofstream file(path);
        if(!file) {
            throw RuntimeError("Cannot create detectors file " + path);
        }

        ConfigReader final_detectors;
        for(auto& detector : m_detectors) {
            final_detectors.addConfiguration(detector->getConfiguration());
        }

        final_detectors.write(file);
        LOG(STATUS) << "Wrote updated detector configuration to " << path;
    }

    // Check for unused configuration keys:
    auto unused_keys = global_config.getUnusedKeys();
    if(!unused_keys.empty()) {
        std::stringstream st;
        st << "Unused configuration keys in global section:";
        for(auto& key : unused_keys) {
            st << std::endl << key;
        }
        LOG(WARNING) << st.str();
    }
    for(auto& config : conf_manager_->getInstanceConfigurations()) {
        auto unique_name = config.getName();
        auto identifier = config.get<std::string>("identifier");
        if(!identifier.empty()) {
            unique_name += ":";
            unique_name += identifier;
        }
        auto cfg_unused_keys = config.getUnusedKeys();
        if(!cfg_unused_keys.empty()) {
            std::stringstream st;
            st << "Unused configuration keys in section " << unique_name << ":";
            for(auto& key : cfg_unused_keys) {
                st << std::endl << key;
            }
            LOG(WARNING) << st.str();
        }
    }

    // Check the timing for all events
    timing();
}

// Display timing statistics for each module, over all events and per event
void ModuleManager::timing() {
    LOG(STATUS) << "===============| Wall-clock timing (seconds) |================";
    for(auto& module : m_modules) {
        auto identifier = module->get_identifier().getIdentifier();
        LOG(STATUS) << std::setw(20) << module->get_configuration().getName() << (identifier.empty() ? "   " : " : ")
                    << std::setw(10) << identifier << "  --  " << std::fixed << std::setprecision(5)
                    << module_execution_time_[module.get()] << "s = " << std::setprecision(6)
                    << 1000 * module_execution_time_[module.get()] / m_events << "ms/evt";
    }
    LOG(STATUS) << "==============================================================";
}

// Helper functions to set the module specific log settings if necessary
std::tuple<LogLevel, LogFormat> ModuleManager::set_module_before(const std::string&, const Configuration& config) {
    // Set new log level if necessary
    LogLevel prev_level = Log::getReportingLevel();
    if(config.has("log_level")) {
        std::string log_level_string = config.get<std::string>("log_level");
        std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
        try {
            LogLevel log_level = Log::getLevelFromString(log_level_string);
            if(log_level != prev_level) {
                LOG(TRACE) << "Local log level is set to " << log_level_string;
                Log::setReportingLevel(log_level);
            }
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config, "log_level", e.what());
        }
    }

    // Set new log format if necessary
    LogFormat prev_format = Log::getFormat();
    if(config.has("log_format")) {
        std::string log_format_string = config.get<std::string>("log_format");
        std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
        try {
            LogFormat log_format = Log::getFormatFromString(log_format_string);
            if(log_format != prev_format) {
                LOG(TRACE) << "Local log format is set to " << log_format_string;
                Log::setFormat(log_format);
            }
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config, "log_format", e.what());
        }
    }

    return std::make_tuple(prev_level, prev_format);
}
void ModuleManager::set_module_after(std::tuple<LogLevel, LogFormat> prev) {
    // Reset the previous log level
    LogLevel cur_level = Log::getReportingLevel();
    LogLevel old_level = std::get<0>(prev);
    if(cur_level != old_level) {
        Log::setReportingLevel(old_level);
        LOG(TRACE) << "Reset log level to global level of " << Log::getStringFromLevel(old_level);
    }

    // Reset the previous log format
    LogFormat cur_format = Log::getFormat();
    LogFormat old_format = std::get<1>(prev);
    if(cur_format != old_format) {
        Log::setFormat(old_format);
        LOG(TRACE) << "Reset log format to global level of " << Log::getStringFromFormat(old_format);
    }
}
