/** @file
 *  @brief Implementation of the detector model
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
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
#include "core/utils/log.h"
#include "exceptions.h"

using namespace ROOT::Math;
using namespace corryvreckan;

Detector::Detector(const Configuration& config) : m_role(DetectorRole::NONE) {

    // Role of this detector:
    auto roles = config.getArray<std::string>("role", std::vector<std::string>{"none"});
    for(auto& role : roles) {
        std::transform(role.begin(), role.end(), role.begin(), ::tolower);
        if(role == "none") {
            m_role |= DetectorRole::NONE;
        } else if(role == "reference" || role == "ref") {
            m_role |= DetectorRole::REFERENCE;
        } else if(role == "dut") {
            m_role |= DetectorRole::DUT;
        } else if(role == "auxiliary" || role == "aux") {
            m_role |= DetectorRole::AUXILIARY;
        } else {
            throw InvalidValueError(config, "role", "Detector role does not exist.");
        }
    }

    // Auxiliary devices cannot hold other roles:
    if(static_cast<bool>(m_role & DetectorRole::AUXILIARY) && m_role != DetectorRole::AUXILIARY) {
        throw InvalidValueError(config, "role", "Auxiliary devices cannot hold any other detector role");
    }

    // Detector position and orientation
    m_displacement = config.get<ROOT::Math::XYZPoint>("position", ROOT::Math::XYZPoint());
    m_orientation = config.get<ROOT::Math::XYZVector>("orientation", ROOT::Math::XYZVector());
    m_orientation_mode = config.get<std::string>("orientation_mode", "xyz");
    if(m_orientation_mode != "xyz" && m_orientation_mode != "zyx") {
        throw InvalidValueError(config, "orientation_mode", "Invalid detector orientation mode");
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
    m_timeOffset = config.get<double>("time_offset", 0.0);

    // Time resolution - default ot negative number, i.e. unknown. This will trigger an exception
    // when calling getTimeResolution
    m_timeResolution = config.get<double>("time_resolution", -1.0);

    ///// Initialize the detector, calculate transformations etc
    /// this->initialise();
    /////buildNotAuxiliaryAxis(config);

    if(config.has("calibration_file")) {
        m_calibrationfile = config.getPath("calibration_file");
    }

    /*
    if(!isAuxiliary()) {
        // Number of pixels:
        m_nPixels = config.get<ROOT::Math::DisplacementVector2D<Cartesian2D<int>>>("number_of_pixels");
        // Size of the pixels:
        m_pitch = config.get<ROOT::Math::XYVector>("pixel_pitch");

        if(Units::convert(m_pitch.X(), "mm") >= 1 or Units::convert(m_pitch.Y(), "mm") >= 1 or
           Units::convert(m_pitch.X(), "um") <= 1 or Units::convert(m_pitch.Y(), "um") <= 1) {
            LOG(WARNING) << "Pixel pitch unphysical for detector " << m_detectorName << ": " << std::endl
                         << Units::display(m_pitch, {"nm", "um", "mm"});
        }

        // Intrinsic spatial resolution, defaults to pitch/sqrt(12):
        m_spatial_resolution = config.get<ROOT::Math::XYVector>("spatial_resolution", m_pitch / std::sqrt(12));
        if(!config.has("spatial_resolution")) {
            LOG(WARNING) << "Spatial resolution for detector '" << m_detectorName << "' not set." << std::endl
                         << "Using pitch/sqrt(12) as default";
        }

        // region of interest:
        m_roi = config.getMatrix<int>("roi", std::vector<std::vector<int>>());
        // set the mask
        if(config.has("mask_file")) {
            m_maskfile_name = config.get<std::string>("mask_file");
            std::string mask_file = config.getPath("mask_file");
            LOG(DEBUG) << "Adding mask to detector \"" << config.getName() << "\", reading from " << mask_file;
            setMaskFile(mask_file);
            processMaskFile();
        }
    }
    */
}

double Detector::getTimeResolution() const {
    if(m_timeResolution > 0) {
        return m_timeResolution;
    } else {
        throw InvalidSettingError(this, "time_resolution", "Time resolution not set but requested");
    }
}

std::string Detector::Name() const {
    return m_detectorName;
}

std::string Detector::Type() const {
    return m_detectorType;
}

XYVector Detector::size() const {
    return XYVector(m_pitch.X() * m_nPixels.X(), m_pitch.Y() * m_nPixels.Y());
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

// Functions to set and check channel masking
void Detector::setMaskFile(std::string file) {
    m_maskfile = file;
}

// Function to update transforms (such as during alignment)
void Detector::update() {
    this->initialise();
}

Configuration Detector::getConfiguration() const {

    Configuration config(Name());
    config.set("type", m_detectorType);

    // Store the role of the detector
    std::vector<std::string> roles;
    if(this->isDUT()) {
        roles.push_back("dut");
    }
    if(this->isReference()) {
        roles.push_back("reference");
    }
    if(this->isAuxiliary()) {
        roles.push_back("auxiliary");
    }

    if(!roles.empty()) {
        config.setArray("role", roles);
    }

    config.set("position", m_displacement, {"um", "mm"});
    config.set("orientation_mode", m_orientation_mode);
    config.set("orientation", m_orientation, {"deg"});

    if(m_timeOffset != 0.) {
        config.set("time_offset", m_timeOffset, {"ns", "us", "ms", "s"});
    }

    config.set("time_resolution", m_timeResolution, {"ns", "us", "ms", "s"});

    // material budget
    if(m_materialBudget > std::numeric_limits<double>::epsilon()) {
        config.set("material_budget", m_materialBudget);
    }
    // only if detector is not auxiliary:
    if(!this->isAuxiliary()) {
        this->configureDetector(config);
    }

    return config;
}

// Function to get global intercept with a track
PositionVector3D<Cartesian3D<double>> Detector::getIntercept(const Track* track) const {

    // FIXME: this is else statement can only be temporary
    if(track->getType() == "gbl") {
        return track->state(name());
    } else {
        // Get the distance from the plane to the track initial state
        double distance = (m_origin.X() - track->state(m_detectorName).X()) * m_normal.X();
        distance += (m_origin.Y() - track->state(m_detectorName).Y()) * m_normal.Y();
        distance += (m_origin.Z() - track->state(m_detectorName).Z()) * m_normal.Z();
        distance /=
            (track->direction(m_detectorName).X() * m_normal.X() + track->direction(m_detectorName).Y() * m_normal.Y() +
             track->direction(m_detectorName).Z() * m_normal.Z());

        // Propagate the track
        PositionVector3D<Cartesian3D<double>> globalIntercept(
            track->state(m_detectorName).X() + distance * track->direction(m_detectorName).X(),
            track->state(m_detectorName).Y() + distance * track->direction(m_detectorName).Y(),
            track->state(m_detectorName).Z() + distance * track->direction(m_detectorName).Z());
        return globalIntercept;
    }
}

PositionVector3D<Cartesian3D<double>> Detector::getLocalIntercept(const Track* track) const {
    return globalToLocal(getIntercept(track));
}

// Function to check if a track intercepts with a plane
bool Detector::hasIntercept(const Track* track, double pixelTolerance) const {

    // First, get the track intercept in global co-ordinates with the plane
    PositionVector3D<Cartesian3D<double>> globalIntercept = this->getIntercept(track);

    // Convert to local co-ordinates
    PositionVector3D<Cartesian3D<double>> localIntercept = this->m_globalToLocal * globalIntercept;

    // Get the row and column numbers
    double row = this->getRow(localIntercept);
    double column = this->getColumn(localIntercept);

    // Check if the row and column are outside of the chip
    // Chip reaches from -0.5 to nPixels-0.5
    bool intercept = true;
    if(row < pixelTolerance - 0.5 || row > (this->m_nPixels.Y() - pixelTolerance - 0.5) || column < pixelTolerance - 0.5 ||
       column > (this->m_nPixels.X() - pixelTolerance - 0.5))
        intercept = false;

    return intercept;
}

// Function to check if a track goes through/near a masked pixel
bool Detector::hitMasked(Track* track, int tolerance) const {

    // First, get the track intercept in global co-ordinates with the plane
    PositionVector3D<Cartesian3D<double>> globalIntercept = this->getIntercept(track);

    // Convert to local co-ordinates
    PositionVector3D<Cartesian3D<double>> localIntercept = this->m_globalToLocal * globalIntercept;

    // Get the row and column numbers
    int row = static_cast<int>(floor(this->getRow(localIntercept) + 0.5));
    int column = static_cast<int>(floor(this->getColumn(localIntercept) + 0.5));

    // Check if the pixels around this pixel are masked
    bool hitmasked = false;
    for(int r = (row - tolerance); r <= (row + tolerance); r++) {
        for(int c = (column - tolerance); c <= (column + tolerance); c++) {
            if(this->masked(c, r))
                hitmasked = true;
        }
    }

    return hitmasked;
}

// Functions to get row and column from local position
double Detector::getRow(const PositionVector3D<Cartesian3D<double>> localPosition) const {

    double row = localPosition.Y() / m_pitch.Y() + static_cast<double>(m_nPixels.Y() - 1) / 2.;
    return row;
}

double Detector::getColumn(const PositionVector3D<Cartesian3D<double>> localPosition) const {

    double column = localPosition.X() / m_pitch.X() + static_cast<double>(m_nPixels.X() - 1) / 2.;
    return column;
}

// Function to get local position from row and column
PositionVector3D<Cartesian3D<double>> Detector::getLocalPosition(double column, double row) const {

    return PositionVector3D<Cartesian3D<double>>(
        m_pitch.X() * (column - (m_nPixels.X() - 1) / 2.), m_pitch.Y() * (row - (m_nPixels.Y() - 1) / 2.), 0.);
}

// Function to get in-pixel position
ROOT::Math::XYVector Detector::inPixel(const double column, const double row) const {
    // a pixel ranges from (col-0.5) to (col+0.5)
    return XYVector(m_pitch.X() * (column - floor(column) - 0.5), m_pitch.Y() * (row - floor(row) - 0.5));
}

ROOT::Math::XYVector Detector::inPixel(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    double column = getColumn(localPosition);
    double row = getRow(localPosition);
    return inPixel(column, row);
}

// Check if track position is within ROI:
bool Detector::isWithinROI(const Track* track) const {

    // Empty region of interest:
    if(m_roi.empty()) {
        return true;
    }

    // Check that track is within region of interest using winding number algorithm
    auto localIntercept = this->getLocalIntercept(track);
    auto coordinates = std::make_pair(this->getColumn(localIntercept), this->getRow(localIntercept));
    if(winding_number(coordinates, m_roi) != 0) {
        return true;
    }

    // Outside ROI:
    return false;
}

// Check if cluster is within ROI and/or touches ROI border:
bool Detector::isWithinROI(Cluster* cluster) const {

    // Empty region of interest:
    if(m_roi.empty()) {
        return true;
    }

    // Loop over all pixels of the cluster
    for(auto& pixel : cluster->pixels()) {
        if(winding_number(pixel->coordinates(), m_roi) == 0) {
            return false;
        }
    }
    return true;
}

/* isLeft(): tests if a point is Left|On|Right of an infinite line.
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *    Input:  three points P0, P1, and P2
 *    Return: >0 for P2 left of the line through P0 and P1
 *            =0 for P2  on the line
 *            <0 for P2  right of the line
 *    See: Algorithm 1 "Area of Triangles and Polygons"
 */
int Detector::isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2) {
    return ((pt1.first - pt0.first) * (pt2.second - pt0.second) - (pt2.first - pt0.first) * (pt1.second - pt0.second));
}
