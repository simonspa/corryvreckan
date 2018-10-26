/** @file
 *  @brief Implementation of the base module class
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Module.hpp"

using namespace corryvreckan;

Module::Module(Configuration config, std::shared_ptr<Detector> detector)
    : Module(config, std::vector<std::shared_ptr<Detector>>{detector}) {}

Module::~Module() {}

Module::Module(Configuration config, std::vector<std::shared_ptr<Detector>> detectors) {
    m_name = config.getName();
    m_config = config;
    m_detectors = detectors;
    IFLOG(TRACE) {
        std::stringstream det;
        for(auto& d : m_detectors) {
            det << d->name() << ", ";
        }
        LOG(TRACE) << "Module determined to run on detectors: " << det.str();
    }
}

std::shared_ptr<Detector> Module::get_detector(std::string name) {
    auto it = find_if(
        m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) { return obj->name() == name; });
    if(it == m_detectors.end()) {
        throw ModuleError("Device with detector ID " + name + " is not registered.");
    }

    return (*it);
}

std::shared_ptr<Detector> Module::get_reference() {
    return m_reference;
}

std::shared_ptr<Detector> Module::get_dut() {
    auto it = find_if(m_detectors.begin(), m_detectors.end(), [](std::shared_ptr<Detector> obj) { return obj->isDUT(); });
    if(it == m_detectors.end()) {
        return nullptr;
    }

    return (*it);
}

bool Module::has_detector(std::string name) {
    auto it = find_if(
        m_detectors.begin(), m_detectors.end(), [&name](std::shared_ptr<Detector> obj) { return obj->name() == name; });
    if(it == m_detectors.end()) {
        return false;
    }
    return true;
}
