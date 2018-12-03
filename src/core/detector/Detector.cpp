/** @file
 *  @brief Implementation of the detector model
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
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

using namespace ROOT::Math;
using namespace corryvreckan;

Detector::Detector(const Configuration& config) : m_role(DetectorRole::NONE) {

    // Role of this detector:
    auto roles = config.getArray<std::string>("role", std::vector<std::string>{"none"});
    for(auto& role : roles) {
        std::transform(role.begin(), role.end(), role.begin(), ::tolower);
        if(role == "none") {
            m_role |= DetectorRole::NONE;
        } else if(role == "reference") {
            m_role |= DetectorRole::REFERENCE;
        } else if(role == "dut") {
            m_role |= DetectorRole::DUT;
        } else {
            throw InvalidValueError(config, "role", "Detector role does not exist.");
        }
    }

    // Detector position and orientation
    m_displacement = config.get<ROOT::Math::XYZPoint>("position", ROOT::Math::XYZPoint());
    m_orientation = config.get<ROOT::Math::XYZVector>("orientation", ROOT::Math::XYZVector());
    m_orientation_mode = config.get<std::string>("orientation_mode", "xyz");
    if(m_orientation_mode != "xyz" && m_orientation_mode != "zyx") {
        throw InvalidValueError(config, "orientation_mode", "Invalid detector orientation mode");
    }

    // Number of pixels
    auto npixels = config.get<ROOT::Math::DisplacementVector2D<Cartesian2D<int>>>("number_of_pixels");
    // Size of the pixels
    m_pitch = config.get<ROOT::Math::XYVector>("pixel_pitch");

    // Intrinsic position resolution, defaults to 4um:
    m_resolution = config.get<ROOT::Math::XYVector>("resolution", ROOT::Math::XYVector(0.004, 0.004));

    m_detectorName = config.getName();

    if(Units::convert(m_pitch.X(), "mm") >= 1 or Units::convert(m_pitch.Y(), "mm") >= 1 or
       Units::convert(m_pitch.X(), "um") <= 1 or Units::convert(m_pitch.Y(), "um") <= 1) {
        LOG(WARNING) << "Pixel pitch unphysical for detector " << m_detectorName << ": " << std::endl
                     << Units::display(m_pitch, {"nm", "um", "mm"});
    }

    m_detectorType = config.get<std::string>("type");
    m_nPixelsX = npixels.x();
    m_nPixelsY = npixels.y();
    m_timingOffset = config.get<double>("time_offset", 0.0);
    m_roi = config.getMatrix<int>("roi", std::vector<std::vector<int>>());

    this->initialise();

    LOG(TRACE) << "Initialized \"" << m_detectorType << "\": " << m_nPixelsX << "x" << m_nPixelsY << " px, pitch of "
               << Units::display(m_pitch, {"mm", "um"});
    LOG(TRACE) << "  Position:    " << Units::display(m_displacement, {"mm", "um"});
    LOG(TRACE) << "  Orientation: " << Units::display(m_orientation, {"deg"}) << " (" << m_orientation_mode << ")";
    if(m_timingOffset > 0.) {
        LOG(TRACE) << "Timing offset: " << m_timingOffset;
    }

    if(config.has("mask_file")) {
        m_maskfile_name = config.get<std::string>("mask_file");
        std::string mask_file = config.getPath("mask_file");
        LOG(DEBUG) << "Adding mask to detector \"" << config.getName() << "\", reading from " << mask_file;
        setMaskFile(mask_file);
        processMaskFile();
    }
}

bool Detector::isReference() {
    return static_cast<bool>(m_role & DetectorRole::REFERENCE);
}

bool Detector::isDUT() {
    return static_cast<bool>(m_role & DetectorRole::DUT);
}

// Functions to set and check channel masking
void Detector::setMaskFile(std::string file) {
    m_maskfile = file;
}

void Detector::processMaskFile() {
    // Open the file with masked pixels
    std::ifstream inputMaskFile(m_maskfile, std::ios::in);
    if(!inputMaskFile.is_open()) {
        LOG(ERROR) << "Could not open mask file " << m_maskfile;
    } else {
        int row = 0, col = 0;
        std::string id;
        std::string line;
        // loop over all lines and apply masks
        while(inputMaskFile >> id >> col >> row) {
            if(id == "c") {
                LOG(TRACE) << "Masking column " << col;
                int nRows = nPixelsY();
                for(int r = 0; r < nRows; r++) {
                    maskChannel(col, r);
                }
            } else if(id == "r") {
                LOG(TRACE) << "Masking row " << row;
                int nColumns = nPixelsX();
                for(int c = 0; c < nColumns; c++) {
                    maskChannel(c, row);
                }
            } else if(id == "p") {
                LOG(TRACE) << "Masking pixel " << col << " " << row;
                maskChannel(col, row); // Flag to mask a pixel
            } else {
                LOG(WARNING) << "Could not parse mask entry (id \"" << id << "\", col " << col << " row " << row << ")";
            }
        }
        LOG(INFO) << m_masked.size() << " masked pixels";
    }
}

void Detector::maskChannel(int chX, int chY) {
    int channelID = chX + m_nPixelsX * chY;
    m_masked[channelID] = true;
}

bool Detector::masked(int chX, int chY) {
    int channelID = chX + m_nPixelsX * chY;
    if(m_masked.count(channelID) > 0)
        return true;
    return false;
}

// Function to initialise transforms
void Detector::initialise() {

    // Make the local to global transform, built from a displacement and
    // rotation
    Translation3D translations = Translation3D(m_displacement.X(), m_displacement.Y(), m_displacement.Z());

    Rotation3D rotations;
    if(m_orientation_mode == "xyz") {
        rotations = RotationZ(m_orientation.Z()) * RotationY(m_orientation.Y()) * RotationX(m_orientation.X());
    } else if(m_orientation_mode == "zyx") {
        rotations = RotationZYX(m_orientation.x(), m_orientation.y(), m_orientation.x());
    }

    m_localToGlobal = Transform3D(rotations, translations);
    m_globalToLocal = m_localToGlobal.Inverse();

    // Find the normal to the detector surface. Build two points, the origin and a unit step in z,
    // transform these points to the global co-ordinate frame and then make a vector pointing between them
    m_origin = PositionVector3D<Cartesian3D<double>>(0., 0., 0.);
    m_origin = m_localToGlobal * m_origin;
    PositionVector3D<Cartesian3D<double>> localZ(0., 0., 1.);
    localZ = m_localToGlobal * localZ;
    m_normal = PositionVector3D<Cartesian3D<double>>(
        localZ.X() - m_origin.X(), localZ.Y() - m_origin.Y(), localZ.Z() - m_origin.Z());
}

// Function to update transforms (such as during alignment)
void Detector::update() {
    this->initialise();
}

Configuration Detector::getConfiguration() {

    Configuration config(name());
    config.set("type", m_detectorType);

    // Store the role of the detector
    std::vector<std::string> roles;
    if(this->isDUT()) {
        roles.push_back("dut");
    }
    if(this->isReference()) {
        roles.push_back("reference");
    }

    if(!roles.empty()) {
        config.setArray("role", roles);
    }

    config.set("position", m_displacement, {"um", "mm"});
    config.set("orientation_mode", m_orientation_mode);
    config.set("orientation", m_orientation, {"deg"});
    auto npixels = ROOT::Math::DisplacementVector2D<Cartesian2D<int>>(m_nPixelsX, m_nPixelsY);
    config.set("number_of_pixels", npixels);

    // Size of the pixels
    config.set("pixel_pitch", m_pitch, {"um"});

    // Intrinsic resolution:
    config.set("resolution", m_resolution, {"um"});

    if(m_timingOffset != 0.) {
        config.set("time_offset", m_timingOffset, {"ns", "us", "ms", "s"});
    }

    if(!m_maskfile_name.empty()) {
        config.set("mask_file", m_maskfile_name);
    }

    config.setMatrix("roi", m_roi);

    return config;
}

// Function to get global intercept with a track
PositionVector3D<Cartesian3D<double>> Detector::getIntercept(const Track* track) {

    // Get the distance from the plane to the track initial state
    double distance = (m_origin.X() - track->state().X()) * m_normal.X();
    distance += (m_origin.Y() - track->state().Y()) * m_normal.Y();
    distance += (m_origin.Z() - track->state().Z()) * m_normal.Z();
    distance /= (track->direction().X() * m_normal.X() + track->direction().Y() * m_normal.Y() +
                 track->direction().Z() * m_normal.Z());

    // Propagate the track
    PositionVector3D<Cartesian3D<double>> globalIntercept(track->state().X() + distance * track->direction().X(),
                                                          track->state().Y() + distance * track->direction().Y(),
                                                          track->state().Z() + distance * track->direction().Z());
    return globalIntercept;
}

PositionVector3D<Cartesian3D<double>> Detector::getLocalIntercept(const Track* track) {
    return globalToLocal(getIntercept(track));
}

// Function to check if a track intercepts with a plane
bool Detector::hasIntercept(const Track* track, double pixelTolerance) {

    // First, get the track intercept in global co-ordinates with the plane
    PositionVector3D<Cartesian3D<double>> globalIntercept = this->getIntercept(track);

    // Convert to local co-ordinates
    PositionVector3D<Cartesian3D<double>> localIntercept = this->m_globalToLocal * globalIntercept;

    // Get the row and column numbers
    double row = this->getRow(localIntercept);
    double column = this->getColumn(localIntercept);

    // Check if the row and column are outside of the chip
    bool intercept = true;
    if(row < (pixelTolerance - 0.5) || row > (this->m_nPixelsY - 0.5 - pixelTolerance) || column < (pixelTolerance - 0.5) ||
       column > (this->m_nPixelsX - 0.5 - pixelTolerance))
        intercept = false;

    return intercept;
}

// Function to check if a track goes through/near a masked pixel
bool Detector::hitMasked(Track* track, int tolerance) {

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
double Detector::getRow(const PositionVector3D<Cartesian3D<double>> localPosition) {
    // (1-m_nPixelsY%2)/2. --> add 1/2 pixel pitch if even number of rows
    double row = localPosition.Y() / m_pitch.Y() + static_cast<double>(m_nPixelsY) / 2. + (1 - m_nPixelsY % 2) / 2.;
    return row;
}
double Detector::getColumn(const PositionVector3D<Cartesian3D<double>> localPosition) {
    // (1-m_nPixelsX%2)/2. --> add 1/2 pixel pitch if even number of columns
    double column = localPosition.X() / m_pitch.X() + static_cast<double>(m_nPixelsX) / 2. + (1 - m_nPixelsX % 2) / 2.;
    return column;
}

// Function to get local position from row and column
PositionVector3D<Cartesian3D<double>> Detector::getLocalPosition(double row, double column) {

    return PositionVector3D<Cartesian3D<double>>(
        m_pitch.X() * (column - m_nPixelsX / 2.), m_pitch.Y() * (row - m_nPixelsY / 2.), 0.);
}

// Function to get in-pixel position
double Detector::inPixelX(const PositionVector3D<Cartesian3D<double>> localPosition) {
    double column = getColumn(localPosition);
    double inPixelX = m_pitch.X() * (column - floor(column));
    return inPixelX;
}
double Detector::inPixelY(const PositionVector3D<Cartesian3D<double>> localPosition) {
    double row = getRow(localPosition);
    double inPixelY = m_pitch.Y() * (row - floor(row));
    return inPixelY;
}

// Check if track position is within ROI:
bool Detector::isWithinROI(const Track* track) {

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
bool Detector::isWithinROI(Cluster* cluster) {

    // Empty region of interest:
    if(m_roi.empty()) {
        return true;
    }

    // Loop over all pixels of the cluster
    for(auto& pixel : (*cluster->pixels())) {
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

/* Winding number test for a point in a polygon
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *      Input:   x, y = a point,
 *               polygon = vector of vertex points of a polygon V[n+1] with V[n]=V[0]
 *      Return:  wn = the winding number (=0 only when P is outside)
 */
int Detector::winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon) {
    // Two points don't make an area
    if(polygon.size() < 3) {
        LOG(DEBUG) << "No ROI given.";
        return 0;
    }

    int wn = 0; // the  winding number counter

    // loop through all edges of the polygon

    // edge from V[i] to  V[i+1]
    for(size_t i = 0; i < polygon.size(); i++) {
        auto point_this = std::make_pair(polygon.at(i).at(0), polygon.at(i).at(1));
        auto point_next = (i + 1 < polygon.size() ? std::make_pair(polygon.at(i + 1).at(0), polygon.at(i + 1).at(1))
                                                  : std::make_pair(polygon.at(0).at(0), polygon.at(0).at(1)));

        // start y <= P.y
        if(point_this.second <= probe.second) {
            // an upward crossing
            if(point_next.second > probe.second) {
                // P left of  edge
                if(isLeft(point_this, point_next, probe) > 0) {
                    // have  a valid up intersect
                    ++wn;
                }
            }
        } else {
            // start y > P.y (no test needed)

            // a downward crossing
            if(point_next.second <= probe.second) {
                // P right of  edge
                if(isLeft(point_this, point_next, probe) < 0) {
                    // have  a valid down intersect
                    --wn;
                }
            }
        }
    }
    return wn;
}
