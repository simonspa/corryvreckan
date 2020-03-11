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

#include "PlanarDetector.hpp"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace ROOT::Math;
using namespace corryvreckan;

PlanarDetector::PlanarDetector(const Configuration& config) : Detector(config) {

    this->initialise();

    LOG(TRACE) << "  Position:    " << Units::display(m_displacement, {"mm", "um"});
    LOG(TRACE) << "  Orientation: " << Units::display(m_orientation, {"deg"}) << " (" << m_orientation_mode << ")";
    if(m_timeOffset > 0.) {
        LOG(TRACE) << "Time offset: " << m_timeOffset;
    }

    if(m_timeResolution > 0) {
        LOG(TRACE) << "  Time resolution: " << Units::display(m_timeResolution, {"ms", "us"});
    }
    // Auxiliary devices don't have: number_of_pixels, pixel_pitch, spatial_resolution, mask_file, region-of-interest
    if(!isAuxiliary()) {
        buildNotAuxiliaryAxis(config);
    }
}

void PlanarDetector::buildNotAuxiliaryAxis(const Configuration& config) {
    // Auxiliary devices don't have: number_of_pixels, pixel_pitch, spatial_resolution, mask_file, region-of-interest
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
    if(config.has("mask_file")) {
        m_maskfile_name = config.get<std::string>("mask_file");
        std::string mask_file = config.getPath("mask_file");
        LOG(DEBUG) << "Adding mask to detector \"" << config.getName() << "\", reading from " << mask_file;
        setMaskFile(mask_file);
        processMaskFile();
    }
}

void PlanarDetector::processMaskFile() {
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

void PlanarDetector::maskChannel(int chX, int chY) {
    int channelID = chX + m_nPixels.X() * chY;
    m_masked[channelID] = true;
}

bool PlanarDetector::masked(int chX, int chY) const {
    int channelID = chX + m_nPixels.X() * chY;
    if(m_masked.count(channelID) > 0)
        return true;
    return false;
}

// Function to initialise transforms
void PlanarDetector::initialise() {

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

// Only if detector is not auxiliary
void PlanarDetector::configNotAuxiliary(Configuration& config) const {

    // Number of pixels
    config.set("number_of_pixels", m_nPixels);

    // Size of the pixels
    config.set("pixel_pitch", m_pitch, {"um"});

    // Intrinsic resolution:
    config.set("spatial_resolution", m_spatial_resolution, {"um"});

    // Pixel mask file:
    if(!m_maskfile_name.empty()) {
        config.set("mask_file", m_maskfile_name);
    }

    // Region-of-interest:
    config.setMatrix("roi", m_roi);
}

// Function to get global intercept with a track
PositionVector3D<Cartesian3D<double>> PlanarDetector::getIntercept(const Track* track) const {

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

PositionVector3D<Cartesian3D<double>> PlanarDetector::getLocalIntercept(const Track* track) const {
    return globalToLocal(getIntercept(track));
}

// Function to check if a track intercepts with a plane
bool PlanarDetector::hasIntercept(const Track* track, double pixelTolerance) const {

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
bool PlanarDetector::hitMasked(Track* track, int tolerance) const {

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
double PlanarDetector::getRow(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    // (1-m_nPixelsX%2)/2. --> add 1/2 pixel pitch if even number of rows
    double row = localPosition.Y() / m_pitch.Y() + static_cast<double>(m_nPixels.Y()) / 2. + (1 - m_nPixels.Y() % 2) / 2.;
    return row;
}
double PlanarDetector::getColumn(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    // (1-m_nPixelsX%2)/2. --> add 1/2 pixel pitch if even number of columns
    double column = localPosition.X() / m_pitch.X() + static_cast<double>(m_nPixels.X()) / 2. + (1 - m_nPixels.X() % 2) / 2.;
    return column;
}

// Function to get local position from row and column
PositionVector3D<Cartesian3D<double>> PlanarDetector::getLocalPosition(double column, double row) const {

    return PositionVector3D<Cartesian3D<double>>(
        m_pitch.X() * (column - m_nPixels.X() / 2), m_pitch.Y() * (row - m_nPixels.Y() / 2), 0.);
}

// Function to get in-pixel position
ROOT::Math::XYVector PlanarDetector::inPixel(const double column, const double row) const {
    // a pixel ranges from (col-0.5) to (col+0.5)
    return XYVector(m_pitch.X() * (column - floor(column) - 0.5), m_pitch.Y() * (row - floor(row) - 0.5));
}

ROOT::Math::XYVector PlanarDetector::inPixel(const PositionVector3D<Cartesian3D<double>> localPosition) const {
    double column = getColumn(localPosition);
    double row = getRow(localPosition);
    return inPixel(column, row);
}

// Check if track position is within ROI:
bool PlanarDetector::isWithinROI(const Track* track) const {

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
bool PlanarDetector::isWithinROI(Cluster* cluster) const {

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

/* Winding number test for a point in a polygon
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *      Input:   x, y = a point,
 *               polygon = vector of vertex points of a polygon V[n+1] with V[n]=V[0]
 *      Return:  wn = the winding number (=0 only when P is outside)
 */
int PlanarDetector::winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon) {
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
int PlanarDetector::isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2) {
    return ((pt1.first - pt0.first) * (pt2.second - pt0.second) - (pt2.first - pt0.first) * (pt1.second - pt0.second));
}
