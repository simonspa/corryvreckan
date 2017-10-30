// ROOT include files
#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include <TFile.h>
#include <TSystem.h>

// Local include files
#include "Analysis.h"
#include "algorithm/exceptions.h"
#include "utils/log.h"

#include <dlfcn.h>
#include <fstream>
#include <iomanip>

#define CORRYVRECKAN_ALGORITHM_PREFIX "libCorryvreckanAlgorithm"
#define CORRYVRECKAN_GENERATOR_FUNCTION "corryvreckan_algorithm_generator"

using namespace corryvreckan;

// Default constructor
Analysis::Analysis(std::string config_file_name, std::vector<std::string> options) : m_terminate(false) {

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
    load_detectors();
    load_algorithms();
}

void Analysis::load_detectors() {
    std::string detectors_file = global_config.getPath("detectors_file");
    std::fstream file(detectors_file);
    ConfigReader reader(file, detectors_file);

    for(auto& detector : reader.getConfigurations()) {
        LOG(INFO) << "Detector: " << detector.getName();
        Detector* det_parm = new Detector(detector);

        // Add the new detector to the global list:
        detectors.push_back(det_parm);
    }
    LOG(STATUS) << "Loaded " << detectors.size() << " detectors";

    // Finally, sort the list of detectors by z position (from lowest to highest)
    // FIXME reimplement - std::sort(m_parameters->detectors.begin(), m_parameters->detectors.end(), sortByZ);
    //
}

void Analysis::load_algorithms() {
    std::vector<Configuration> configs = conf_mgr_->getConfigurations();

    // Create histogram output file
    std::string histogramFile = global_config.getPath("histogramFile");
    m_histogramFile = new TFile(histogramFile.c_str(), "RECREATE");
    m_directory = m_histogramFile->mkdir("corryvreckan");
    if(m_histogramFile->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + histogramFile);
    }

    LOG(DEBUG) << "Start loading algorithms, have " << configs.size() << " configurations.";
    // Loop through all non-global configurations
    for(auto& config : configs) {
        // Load library for each module. Libraries are named (by convention + CMAKE)
        // libAllpixModule Name.suffix
        std::string lib_name =
            std::string(CORRYVRECKAN_ALGORITHM_PREFIX).append(config.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loading algorithm " << config.getName();

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
                      problem_lib.find(CORRYVRECKAN_ALGORITHM_PREFIX) == std::string::npos) {
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

        // Merge the global configuration into the algorithms config:
        config.merge(global_config);

        // Create the algorithms from the library
        m_algorithms.emplace_back(create_algorithm(loaded_libraries_[lib_name], config));
    }
    LOG(STATUS) << "Loaded " << configs.size() << " modules";
}

Algorithm* Analysis::create_algorithm(void* library, Configuration config) {
    LOG(TRACE) << "Creating algorithm " << config.getName() << ", using generator \"" << CORRYVRECKAN_GENERATOR_FUNCTION
               << "\"";

    // Make the vector to return
    std::string algorithm_name = config.getName();

    // Get the generator function for this module
    void* generator = dlsym(library, CORRYVRECKAN_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Algorithm library is invalid or outdated: required "
                      "interface function not found!";
        throw corryvreckan::RuntimeError("Error instantiating algorithm from " + config.getName());
    }

    // Convert to correct generator function
    auto algorithm_generator = reinterpret_cast<Algorithm* (*)(Configuration, std::vector<Detector*>)>(generator); // NOLINT

    // Figure out which detectors should run on this algorithm:
    std::vector<Detector*> algorithm_det;
    if(!config.has("detectors")) {
        algorithm_det = detectors;
    } else {
        std::vector<std::string> det_list = config.getArray<std::string>("detectors");

        for(auto& d : detectors) {
            if(std::find(det_list.begin(), det_list.end(), d->name()) != det_list.end()) {
                algorithm_det.push_back(d);
            }
        }
    }

    // Check for potentially masked detectors:
    if(config.has("masked")) {
        std::vector<std::string> mask_list = config.getArray<std::string>("masked");

        for(auto it = algorithm_det.begin(); it != algorithm_det.end();) {
            // Remove detectors which are masked:
            if(std::find(mask_list.begin(), mask_list.end(), (*it)->name()) != mask_list.end()) {
                it = algorithm_det.erase(it);
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
    auto old_settings = set_algorithm_before(config.getName(), config);
    // Build algorithm
    Algorithm* algorithm = algorithm_generator(config, algorithm_det);
    // Reset log
    Log::setSection(old_section_name);
    set_algorithm_after(old_settings);

    // Return the algorithm to the analysis
    return algorithm;
}

// Run the analysis loop - this initialises, runs and finalises all algorithms
void Analysis::run() {

    // Check if we have an event limit:
    int number_of_events = global_config.get<int>("number_of_events", -1);

    // Loop over all events, running each algorithm on each "event"
    LOG(STATUS) << "========================| Event loop |========================";
    m_events = 1;
    m_tracks = 0;
    int skipped = 0;

    while(1) {
        bool run = true;
        bool noData = false;

        // Check if we have reached the maximum number of events
        if(number_of_events > -1 && m_events >= number_of_events)
            break;

        // Run all algorithms
        for(auto& algorithm : m_algorithms) {
            // Set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += algorithm->getName();
            Log::setSection(section_name);
            // Set module specific settings
            auto old_settings = set_algorithm_before(algorithm->getName(), algorithm->getConfig());
            // Change to the output file directory
            m_directory->cd(algorithm->getName().c_str());
            // Run the algorithms with timing enabled
            algorithm->getStopwatch()->Start(false);
            StatusCode check = algorithm->run(m_clipboard);
            algorithm->getStopwatch()->Stop();
            // Reset logging
            Log::setSection(old_section_name);
            set_algorithm_after(old_settings);
            if(check == NoData) {
                noData = true;
                skipped++;
                break;
            } // Nothing to be done in this event
            if(check == Failure)
                run = false;
        }

        // Print statistics:
        Tracks* tracks = (Tracks*)m_clipboard->get("tracks");
        m_tracks += (tracks == NULL ? 0 : tracks->size());
        if(m_events % 100 == 0 || skipped % 1000 == 0) {
            LOG_PROGRESS(STATUS, "event_loop")
                << "Events processed: " << m_events << ", skipped: " << skipped << " Tracks found: " << m_tracks << " ("
                << ((double)m_tracks / m_events) << " tr/ev)";
        }

        // Clear objects from this iteration from the clipboard
        m_clipboard->clear();
        // Check if any of the algorithms return a value saying it should stop
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

// Initalise all algorithms
void Analysis::initialiseAll() {
    // Loop over all algorithms and initialise them
    LOG(STATUS) << "=================| Initialising algorithms |==================";
    for(auto& algorithm : m_algorithms) {
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += algorithm->getName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_algorithm_before(algorithm->getName(), algorithm->getConfig());

        // Make a new folder in the output file
        m_directory->cd();
        m_directory->mkdir(algorithm->getName().c_str());
        m_directory->cd(algorithm->getName().c_str());
        LOG(INFO) << "Initialising \"" << algorithm->getName() << "\"";
        // Initialise the algorithm
        algorithm->initialise();

        // Reset logging
        Log::setSection(old_section_name);
        set_algorithm_after(old_settings);
    }
}

// Finalise all algorithms
void Analysis::finaliseAll() {

    // Loop over all algorithms and finalise them
    for(auto& algorithm : m_algorithms) {
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += algorithm->getName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_algorithm_before(algorithm->getName(), algorithm->getConfig());

        // Change to the output file directory
        m_directory->cd(algorithm->getName().c_str());
        // Finalise the algorithm
        algorithm->finalise();
        // Reset logging
        Log::setSection(old_section_name);
        set_algorithm_after(old_settings);
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

// Display timing statistics for each algorithm, over all events and per event
void Analysis::timing() {
    LOG(STATUS) << "===============| Wall-clock timing (seconds) |================";
    for(auto& algorithm : m_algorithms) {
        LOG(STATUS) << std::setw(25) << algorithm->getName() << "  --  " << std::fixed << std::setprecision(5)
                    << algorithm->getStopwatch()->RealTime() << " = " << std::setprecision(9)
                    << algorithm->getStopwatch()->RealTime() / m_events << " s/evt";
    }
    LOG(STATUS) << "==============================================================";
}

// Helper functions to set the module specific log settings if necessary
std::tuple<LogLevel, LogFormat> Analysis::set_algorithm_before(const std::string&, const Configuration& config) {
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
void Analysis::set_algorithm_after(std::tuple<LogLevel, LogFormat> prev) {
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
