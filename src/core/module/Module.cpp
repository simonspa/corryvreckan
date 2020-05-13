/** @file
 *  @brief Implementation of the base module class
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Module.hpp"
#include "core/utils/file.h"

using namespace corryvreckan;

Module::Module(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), std::vector<std::shared_ptr<Detector>>{detector}) {}

Module::~Module() {}

Module::Module(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : m_config(std::move(config)), m_detectors(std::move(detectors)) {}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor
 *
 * This name is guaranteed to be unique for every single instantiation of all modules
 */
std::string Module::getUniqueName() const {
    std::string unique_name = get_identifier().getUniqueName();
    if(unique_name.empty()) {
        throw InvalidModuleActionException("Cannot uniquely identify module in constructor");
    }
    return unique_name;
}

void Module::set_identifier(ModuleIdentifier identifier) {
    identifier_ = std::move(identifier);
}
ModuleIdentifier Module::get_identifier() const {
    return identifier_;
}

/**
 * @throws ModuleError If the file cannot be accessed (or created if it did not yet exist)
 * @throws InvalidModuleActionException If this method is called from the constructor with the global flag false
 * @throws ModuleError If the file exists but the "deny_overwrite" flag is set to true (defaults to false)
 * @warning A local path cannot be fetched from the constructor, because the instantiation logic has not finished yet
 *
 * The output path is automatically created if it does not exists. The path is always accessible if this functions returns.
 * Obeys the "deny_overwrite" parameter of the module.
 */
std::string Module::createOutputFile(const std::string& path, bool global) {
    std::string file;
    if(global) {
        file = m_config.get<std::string>("_global_dir", std::string());
    } else {
        file = m_config.get<std::string>("_output_dir", std::string());
    }

    // The file name will only be empty if this method is executed from the constructor
    if(file.empty()) {
        throw InvalidModuleActionException("Cannot access local output path in constructor");
    }

    try {
        // Create all the required main directories
        corryvreckan::create_directories(file);

        // Add the file itself
        file += "/";
        file += path;

        if(path_is_file(file)) {
            auto global_overwrite = getConfigManager()->getGlobalConfiguration().get<bool>("deny_overwrite", false);
            if(m_config.get<bool>("deny_overwrite", global_overwrite)) {
                throw ModuleError("Overwriting of existing file " + file + " denied.");
            }
            LOG(WARNING) << "File " << file << " exists and will be overwritten.";
            try {
                corryvreckan::remove_file(file);
            } catch(std::invalid_argument& e) {
                throw ModuleError("Deleting file " + file + " failed: " + e.what());
            }
        }

        // Open the file to check if it can be accessed
        std::fstream file_stream(file, std::ios_base::out | std::ios_base::app);
        if(!file_stream.good()) {
            throw std::invalid_argument("file not accessible");
        }

        // Convert the file to an absolute path
        file = get_canonical_path(file);
    } catch(std::invalid_argument& e) {
        throw ModuleError("Path " + file + " cannot be created");
    }

    return file;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor or destructor
 * @warning Cannot be used from the constructor, because the instantiation logic has not finished yet
 * @warning This method should not be accessed from the destructor (the file is then already closed)
 * @note It is not needed to change directory to this file explicitly in the module, this is done automatically.
 */
TDirectory* Module::getROOTDirectory() const {
    // The directory will only be a null pointer if this method is executed from the constructor or destructor
    if(directory_ == nullptr) {
        throw InvalidModuleActionException("Cannot access ROOT directory in constructor or destructor");
    }

    return directory_;
}
void Module::set_ROOT_directory(TDirectory* directory) {
    directory_ = directory;
}

std::shared_ptr<Detector> Module::get_detector(std::string name) {
    auto it = find_if(
        m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) { return obj->getName() == name; });
    if(it == m_detectors.end()) {
        throw ModuleError("Device with detector ID " + name + " is not registered.");
    }

    return (*it);
}

std::shared_ptr<Detector> Module::get_reference() {
    return m_reference;
}

std::vector<std::shared_ptr<Detector>> Module::get_duts() {
    std::vector<std::shared_ptr<Detector>> duts;
    for_each(m_detectors.begin(), m_detectors.end(), [&duts](std::shared_ptr<Detector> obj) {
        if(obj->isDUT())
            duts.push_back(obj);
    });
    return duts;
}

bool Module::has_detector(std::string name) {
    auto it = find_if(
        m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) { return obj->getName() == name; });
    if(it == m_detectors.end()) {
        return false;
    }
    return true;
}

Configuration& Module::get_configuration() {
    return m_config;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor or destructor
 * @warning This function technically allows to write to the configurations of other modules, but this should never be done
 */
ConfigManager* Module::getConfigManager() {
    if(conf_manager_ == nullptr) {
        throw InvalidModuleActionException("Cannot access the config manager in constructor or destructor.");
    };
    return conf_manager_;
}
void Module::set_config_manager(ConfigManager* conf_manager) {
    conf_manager_ = conf_manager;
}
