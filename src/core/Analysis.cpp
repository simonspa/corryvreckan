/** @file
 *  @brief Interface to the core framework
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
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
#include "Analysis.hpp"
#include "module/exceptions.h"
#include "utils/log.h"

#include <chrono>
#include <dlfcn.h>
#include <fstream>
#include <iomanip>

#define CORRYVRECKAN_MODULE_PREFIX "libCorryvreckanModule"
#define CORRYVRECKAN_GENERATOR_FUNCTION "corryvreckan_module_generator"

using namespace corryvreckan;

// Default constructor
Analysis::Analysis(std::string config_file_name, std::vector<std::string> options) : m_terminate(false) {

    LOG(TRACE) << "Loading Corryvreckan";

    // Put welcome message
    LOG(STATUS) << "Welcome to Corryvreckan " << CORRYVRECKAN_PROJECT_VERSION;

    // Load the global configuration
    conf_mgr_ = std::make_unique<corryvreckan::ConfigManager>(std::move(config_file_name));

    // Configure the standard special sections
    conf_mgr_->setGlobalHeaderName("Corryvreckan");
    conf_mgr_->addGlobalHeaderName("");
    conf_mgr_->addIgnoreHeaderName("Ignore");

    // Parse all command line options
    for(auto& option : options) {
        conf_mgr_->parseOption(option);
    }

    // Fetch the global configuration
    global_config = conf_mgr_->getGlobalConfiguration();

    // Set the log level from config if not specified earlier
    std::string log_level_string;
    if(Log::getReportingLevel() == LogLevel::NONE) {
        log_level_string = global_config.get<std::string>("log_level", "INFO");
        std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
        try {
            LogLevel log_level = Log::getLevelFromString(log_level_string);
            Log::setReportingLevel(log_level);
        } catch(std::invalid_argument& e) {
            LOG(ERROR) << "Log level \"" << log_level_string
                       << "\" specified in the configuration is invalid, defaulting to INFO instead";
            Log::setReportingLevel(LogLevel::INFO);
        }
    } else {
        log_level_string = Log::getStringFromLevel(Log::getReportingLevel());
    }

    // Set the log format from config
    std::string log_format_string = global_config.get<std::string>("log_format", "DEFAULT");
    std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
    try {
        LogFormat log_format = Log::getFormatFromString(log_format_string);
        Log::setFormat(log_format);
    } catch(std::invalid_argument& e) {
        LOG(ERROR) << "Log format \"" << log_format_string
                   << "\" specified in the configuration is invalid, using DEFAULT instead";
        Log::setFormat(LogFormat::DEFAULT);
    }

    // Open log file to write output to
    if(global_config.has("log_file")) {
        // NOTE: this stream should be available for the duration of the logging
        log_file_.open(global_config.getPath("log_file"), std::ios_base::out | std::ios_base::trunc);
        Log::addStream(log_file_);
    }

    // Wait for the first detailed messages until level and format are properly set
    LOG(TRACE) << "Global log level is set to " << log_level_string;
    LOG(TRACE) << "Global log format is set to " << log_format_string;

    // New clipboard for storage:
    m_clipboard = new Clipboard();
}

void Analysis::load() {

    add_units();

    load_detectors();
    load_modules();
}

void Analysis::load_detectors() {

    std::vector<std::string> detectors_files = global_config.getPathArray("detectors_file");

    for(auto& detectors_file : detectors_files) {
        std::fstream file(detectors_file);
        ConfigManager det_mgr(detectors_file);
        det_mgr.addIgnoreHeaderName("Ignore");

        for(auto& detector : det_mgr.getConfigurations()) {

            std::string name = detector.getName();

            // Check if we have a duplicate:
            if(std::find_if(detectors.begin(), detectors.end(), [&name](Detector* obj) { return obj->name() == name; }) !=
               detectors.end()) {
                throw InvalidValueError(
                    global_config, "detectors_file", "Detector " + detector.getName() + " defined twice");
            }

            LOG(INFO) << "Detector: " << name;
            Detector* det_parm = new Detector(detector);

            // Add the new detector to the global list:
            detectors.push_back(det_parm);
        }
    }

    LOG(STATUS) << "Loaded " << detectors.size() << " detectors";

    // Finally, sort the list of detectors by z position (from lowest to highest)
    std::sort(detectors.begin(), detectors.end(), [](Detector* det1, Detector* det2) {
        return det1->displacement().Z() < det2->displacement().Z();
    });
}

void Analysis::load_modules() {
    std::vector<Configuration> configs = conf_mgr_->getConfigurations();

    // Create histogram output file
    std::string histogramFile = global_config.getPath("histogramFile");
    m_histogramFile = new TFile(histogramFile.c_str(), "RECREATE");
    m_directory = m_histogramFile->mkdir("corryvreckan");
    if(m_histogramFile->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + histogramFile);
    }

    LOG(DEBUG) << "Start loading modules, have " << configs.size() << " configurations.";
    // Loop through all non-global configurations
    for(auto& config : configs) {
        // Load library for each module. Libraries are named (by convention + CMAKE libCorryvreckanModule Name.suffix
        std::string lib_name =
            std::string(CORRYVRECKAN_MODULE_PREFIX).append(config.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loading module " << config.getName();

        void* lib = nullptr;
        bool load_error = false;
        dlerror();
        if(loaded_libraries_.count(lib_name) == 0) {
            // If library is not loaded then try to load it first from the config
            // directories
            if(global_config.has("library_directories")) {
                std::vector<std::string> lib_paths = global_config.getPathArray("library_directories", true);
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

        // Apply the module specific options to the module configuration
        conf_mgr_->applyOptions(config.getName(), config);

        // Add the global internal parameters to the configuration
        std::string global_dir = gSystem->pwd();
        config.set<std::string>("_global_dir", global_dir);

        // Merge the global configuration into the modules config:
        config.merge(global_config);

        // Create the modules from the library
        m_modules.emplace_back(create_module(loaded_libraries_[lib_name], config));
    }
    LOG(STATUS) << "Loaded " << configs.size() << " modules";
}

Module* Analysis::create_module(void* library, Configuration config) {
    LOG(TRACE) << "Creating module " << config.getName() << ", using generator \"" << CORRYVRECKAN_GENERATOR_FUNCTION
               << "\"";

    // Make the vector to return
    std::string module_name = config.getName();

    // Get the generator function for this module
    void* generator = dlsym(library, CORRYVRECKAN_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required "
                      "interface function not found!";
        throw corryvreckan::RuntimeError("Error instantiating module from " + config.getName());
    }

    // Convert to correct generator function
    auto module_generator = reinterpret_cast<Module* (*)(Configuration, std::vector<Detector*>)>(generator); // NOLINT

    // Figure out which detectors should run on this module:
    std::vector<Detector*> module_det;
    if(!config.has("detectors")) {
        module_det = detectors;
    } else {
        std::vector<std::string> det_list = config.getArray<std::string>("detectors");

        for(auto& d : detectors) {
            if(std::find(det_list.begin(), det_list.end(), d->name()) != det_list.end()) {
                module_det.push_back(d);
            }
        }
    }

    // Check for potentially masked detectors:
    if(config.has("masked")) {
        std::vector<std::string> mask_list = config.getArray<std::string>("masked");

        for(auto it = module_det.begin(); it != module_det.end();) {
            // Remove detectors which are masked:
            if(std::find(mask_list.begin(), mask_list.end(), (*it)->name()) != mask_list.end()) {
                it = module_det.erase(it);
            } else {
                it++;
            }
        }
    }

    // Set the log section header
    std::string old_section_name = Log::getSection();
    std::string section_name = "C:";
    section_name += config.getName();
    Log::setSection(section_name);
    // Set module specific log settings
    auto old_settings = set_module_before(config.getName(), config);
    // Build module
    Module* module = module_generator(config, module_det);
    // Reset log
    Log::setSection(old_section_name);
    set_module_after(old_settings);

    // Return the module to the analysis
    return module;
}

// Run the analysis loop - this initialises, runs and finalises all modules
void Analysis::run() {

    // Check if we have an event or track limit:
    int number_of_events = global_config.get<int>("number_of_events", -1);
    int number_of_tracks = global_config.get<int>("number_of_tracks", -1);
    float run_time = global_config.get<float>("run_time", Units::convert(-1.0, "s"));

    // Loop over all events, running each module on each "event"
    LOG(STATUS) << "========================| Event loop |========================";
    m_events = 1;
    int events_prev = 1;
    m_tracks = 0;
    int skipped = 0;
    int skipped_prev = 0;

    while(1) {
        bool run = true;
        bool noData = false;

        // Check if we have reached the maximum number of events

        if(number_of_events > -1 && m_events >= number_of_events)
            break;

        if(run_time > 0.0 && (m_clipboard->get_persistent("currentTime")) >= run_time)
            break;

        // Check if we have reached the maximum number of tracks
        if(number_of_tracks > -1 && m_tracks >= number_of_tracks)
            break;

        // Run all modules
        for(auto& module : m_modules) {
            // Get current time
            auto start = std::chrono::steady_clock::now();

            // Set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += module->getName();
            Log::setSection(section_name);
            // Set module specific settings
            auto old_settings = set_module_before(module->getName(), module->getConfig());
            // Change to the output file directory
            m_directory->cd(module->getName().c_str());
            StatusCode check = module->run(m_clipboard);
            // Reset logging
            Log::setSection(old_section_name);
            set_module_after(old_settings);

            // Update execution time
            auto end = std::chrono::steady_clock::now();
            module_execution_time_[module] += static_cast<std::chrono::duration<long double>>(end - start).count();

            if(check == NoData) {
                noData = true;
                skipped++;
                break;
            } // Nothing to be done in this event
            if(check == Failure) {
                run = false;
            }
        }

        // Print statistics:
        Tracks* tracks = (Tracks*)m_clipboard->get("tracks");
        m_tracks += (tracks == NULL ? 0 : tracks->size());

        bool update_progress = false;
        if(m_events % 100 == 0 && m_events != events_prev) {
            update_progress = true;
        }
        if(skipped % 1000 == 0 && skipped != skipped_prev) {
            update_progress = true;
        }

        if(update_progress) {
            skipped_prev = skipped;
            events_prev = m_events;
            LOG_PROGRESS(STATUS, "event_loop")
                << "Ev: +" << m_events << " \\" << skipped << " Tr: " << m_tracks << " (" << std::setprecision(3)
                << ((double)m_tracks / m_events)
                << "/ev) t = " << Units::display(m_clipboard->get_persistent("currentTime"), {"ns", "us", "s"});
        }

        // Clear objects from this iteration from the clipboard
        m_clipboard->clear();
        // Check if any of the modules return a value saying it should stop
        if(!run)
            break;
        // Increment event number
        if(!noData)
            m_events++;

        // Check for user termination and stop the event loop:
        if(m_terminate) {
            break;
        }
    }

    // If running the gui, don't close until the user types a command
    if(global_config.get<bool>("gui", false))
        std::cin.ignore();
}

void Analysis::terminate() {
    m_terminate = true;
}

// Initalise all modules
void Analysis::initialiseAll() {
    // Loop over all modules and initialise them
    LOG(STATUS) << "=================| Initialising modules |==================";
    for(auto& module : m_modules) {
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += module->getName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->getName(), module->getConfig());

        // Make a new folder in the output file
        m_directory->cd();
        m_directory->mkdir(module->getName().c_str());
        m_directory->cd(module->getName().c_str());
        LOG(INFO) << "Initialising \"" << module->getName() << "\"";
        // Initialise the module
        module->initialise();

        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }
}

// Finalise all modules
void Analysis::finaliseAll() {

    // Loop over all modules and finalise them
    LOG(STATUS) << "===================| Finalising modules |===================";
    for(auto& module : m_modules) {
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += module->getName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->getName(), module->getConfig());

        // Change to the output file directory
        m_directory->cd(module->getName().c_str());
        // Finalise the module
        module->finalise();
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }

    // Write the output histogram file
    m_directory->cd();
    m_directory->Write();
    m_histogramFile->Close();

    // Write out update detectors file:
    if(global_config.has("detectors_file_updated")) {
        std::string file_name = global_config.getPath("detectors_file_updated");
        // Check if the file exists
        std::ofstream file(file_name);
        if(!file) {
            throw ConfigFileUnavailableError(file_name);
        }

        ConfigReader final_detectors;
        for(auto& detector : detectors) {
            final_detectors.addConfiguration(detector->getConfiguration());
        }

        final_detectors.write(file);
        LOG(STATUS) << "Wrote updated detector configuration to " << file_name;
    }

    // Check the timing for all events
    timing();
}

// Display timing statistics for each module, over all events and per event
void Analysis::timing() {
    LOG(STATUS) << "===============| Wall-clock timing (seconds) |================";
    for(auto& module : m_modules) {
        LOG(STATUS) << std::setw(25) << module->getName() << "  --  " << std::fixed << std::setprecision(5)
                    << module_execution_time_[module] << " = " << std::setprecision(9)
                    << module_execution_time_[module] / m_events << " s/evt";
    }
    LOG(STATUS) << "==============================================================";
}

// Helper functions to set the module specific log settings if necessary
std::tuple<LogLevel, LogFormat> Analysis::set_module_before(const std::string&, const Configuration& config) {
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
void Analysis::set_module_after(std::tuple<LogLevel, LogFormat> prev) {
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

void Analysis::add_units() {

    LOG(TRACE) << "Adding physical units";

    // LENGTH
    Units::add("nm", 1e-6);
    Units::add("um", 1e-3);
    Units::add("mm", 1);
    Units::add("cm", 1e1);
    Units::add("dm", 1e2);
    Units::add("m", 1e3);
    Units::add("km", 1e6);

    // TIME
    Units::add("ps", 1e-3);
    Units::add("ns", 1);
    Units::add("us", 1e3);
    Units::add("ms", 1e6);
    Units::add("s", 1e9);

    // TEMPERATURE
    Units::add("K", 1);

    // ENERGY
    Units::add("eV", 1e-6);
    Units::add("keV", 1e-3);
    Units::add("MeV", 1);
    Units::add("GeV", 1e3);

    // CHARGE
    Units::add("e", 1);
    Units::add("ke", 1e3);
    Units::add("C", 1.6021766208e-19);

    // VOLTAGE
    // NOTE: fixed by above
    Units::add("V", 1e-6);
    Units::add("kV", 1e-3);

    // ANGLES
    // NOTE: these are fake units
    Units::add("deg", 0.01745329252);
    Units::add("rad", 1);
    Units::add("mrad", 1e-3);
}
