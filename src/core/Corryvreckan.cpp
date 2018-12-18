/** @file
 *  @brief Implementation of interface to the core framework
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Corryvreckan.hpp"

#include <chrono>
#include <climits>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

#include <TROOT.h>
#include <TRandom.h>
#include <TStyle.h>
#include <TSystem.h>

#include "core/config/exceptions.h"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace corryvreckan;

/**
 * This class will own the managers for the lifetime of the simulation. Will do early initialization:
 * - Configure the special header sections.
 * - Set the log level and log format as requested.
 * - Load the detector configuration and parse it
 */
Corryvreckan::Corryvreckan(std::string config_file_name,
                           const std::vector<std::string>& module_options,
                           const std::vector<std::string>& detector_options)
    : terminate_(false), has_run_(false), mod_mgr_(std::make_unique<ModuleManager>()) {

    // Load the global configuration
    conf_mgr_ = std::make_unique<ConfigManager>(std::move(config_file_name),
                                                std::initializer_list<std::string>({"Corryvreckan", ""}),
                                                std::initializer_list<std::string>({"Ignore"}));

    // Load and apply the provided module options
    conf_mgr_->loadModuleOptions(module_options);

    // Load and apply the provided detector options
    conf_mgr_->loadDetectorOptions(detector_options);

    // Fetch the global configuration
    Configuration& global_config = conf_mgr_->getGlobalConfiguration();

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
        LOG(TRACE) << "Added log stream to file " << global_config.getPath("log_file");
        Log::addStream(log_file_);
    }

    // Wait for the first detailed messages until level and format are properly set
    LOG(TRACE) << "Global log level is set to " << log_level_string;
    LOG(TRACE) << "Global log format is set to " << log_format_string;
}

/**
 * Performs the initialization, including:
 * - Determine and create the output directory
 * - Include all the defined units
 * - Load the modules from the configuration
 */
void Corryvreckan::load() {
    LOG(TRACE) << "Loading Corryvreckan";

    // Fetch the global configuration
    Configuration& global_config = conf_mgr_->getGlobalConfiguration();

    // Put welcome message and set version
    LOG(STATUS) << "Welcome to Corryvreckan " << CORRYVRECKAN_PROJECT_VERSION;
    global_config.set<std::string>("version", CORRYVRECKAN_PROJECT_VERSION);

    // Set the default units to use
    add_units();

    // Load the modules from the configuration
    if(!terminate_) {
        mod_mgr_->load(conf_mgr_.get());
    } else {
        LOG(INFO) << "Skip loading modules because termination is requested";
    }
}

/**
 * Runs the Module::init() method linearly for every module
 */
void Corryvreckan::init() {
    if(!terminate_) {
        LOG(TRACE) << "Initializing Corryvreckan";
        mod_mgr_->initialiseAll();
    } else {
        LOG(INFO) << "Skip initializing modules because termination is requested";
    }
}
/**
 * Runs every modules Module::run() method linearly for the number of events
 */
void Corryvreckan::run() {
    if(!terminate_) {
        LOG(TRACE) << "Running Corryvreckan";
        mod_mgr_->run();

        // Set that we have run and want to finalize as well
        has_run_ = true;
    } else {
        LOG(INFO) << "Skip running modules because termination is requested";
    }
}
/**
 * Runs all modules Module::finalize() method linearly for every module
 */
void Corryvreckan::finalize() {
    if(has_run_) {
        LOG(TRACE) << "Finalizing Corryvreckan";
        mod_mgr_->finaliseAll();
    } else {
        LOG(INFO) << "Skip finalizing modules because no module did run";
    }
}

/*
 * This function can be called safely from any signal handler. Time between the request to terminate
 * and the actual termination is not always negigible.
 */
void Corryvreckan::terminate() {
    terminate_ = true;
    mod_mgr_->terminate();
}

void Corryvreckan::add_units() {
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
    Units::add("fC", 1 / 1.6021766208e-4);
    Units::add("C", 1 / 1.6021766208e-19);

    // VOLTAGE
    // NOTE: fixed by above
    Units::add("V", 1e-6);
    Units::add("kV", 1e-3);

    // MAGNETIC FIELD
    Units::add("T", 1e-3);
    Units::add("mT", 1e-6);

    // ANGLES
    // NOTE: these are fake units
    Units::add("deg", 0.01745329252);
    Units::add("rad", 1);
    Units::add("mrad", 1e-3);
}
