/** @file
 *  @brief Implementation of the detector model
 *  @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <fstream>
#include <map>
#include <string>

#include "Math/RotationX.h"
#include "Math/RotationY.h"
#include "Math/RotationZ.h"
#include "Math/RotationZYX.h"

#include "Detector.hpp"
#include "HexagonalPixelDetector.hpp"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace ROOT::Math;
using namespace corryvreckan;

Detector::Detector(const Configuration& config) : m_role(DetectorRole::NONE) {

    // Role of this detector:
    auto roles = config.getArray<DetectorRole>("role", {DetectorRole::NONE});
    for(auto& role : roles) {
        LOG(DEBUG) << "Adding role " << corryvreckan::to_string(role);
        m_role |= role;
    }

    // Auxiliary devices cannot hold other roles:
    if(hasRole(DetectorRole::AUXILIARY) && m_role != DetectorRole::AUXILIARY) {
        throw InvalidValueError(config, "role", "Auxiliary devices cannot hold any other detector role");
    }

    if(hasRole(DetectorRole::PASSIVE) && m_role != DetectorRole::PASSIVE) {
        throw InvalidValueError(config, "role", "Passive detector cannot hold any other role");
    }

    m_detectorName = config.getName();

    // Material budget of detector, including support material
    if(!config.has("material_budget")) {
        m_materialBudget = 0.0;
        LOG(WARNING) << "No material budget given for " << m_detectorName << ", assuming " << m_materialBudget;
    } else if(config.get<double>("material_budget") < 0) {
        throw InvalidValueError(config, "material_budget", "Material budget has to be positive");
    } else {
        m_materialBudget = config.get<double>("material_budget");
    }

    m_detectorType = config.get<std::string>("type");
    std::transform(m_detectorType.begin(), m_detectorType.end(), m_detectorType.begin(), ::tolower);
    m_detectorCoordinates = config.get<std::string>("coordinates", "cartesian");
    std::transform(m_detectorCoordinates.begin(), m_detectorCoordinates.end(), m_detectorCoordinates.begin(), ::tolower);
    m_timeOffset = config.get<double>("time_offset", 0.0);
    if(m_timeOffset > 0.) {
        LOG(TRACE) << "Time offset: " << m_timeOffset;
    }

    // Time resolution - default to negative number, i.e. unknown. This will trigger an exception
    // when calling getTimeResolution
    m_timeResolution = config.get<double>("time_resolution", -1.0);
    if(m_timeResolution > 0) {
        LOG(TRACE) << "  Time resolution: " << Units::display(m_timeResolution, {"ms", "us"});
    }

    if(config.has("calibration_file")) {
        m_calibrationfile = config.getPath("calibration_file", true);
        LOG(DEBUG) << "Found calibration file for detector " << getName() << " at " << m_calibrationfile.value_or("");
    }
}

std::shared_ptr<Detector> corryvreckan::Detector::factory(const Configuration& config) {
    // default coordinate is cartesian coordinate
    auto coordinates = config.get<std::string>("coordinates", "cartesian");
    std::transform(coordinates.begin(), coordinates.end(), coordinates.begin(), ::tolower);
    if(coordinates == "cartesian") {
        return std::make_shared<PixelDetector>(config);
    } else if(coordinates == "hexagonal") {
        return std::make_shared<HexagonalPixelDetector>(config);
    } else {
        throw InvalidValueError(config, "coordinates", "Coordinates can only set to be cartesian now");
    }
}

double Detector::getTimeResolution() const {
    if(m_timeResolution > 0) {
        return m_timeResolution;
    } else {
        throw InvalidSettingError(this, "time_resolution", "Time resolution not set but requested");
    }
}

std::string Detector::getName() const {
    return m_detectorName;
}

std::string Detector::getType() const {
    return m_detectorType;
}

bool Detector::isReference() const {
    return static_cast<bool>(m_role & DetectorRole::REFERENCE);
}

bool Detector::isDUT() const {
    return static_cast<bool>(m_role & DetectorRole::DUT);
}

bool Detector::isAuxiliary() const {
    return static_cast<bool>(m_role & DetectorRole::AUXILIARY);
}

bool Detector::isPassive() const {
    return static_cast<bool>(m_role & DetectorRole::PASSIVE);
}

DetectorRole Detector::getRoles() const {
    return m_role;
}

bool Detector::hasRole(DetectorRole role) const {
    return static_cast<bool>(m_role & role);
}

// Function to set the channel maskfile
void Detector::maskFile(std::filesystem::path file) {
    m_maskfile = std::move(file);
}

// Function to update transforms (such as during alignment)
void Detector::update() {
    this->initialise();
}

Configuration Detector::getConfiguration() const {

    Configuration config(getName());
    config.set("type", m_detectorType);

    if(m_detectorCoordinates != "cartesian") {
        config.set("coordinates", m_detectorCoordinates);
    }

    // Store the role of the detector
    std::vector<std::string> roles;
    if(this->isDUT()) {
        roles.emplace_back("dut");
    }
    if(this->isReference()) {
        roles.emplace_back("reference");
    }
    if(this->isAuxiliary()) {
        roles.emplace_back("auxiliary");
    }
    if(this->isPassive()) {
        roles.push_back("passive");
    }

    if(!roles.empty()) {
        config.setArray("role", roles);
    }

    if(m_timeOffset != 0.) {
        config.set("time_offset", m_timeOffset, {"ns", "us", "ms", "s"});
    }

    config.set("time_resolution", m_timeResolution, {"ns", "us", "ms", "s"});

    // different for PixelDetector and StripDetector
    this->configure_pos_and_orientation(config);

    // material budget
    if(m_materialBudget > std::numeric_limits<double>::epsilon()) {
        config.set("material_budget", m_materialBudget);
    }

    // only if detector is not auxiliary:
    if(!this->isAuxiliary()) {
        this->configure_detector(config);
    }

    // add detector calibration file path, if set in the main configuration
    if(this->calibrationFile() != "") {
        config.set("calibration_file", this->calibrationFile());
    }

    return config;
}
