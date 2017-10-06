#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "core/Analysis.h"
#include "core/config/ConfigManager.hpp"
#include "core/utils/exceptions.h"
/**
 * @file
 * @brief Executable running the framework
 */

#include "core/utils/log.h"

using namespace corryvreckan;

void clean();
void abort_handler(int);
void interrupt_handler(int);

std::unique_ptr<Analysis> corry;
std::atomic<bool> cv_ready{false};

/**
 * @brief Handle user abort (CTRL+\) which should stop the framework immediately
 * @note This handler is actually not fully reliable (but otherwise crashing is okay...)
 */
void abort_handler(int) {
    // Output interrupt message and clean
    LOG(FATAL) << "Aborting!";
    clean();

    // Ignore any segmentation fault that may arise after this
    std::signal(SIGSEGV, SIG_IGN); // NOLINT
    std::abort();
}

/**
 * @brief Handle termination request (CTRL+C) as soon as possible while keeping the program flow
 */
void interrupt_handler(int) {
    // Stop the framework if it is loaded
    if(cv_ready) {
        LOG(STATUS) << "Interrupted! Finishing up current event...";
        corry->terminate();
    }
}

/**
 * @brief Clean the environment when closing application
 */
void clean() {
    Log::finish();
    if(cv_ready) {
        corry.reset();
    }
}

/**
 * @brief Main function running the application
 */
int main(int argc, const char* argv[]) {
    // Add cout as the default logging stream
    Log::addStream(std::cout);

    // Install abort handler (CTRL+\)
    std::signal(SIGQUIT, abort_handler);

    // Install interrupt handler (CTRL+C)
    std::signal(SIGINT, interrupt_handler);

    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    // Parse arguments
    std::string config_file_name;
    std::string log_file_name;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                LogLevel log_level = Log::getLevelFromString(std::string(argv[++i]));
                Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
            }
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            config_file_name = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            log_file_name = std::string(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument \"" << argv[i] << "\"";
            print_help = true;
            return_code = 1;
        }
    }

    // Print help if requested or no arguments given
    if(print_help) {
        std::cout << "Usage: corry -c <config> [-v <level>]" << std::endl;
        std::cout << "\t -c <file>    configuration file to be used" << std::endl;
        std::cout << "\t -l <file>    file to log to besides standard output" << std::endl;
        std::cout << "\t -v <level>   verbosity level overwrites global level,\n"
                  << "\t              but not the per-module configuration." << std::endl;
        clean();
        return return_code;
    }

    // Check if we have a configuration file
    if(config_file_name.empty()) {
        LOG(FATAL) << "No configuration file provided! See usage info with \"corry -h\"";
        clean();
        return 1;
    }

    // Add an extra file to log too if possible
    // NOTE: this stream should be available for the duration of the logging
    std::ofstream log_file;
    if(!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios_base::out | std::ios_base::trunc);
        if(!log_file.good()) {
            LOG(FATAL) << "Cannot write to provided log file! Check if permissions are sufficient.";
            clean();
            return 1;
        }

        Log::addStream(log_file);
    }

    try {
        // Construct main Allpix object
        corry = std::make_unique<Analysis>(config_file_name);
        cv_ready = true;

        // Load modules
        corry->load();

        // Initialize modules (pre-run)
        corry->initialiseAll();

        // Run modules and event-loop
        corry->run();

        // Finalize modules (post-run)
        corry->finaliseAll();
    } catch(ConfigurationError& e) {
        LOG(FATAL) << "Error in the configuration file:" << std::endl
                   << " " << e.what() << std::endl
                   << "The configuration file needs to be updated! Cannot continue...";
        return_code = 1;
    } catch(RuntimeError& e) {
        LOG(FATAL) << "Error during execution of run:" << std::endl
                   << " " << e.what() << std::endl
                   << "Please check your configuration and modules! Cannot continue...";
        return_code = 1;
    } catch(LogicError& e) {
        LOG(FATAL) << "Error in the logic of module:" << std::endl
                   << " " << e.what() << std::endl
                   << "Module has to be properly defined! Cannot continue...";
        return_code = 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << "   " << e.what() << std::endl << "Cannot continue...";
        return_code = 127;
    }

    // Finish the logging
    clean();

    return return_code;
}
