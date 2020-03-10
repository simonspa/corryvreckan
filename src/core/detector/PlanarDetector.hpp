/** @file
 *  @brief Detector model class
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_PLANARDETECTOR_H
#define CORRYVRECKAN_PLANARDETECTOR_H

#include <fstream>
#include <map>
#include <string>

#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include "Math/Transform3D.h"
#include "Math/Vector3D.h"

#include "core/config/Configuration.hpp"
#include "core/utils/ROOT.h"
#include "core/utils/log.h"
#include "objects/Track.hpp"
#include "Detector.hpp"

namespace corryvreckan {

    /**
     * @brief Detector representation in the reconstruction chain
     *
     * Contains the detector with all its properties such as type, name, position and orientation, pitch, spatial resolution
     * etc.
     */
    class PlanarDetector : public Detector {
    public:
        /**
         * Delete default constructor
         */
        PlanarDetector() = delete;

        /**
         * @brief Constructs a detector in the geometry
         * @param config Configuration object describing the detector
         */
        PlanarDetector(const Configuration& config);


        /**
         * @brief Mark a detector channel as masked
         * @param chX X coordinate of the pixel to be masked
         * @param chY Y coordinate of the pixel to be masked
         */
        void maskChannel(int chX, int chY) override;

        /**
         * @brief Check if a detector channel is masked
         * @param chX X coordinate of the pixel to check
         * @param chY Y coordinate of the pixel to check
         * @return    Mask status of the pixel in question
         */
        bool masked(int chX, int chY) const override;

        // Function to get global intercept with a track
        PositionVector3D<Cartesian3D<double>> getIntercept(const Track* track) const override;
        // Function to get local intercept with a track
        PositionVector3D<Cartesian3D<double>> getLocalIntercept(const Track* track) const override;

        // Function to check if a track intercepts with a plane
        bool hasIntercept(const Track* track, double pixelTolerance = 0.) const override;

        // Function to check if a track goes through/near a masked pixel
        bool hitMasked(Track* track, int tolerance = 0.) const override;

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        // Function to get local position from column (x) and row (y) coordinates
        PositionVector3D<Cartesian3D<double>> getLocalPosition(double column, double row) const override;

        /**
         * Transformation from local (sensor) coordinates to in-pixel coordinates
         * @param  column Column address ranging from int_column-0.5*pitch to int_column+0.5*pitch
         * @param  row Row address ranging from int_column-0.5*pitch to int_column+0.5*pitch
         * @return               Position within a single pixel cell, given in units of length
         */
        XYVector inPixel(const double column, const double row) const override;

        /**
         * Transformation from local (sensor) coordinates to in-pixel coordinates
         * @param  localPosition Local position on the sensor
         * @return               Position within a single pixel cell, given in units of length
         */
        XYVector inPixel(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        /**
         * @brief Check whether given track is within the detector's region-of-interest
         * @param  track The track to be checked
         * @return       Boolean indicating cluster affiliation with region-of-interest
         */
        bool isWithinROI(const Track* track) const override;

        /**
         * @brief Check whether given cluster is within the detector's region-of-interest
         * @param  cluster The cluster to be checked
         * @return         Boolean indicating cluster affiliation with region-of-interest
         */
        bool isWithinROI(Cluster* cluster) const override;

    private:
        // Initialize coordinate transformations
        void initialise() override; 

		// Build axis, for devices which is not auxiliary
		// Different in Planar/Disk Detector
		// better name for not auxiliary?
		void buildNotAuxiliaryAxis(const Configuration& config) override;

		// config
		// better name for not auxiliary?
		void configNotAuxiliary(Configuration& config) const override;

        // Functions to set and check channel masking
        void processMaskFile() override;

		// Seems to be used in other coordinate
        inline static int isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2);
        static int winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon);

    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DETECTOR_H
