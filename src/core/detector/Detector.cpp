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

Detector::Detector(const Configuration& config) {
    // Get information from the conditions file:
    m_displacement = config.get<ROOT::Math::XYZPoint>("position", ROOT::Math::XYZPoint());
    m_orientation = config.get<ROOT::Math::XYZVector>("orientation", ROOT::Math::XYZVector());
    m_orientation_mode = config.get<std::string>("orientation_mode", "xyz");

    // Number of pixels
    auto npixels = config.get<ROOT::Math::DisplacementVector2D<Cartesian2D<int>>>("number_of_pixels");
    // Size of the pixels
    m_pitch = config.get<ROOT::Math::XYVector>("pixel_pitch");
    m_detectorName = config.getName();

    if(Units::convert(m_pitch.X(), "mm") >= 1 or Units::convert(m_pitch.Y(), "mm") >= 1 or
       Units::convert(m_pitch.X(), "um") <= 1 or Units::convert(m_pitch.Y(), "um") <= 1) {
        LOG(WARNING) << "Pixel pitch unphysical for detector " << m_detectorName << ".";
        LOG(WARNING) << "Pitch X = " << Units::display(m_pitch.X(), {"nm", "um", "mm"})
                     << " ; Pitch Y = " << Units::display(m_pitch.Y(), {"nm", "um", "mm"});
    }

    m_detectorType = config.get<std::string>("type");
    m_nPixelsX = npixels.x();
    m_nPixelsY = npixels.y();
    m_timingOffset = config.get<double>("time_offset", 0.0);

    this->initialise();

    LOG(TRACE) << "Initialized \"" << m_detectorType << "\": " << m_nPixelsX << "x" << m_nPixelsY << " px, pitch of "
               << display_vector(m_pitch, {"mm", "um"});
    LOG(TRACE) << "  Position:    " << display_vector(m_displacement, {"mm", "um"});
    LOG(TRACE) << "  Orientation: " << display_vector(m_orientation, {"deg"}) << " (" << m_orientation_mode << ")";
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
        rotations = Rotation3D(ROOT::Math::RotationZ(m_orientation.Z()) * ROOT::Math::RotationY(m_orientation.Y()) *
                               ROOT::Math::RotationX(m_orientation.X()));
    } else if(m_orientation_mode == "zyx") {
        rotations = Rotation3D(ROOT::Math::RotationZYX(m_orientation.x(), m_orientation.y(), m_orientation.x()));
    } else {
        throw RuntimeError("Invalid detector orientation mode: " + m_orientation_mode);
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

    config.set("position", m_displacement);
    config.set("orientation_mode", m_orientation_mode);
    config.set("orientation", m_orientation);
    auto npixels = ROOT::Math::DisplacementVector2D<Cartesian2D<int>>(m_nPixelsX, m_nPixelsY);
    config.set("number_of_pixels", npixels);

    // Size of the pixels
    config.set("pixel_pitch", m_pitch);

    if(m_timingOffset != 0.) {
        config.set("time_offset", m_timingOffset);
    }

    if(!m_maskfile_name.empty()) {
        config.set("mask_file", m_maskfile_name);
    }

    return config;
}

// Function to get global intercept with a track
PositionVector3D<Cartesian3D<double>> Detector::getIntercept(Track* track) {

    // Get the distance from the plane to the track initial state
    double distance = (m_origin.X() - track->m_state.X()) * m_normal.X();
    distance += (m_origin.Y() - track->m_state.Y()) * m_normal.Y();
    distance += (m_origin.Z() - track->m_state.Z()) * m_normal.Z();
    distance /= (track->m_direction.X() * m_normal.X() + track->m_direction.Y() * m_normal.Y() +
                 track->m_direction.Z() * m_normal.Z());

    // Propagate the track
    PositionVector3D<Cartesian3D<double>> globalIntercept(track->m_state.X() + distance * track->m_direction.X(),
                                                          track->m_state.Y() + distance * track->m_direction.Y(),
                                                          track->m_state.Z() + distance * track->m_direction.Z());
    return globalIntercept;
}

PositionVector3D<Cartesian3D<double>> Detector::getLocalIntercept(Track* track) {
    return globalToLocal(getIntercept(track));
}

// Function to check if a track intercepts with a plane
bool Detector::hasIntercept(Track* track, double pixelTolerance) {

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
double Detector::getRow(PositionVector3D<Cartesian3D<double>> localPosition) {
    double row = ((localPosition.Y() + m_pitch.Y() / 2.) / m_pitch.Y()) + m_nPixelsY / 2.;
    return row;
}
double Detector::getColumn(PositionVector3D<Cartesian3D<double>> localPosition) {
    double column = ((localPosition.X() + m_pitch.X() / 2.) / m_pitch.X()) + m_nPixelsX / 2.;
    return column;
}

// Function to get local position from row and column
PositionVector3D<Cartesian3D<double>> Detector::getLocalPosition(double row, double column) {

    return PositionVector3D<Cartesian3D<double>>(
        m_pitch.X() * (column - m_nPixelsX / 2.), m_pitch.Y() * (row - m_nPixelsY / 2.), 0.);
}

// Function to get in-pixel position
double Detector::inPixelX(PositionVector3D<Cartesian3D<double>> localPosition) {
    double column = getColumn(localPosition);
    double inPixelX = m_pitch.X() * (column - floor(column));
    return inPixelX;
}
double Detector::inPixelY(PositionVector3D<Cartesian3D<double>> localPosition) {
    double row = getRow(localPosition);
    double inPixelY = m_pitch.Y() * (row - floor(row));
    return inPixelY;
}
