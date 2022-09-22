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

#include "PixelDetector.hpp"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace ROOT::Math;
using namespace corryvreckan;

PixelDetector::PixelDetector(const Configuration& config) : Detector(config) {

    // Set detector position and direction from configuration file
    SetPostionAndOrientation(config);

    // initialize transform
    this->initialise();

    // Auxiliary devices don't have: number_of_pixels, pixel_pitch, spatial_resolution, mask_file, region-of-interest
    if(!isAuxiliary()) {
        build_axes(config);
    }
}

void PixelDetector::build_axes(const Configuration& config) {

    m_nPixels = config.get<ROOT::Math::DisplacementVector2D<Cartesian2D<int>>>("number_of_pixels");
    // Size of the pixels:
    m_pitch = config.get<ROOT::Math::XYVector>("pixel_pitch");

    LOG(TRACE) << "Initialized \"" << m_detectorType << "\": " << m_nPixels.X() << "x" << m_nPixels.Y() << " px, pitch of "
               << Units::display(m_pitch, {"mm", "um"});

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

    if(config.has("mask_file")) {
        auto mask_file = config.getPath("mask_file", true);
        LOG(DEBUG) << "Adding mask to detector \"" << config.getName() << "\", reading from " << mask_file;
        maskFile(mask_file);
        process_mask_file();
    }
}

void PixelDetector::SetPostionAndOrientation(const Configuration& config) {
    // Detector position and orientation
    m_displacement = config.get<ROOT::Math::XYZPoint>("position", ROOT::Math::XYZPoint());
    m_orientation = config.get<ROOT::Math::XYZVector>("orientation", ROOT::Math::XYZVector());
    m_orientation_mode = config.get<std::string>("orientation_mode", "xyz");

    if(m_orientation_mode != "xyz" && m_orientation_mode != "zyx" && m_orientation_mode != "zxz") {
        throw InvalidValueError(config, "orientation_mode", "orientation_mode should be either 'zyx', xyz' or 'zxz'");
    }

    LOG(TRACE) << "  Position:    " << Units::display(m_displacement, {"mm", "um"});
    LOG(TRACE) << "  Orientation: " << Units::display(m_orientation, {"deg"}) << " (" << m_orientation_mode << ")";
}

void PixelDetector::process_mask_file() {
    // Open the file with masked pixels
    std::ifstream inputMaskFile(m_maskfile, std::ios::in);
    if(!inputMaskFile.is_open()) {
        LOG(WARNING) << "Could not open mask file " << m_maskfile;
    } else {
        int row = 0, col = 0;
        std::string id;
        // loop over all lines and apply masks
        while(inputMaskFile >> id) {
            if(id == "c") {
                inputMaskFile >> col;
                if(col > nPixels().X() - 1) {
                    LOG(WARNING) << "Column " << col << " outside of pixel matrix, chip has only " << nPixels().X()
                                 << " columns!";
                }
                LOG(TRACE) << "Masking column " << col;
                for(int r = 0; r < nPixels().Y(); r++) {
                    maskChannel(col, r);
                }
            } else if(id == "r") {
                inputMaskFile >> row;
                if(row > nPixels().Y() - 1) {
                    LOG(WARNING) << "Row " << col << " outside of pixel matrix, chip has only " << nPixels().Y() << " rows!";
                }
                LOG(TRACE) << "Masking row " << row;
                for(int c = 0; c < nPixels().X(); c++) {
                    maskChannel(c, row);
                }
            } else if(id == "p") {
                inputMaskFile >> col >> row;
                if(col > nPixels().X() - 1 || row > nPixels().Y() - 1) {
                    LOG(WARNING) << "Pixel " << col << " " << row << " outside of pixel matrix, chip has only "
                                 << nPixels().X() << " x " << nPixels().Y() << " pixels!";
                }
                LOG(TRACE) << "Masking pixel " << col << " " << row;
                maskChannel(col, row); // Flag to mask a pixel
            } else {
                LOG(WARNING) << "Could not parse mask entry (id \"" << id << "\")";
            }
        }
        LOG(INFO) << m_masked.size() << " masked pixels";
    }
}

bool PixelDetector::isWithinMatrix(const int col, const int row) const {
    return !(col > nPixels().X() - 1 || row > nPixels().Y() - 1 || col < 0 || row < 0);
}

void PixelDetector::maskChannel(int chX, int chY) {
    int channelID = chX + m_nPixels.X() * chY;
    m_masked[channelID] = true;
}

bool PixelDetector::masked(int chX, int chY) const {
    int channelID = chX + m_nPixels.X() * chY;
    if(m_masked.count(channelID) > 0)
        return true;
    return false;
}

// Function to initialise transforms
void PixelDetector::initialise() {

    // Make the local to global transform, built from a displacement and
    // rotation
    Translation3D translations = Translation3D(m_displacement.X(), m_displacement.Y(), m_displacement.Z());

    Rotation3D rotations;
    if(m_orientation_mode == "xyz") {
        LOG(DEBUG) << "Interpreting Euler angles as XYZ rotation";
        // First angle given in the configuration file is around x, second around y, last around z:
        rotations = RotationZ(m_orientation.Z()) * RotationY(m_orientation.Y()) * RotationX(m_orientation.X());
    } else if(m_orientation_mode == "zyx") {
        LOG(DEBUG) << "Interpreting Euler angles as ZYX rotation";
        // First angle given in the configuration file is around z, second around y, last around x:
        rotations = RotationZYX(m_orientation.x(), m_orientation.y(), m_orientation.z());
    } else if(m_orientation_mode == "zxz") {
        LOG(DEBUG) << "Interpreting Euler angles as ZXZ rotation";
        // First angle given in the configuration file is around z, second around x, last around z:
        rotations = EulerAngles(m_orientation.x(), m_orientation.y(), m_orientation.z());
    } else {
        throw InvalidSettingError(this, "orientation_mode", "orientation_mode should be either 'zyx', xyz' or 'zxz'");
    }

    m_localToGlobal = Transform3D(rotations, translations);
    m_globalToLocal = m_localToGlobal.Inverse();

    // Find the normal to the detector surface. Build two points, the origin and a unit step in z,
    // transform these points to the global coordinate frame and then make a vector pointing between them
    m_origin = PositionVector3D<Cartesian3D<double>>(0., 0., 0.);
    m_origin = m_localToGlobal * m_origin;
    PositionVector3D<Cartesian3D<double>> localZ(0., 0., 1.);
    localZ = m_localToGlobal * localZ;
    m_normal = PositionVector3D<Cartesian3D<double>>(
        localZ.X() - m_origin.X(), localZ.Y() - m_origin.Y(), localZ.Z() - m_origin.Z());
}

// Only if detector is not auxiliary
void PixelDetector::configure_detector(Configuration& config) const {

    // Number of pixels
    config.set("number_of_pixels", m_nPixels);

    // Size of the pixels
    config.set("pixel_pitch", m_pitch, {{"um"}});

    // Intrinsic resolution:
    config.set("spatial_resolution", m_spatial_resolution, {{"um"}});

    // Pixel mask file:
    if(!m_maskfile.empty()) {
        config.set("mask_file", m_maskfile.string());
    }

    // Region-of-interest:
    config.setMatrix("roi", m_roi);
}

void PixelDetector::configure_pos_and_orientation(Configuration& config) const {
    config.set("position", m_displacement, {"um", "mm"});
    config.set("orientation_mode", m_orientation_mode);
    config.set("orientation", m_orientation, {{"deg"}});
}

// Function to get global intercept with a track
PositionVector3D<Cartesian3D<double>> PixelDetector::getIntercept(const Track* track) const {

    // FIXME: this is else statement can only be temporary
    if(track->getType() == "GblTrack") {
        return track->getState(getName());
    } else {
        // Get the distance from the plane to the track initial state
        double distance = (m_origin.X() - track->getState(m_detectorName).X()) * m_normal.X();
        distance += (m_origin.Y() - track->getState(m_detectorName).Y()) * m_normal.Y();
        distance += (m_origin.Z() - track->getState(m_detectorName).Z()) * m_normal.Z();
        distance /= (track->getDirection(m_detectorName).X() * m_normal.X() +
                     track->getDirection(m_detectorName).Y() * m_normal.Y() +
                     track->getDirection(m_detectorName).Z() * m_normal.Z());

        // Propagate the track
        PositionVector3D<Cartesian3D<double>> globalIntercept(
            track->getState(m_detectorName).X() + distance * track->getDirection(m_detectorName).X(),
            track->getState(m_detectorName).Y() + distance * track->getDirection(m_detectorName).Y(),
            track->getState(m_detectorName).Z() + distance * track->getDirection(m_detectorName).Z());
        return globalIntercept;
    }
}

PositionVector3D<Cartesian3D<double>> PixelDetector::getLocalIntercept(const Track* track) const {
    return globalToLocal(getIntercept(track));
}

// Function to check if a track intercepts with a plane
bool PixelDetector::hasIntercept(const Track* track, double pixelTolerance) const {

    // First, get the track intercept in global coordinates with the plane
    PositionVector3D<Cartesian3D<double>> globalIntercept = this->getIntercept(track);

    // Convert to local coordinates
    PositionVector3D<Cartesian3D<double>> localIntercept = this->m_globalToLocal * globalIntercept;

    // Get the row and column numbers
    double row = this->getRow(localIntercept);
    double column = this->getColumn(localIntercept);

    // Check if the row and column are outside of the chip
    // Chip reaches from -0.5 to nPixels-0.5
    bool intercept = true;
    if(row < pixelTolerance - 0.5 || row > (this->m_nPixels.Y() - pixelTolerance - 0.5) || column < pixelTolerance - 0.5 ||
       column > (this->m_nPixels.X() - pixelTolerance - 0.5)) {
        intercept = false;
    }

    return intercept;
}

// Function to check if a track goes through/near a masked pixel
bool PixelDetector::hitMasked(const Track* track, int tolerance) const {

    // First, get the track intercept in global coordinates with the plane
    PositionVector3D<Cartesian3D<double>> globalIntercept = this->getIntercept(track);

    // Convert to local coordinates
    PositionVector3D<Cartesian3D<double>> localIntercept = this->m_globalToLocal * globalIntercept;

    // Get the row and column numbers
    int row = static_cast<int>(floor(this->getRow(localIntercept) + 0.5));
    int column = static_cast<int>(floor(this->getColumn(localIntercept) + 0.5));

    // Check if the pixels around this pixel are masked
    bool hitmasked = false;
    for(int r = (row - tolerance); r <= (row + tolerance); r++) {
        for(int c = (column - tolerance); c <= (column + tolerance); c++) {
            if(this->masked(c, r)) {
                hitmasked = true;
            }
        }
    }

    return hitmasked;
}

// Functions to get row and column from local position
double PixelDetector::getRow(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    return localPosition.Y() / m_pitch.Y() + static_cast<double>(m_nPixels.Y() - 1) / 2.;
}
double PixelDetector::getColumn(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    return localPosition.X() / m_pitch.X() + static_cast<double>(m_nPixels.X() - 1) / 2.;
}

// Function to get local position from row and column
PositionVector3D<Cartesian3D<double>> PixelDetector::getLocalPosition(double column, double row) const {

    return PositionVector3D<Cartesian3D<double>>(m_pitch.X() * (column - static_cast<double>(m_nPixels.X() - 1) / 2.),
                                                 m_pitch.Y() * (row - static_cast<double>(m_nPixels.Y() - 1) / 2.),
                                                 0.);
}

// Function to get row and column of pixel
std::pair<int, int> PixelDetector::getInterceptPixel(PositionVector3D<Cartesian3D<double>> localPosition) const {
    return {floor(getColumn(localPosition) + 0.5), floor(getRow(localPosition) + 0.5)};
}

// Function to get in-pixel position
ROOT::Math::XYVector PixelDetector::inPixel(const double column, const double row) const {
    // a pixel ranges from (col-0.5) to (col+0.5)
    return XYVector(m_pitch.X() * (column - floor(column + 0.5)), m_pitch.Y() * (row - floor(row + 0.5)));
}

ROOT::Math::XYVector PixelDetector::inPixel(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    double column = getColumn(localPosition);
    double row = getRow(localPosition);
    return inPixel(column, row);
}

// Check if track position is within ROI:
bool PixelDetector::isWithinROI(const Track* track) const {

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
bool PixelDetector::isWithinROI(Cluster* cluster) const {

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

XYVector PixelDetector::getSize() const {
    return XYVector(m_pitch.X() * m_nPixels.X(), m_pitch.Y() * m_nPixels.Y());
}

/* Winding number test for a point in a polygon
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *      Input:   x, y = a point,
 *               polygon = vector of vertex points of a polygon V[n+1] with V[n]=V[0]
 *      Return:  wn = the winding number (=0 only when P is outside)
 */
int PixelDetector::winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon) {
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
/* isLeft(): tests if a point is Left|On|Right of an infinite line.
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *    Input:  three points P0, P1, and P2
 *    Return: >0 for P2 left of the line through P0 and P1
 *            =0 for P2  on the line
 *            <0 for P2  right of the line
 *    See: Algorithm 1 "Area of Triangles and Polygons"
 */
int PixelDetector::isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2) {
    return ((pt1.first - pt0.first) * (pt2.second - pt0.second) - (pt2.first - pt0.first) * (pt1.second - pt0.second));
}

// Check if a pixel touches any of the pixels in a cluster
bool PixelDetector::isNeighbor(const std::shared_ptr<Pixel>& neighbor,
                               const std::shared_ptr<Cluster>& cluster,
                               const int neighbor_radius_row,
                               const int neighbor_radius_col) const {
    for(const auto* pixel : cluster->pixels()) {
        int row_distance = abs(pixel->row() - neighbor->row());
        int col_distance = abs(pixel->column() - neighbor->column());

        if(row_distance <= neighbor_radius_row && col_distance <= neighbor_radius_col) {
            if(row_distance > 1 || col_distance > 1) {
                cluster->setSplit(true);
            }
            return true;
        }
    }
    return false;
}

std::set<std::pair<int, int>>
PixelDetector::getNeighbors(const std::shared_ptr<Pixel>& px, const size_t distance, const bool include_corners) const {
    std::set<std::pair<int, int>> neighbors;

    for(int x = px->column() - static_cast<int>(distance); x <= px->column() + static_cast<int>(distance); x++) {
        for(int y = px->row() - static_cast<int>(distance); y <= px->row() + static_cast<int>(distance); y++) {
            // Check if we have one common coordinate if corners should be excluded:
            if(!include_corners && x != px->column() && y != px->row()) {
                continue;
            }
            if(!isWithinMatrix(x, y)) {
                continue;
            }
            neighbors.insert({x, y});
        }
    }

    return neighbors;
}
