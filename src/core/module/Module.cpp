/** @file
 *  @brief Implementation of the base module class
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Module.hpp"

using namespace corryvreckan;

Module::Module(Configuration config, std::vector<Detector*> detectors) {
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

Module::~Module() {}

Detector* Module::get_detector(std::string name) {
    auto it = find_if(m_detectors.begin(), m_detectors.end(), [&name](Detector* obj) { return obj->name() == name; });
    if(it == m_detectors.end()) {
        throw ModuleError("Device with detector ID " + name + " is not registered.");
    }

    return (*it);
}

Detector* Module::get_reference() {
    auto it = find_if(m_detectors.begin(), m_detectors.end(), [](Detector* obj) { return obj->isReference(); });
    return (*it);
}

bool Module::has_detector(std::string name) {
    auto it = find_if(m_detectors.begin(), m_detectors.end(), [&name](Detector* obj) { return obj->name() == name; });
    if(it == m_detectors.end()) {
        return false;
    }
    return true;
}
