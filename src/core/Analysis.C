// ROOT include files
#include <TSystem.h>
#include "TFile.h"

// Local include files
#include "Analysis.h"
#include "objects/Timepix3Track.h"
#include "utils/log.h"

#include <dlfcn.h>
#include <fstream>

#define CORRYVRECKAN_ALGORITHM_PREFIX "libCorryvreckanAlgorithm"
#define CORRYVRECKAN_GENERATOR_FUNCTION "corryvreckan_algorithm_generator"

using namespace corryvreckan;

// Default constructor
Analysis::Analysis(std::string config_file_name) : m_terminate(false) {

    // Load the global configuration
    conf_mgr_ = std::make_unique<corryvreckan::ConfigManager>(std::move(config_file_name));

    // Configure the standard special sections
    conf_mgr_->setGlobalHeaderName("Corryvreckan");
    conf_mgr_->addGlobalHeaderName("");
    conf_mgr_->addIgnoreHeaderName("Ignore");

    // Fetch the global configuration
    corryvreckan::Configuration global_config = conf_mgr_->getGlobalConfiguration();

    // FIXME translate new configuration to parameters:
    m_parameters = new Parameters();

    // Define DUT and reference
    m_parameters->DUT = global_config.get<std::string>("DUT");
    m_parameters->reference = global_config.get<std::string>("reference");

    m_parameters->detectorToAlign = m_parameters->DUT;
    m_parameters->excludedFromTracking[m_parameters->DUT] = true;

    std::vector<std::string> excluding = global_config.getArray<std::string>("excludeFromTracking");
    for(auto& ex : excluding) {
        m_parameters->excludedFromTracking[ex] = true;
    }

    std::vector<std::string> masking = global_config.getArray<std::string>("masked");
    for(auto& m : masking) {
        m_parameters->masked[m] = true;
    }

    // FIXME Read remaining parametes from command line:
    // Overwrite steering file values from command line
    // parameters->readCommandLineOptions(argc,argv);

    // Load alignment parameters
    std::string conditionsFile = global_config.get<std::string>("conditionsFile");
    m_parameters->conditionsFile = conditionsFile;
    if(!m_parameters->readConditions())
        throw ConfigFileUnavailableError(conditionsFile);

    // Load mask file for the dut (if specified)
    m_parameters->dutMaskFile = global_config.get<std::string>("dutMaskFile", "defaultMask.dat");
    m_parameters->readDutMask();

    // FIXME per-algorithm settings:
    //   basicTracking->minHitsOnTrack = 7;
    // clicpixAnalysis->timepix3Telescope = true;
    //  spatialTracking->debug = true;
    // testAlgorithm->makeCorrelations = true;
    // dataDump->m_detector = parameters->DUT;

    // New clipboard for storage:
    m_clipboard = new Clipboard();
}

// Add an algorithm to the list of algorithms to run
void Analysis::add(Algorithm* algorithm) {
    m_algorithms.push_back(algorithm);
}

void Analysis::load() {

    std::vector<Configuration> configs = conf_mgr_->getConfigurations();
    Configuration global_config_ = conf_mgr_->getGlobalConfiguration();

    // Create histogram output file
    m_histogramFile = new TFile(m_parameters->histogramFile.c_str(), "RECREATE");
    m_directory = m_histogramFile->mkdir("corryvreckan");
    if(m_histogramFile->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + std::string(m_parameters->histogramFile.c_str()));
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
            if(global_config_.has("library_directories")) {
                std::vector<std::string> lib_paths = global_config_.getPathArray("library_directories", true);
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

            throw corryvreckan::RuntimeError("Error loading " + config.getName());
        }
        // Remember that this library was loaded
        loaded_libraries_[lib_name] = lib;

        // Add the global internal parameters to the configuration
        std::string global_dir = gSystem->pwd();
        config.set<std::string>("_global_dir", global_dir);

        // Create the algorithms from the library
        m_algorithms.emplace_back(create_algorithm(loaded_libraries_[lib_name], config, m_clipboard));
    }
    LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loaded " << configs.size() << " modules";
}

Algorithm* Analysis::create_algorithm(void* library, Configuration config, Clipboard* clipboard) {
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
    auto algorithm_generator = reinterpret_cast<Algorithm* (*)(Configuration, Clipboard*)>(generator); // NOLINT

    // Build algorithm
    Algorithm* algorithm = algorithm_generator(config, clipboard);

    // Return the algorithm to the analysis
    return algorithm;
}

// Run the analysis loop - this initialises, runs and finalises all algorithms
void Analysis::run() {

    // Loop over all events, running each algorithm on each "event"
    LOG(STATUS) << "========================| Event loop |========================";
    m_events = 1;
    while(1) {
        bool run = true;
        bool noData = false;
        // Run all algorithms
        for(int algorithmNumber = 0; algorithmNumber < m_algorithms.size(); algorithmNumber++) {
            // Change to the output file directory
            m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
            // Run the algorithms with timing enabled
            m_algorithms[algorithmNumber]->getStopwatch()->Start(false);
            StatusCode check = m_algorithms[algorithmNumber]->run(m_clipboard);
            m_algorithms[algorithmNumber]->getStopwatch()->Stop();
            if(check == NoData) {
                noData = true;
                break;
            } // Nothing to be done in this event
            if(check == Failure)
                run = false;
        }
        // Count number of tracks produced
        //    Timepix3Tracks* tracks =
        //    (Timepix3Tracks*)m_clipboard->get("Timepix3","tracks");
        //    if(tracks != NULL) nTracks += tracks->size();

        //    LOG(DEBUG) << "\r[Analysis] Current time is
        //    "<<fixed<<setw(10)<<m_parameters->currentTime<<". Produced
        //    "<<nTracks<<" tracks"<<flush;

        // Clear objects from this iteration from the clipboard
        m_clipboard->clear();
        // Check if any of the algorithms return a value saying it should stop
        if(!run)
            break;
        // Check if we have reached the maximum number of events
        if(m_parameters->nEvents > 0 && m_events == m_parameters->nEvents)
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
    if(m_parameters->gui)
        cin.ignore();
}

void Analysis::terminate() {
    m_terminate = true;
}

// Initalise all algorithms
void Analysis::initialiseAll() {
    int nTracks = 0;

    // Loop over all algorithms and initialise them
    LOG(STATUS) << "=================| Initialising algorithms |==================";
    for(int algorithmNumber = 0; algorithmNumber < m_algorithms.size(); algorithmNumber++) {
        // Make a new folder in the output file
        m_directory->cd();
        m_directory->mkdir(m_algorithms[algorithmNumber]->getName().c_str());
        m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
        LOG(INFO) << "Initialising \"" << m_algorithms[algorithmNumber]->getName() << "\"";
        // Initialise the algorithm
        m_algorithms[algorithmNumber]->initialise(m_parameters);
    }
}

// Finalise all algorithms
void Analysis::finaliseAll() {

    // Loop over all algorithms and finalise them
    for(int algorithmNumber = 0; algorithmNumber < m_algorithms.size(); algorithmNumber++) {
        // Change to the output file directory
        m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
        // Finalise the algorithm
        m_algorithms[algorithmNumber]->finalise();
    }

    // Write the output histogram file
    m_directory->cd();
    m_directory->Write();
    m_histogramFile->Close();

    // Check the timing for all events
    timing();
}

// Display timing statistics for each algorithm, over all events and per event
void Analysis::timing() {
    LOG(INFO) << "===============| Wall-clock timing (seconds) |================";
    for(int algorithmNumber = 0; algorithmNumber < m_algorithms.size(); algorithmNumber++) {
        LOG(INFO) << m_algorithms[algorithmNumber]->getName() << "  --  "
                  << m_algorithms[algorithmNumber]->getStopwatch()->RealTime() << " = "
                  << m_algorithms[algorithmNumber]->getStopwatch()->RealTime() / m_events << " s/evt";
    }
    LOG(INFO) << "==============================================================";
}
