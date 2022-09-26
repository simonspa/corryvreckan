/** @file
 *  @brief Detector model class
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_HEXAGONALDETECTOR_H
#define CORRYVRECKAN_HEXAGONALDETECTOR_H

#include <fstream>
#include <map>
#include <string>

#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include "Math/Transform3D.h"
#include "Math/Vector3D.h"

#include "Detector.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/ROOT.h"
#include "core/utils/log.h"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {

    /**
     * @brief PixelDetector representation derived from Detector interface in the reconstruction chain
     *
     * Contains the PixelDetector with all its properties such as position and orientation, pitch, spatial resolution
     * etc.
     */
    class HexagonalPixelDetector : public PixelDetector {
    public:
        /**
         * Delete default constructor
         */
        HexagonalPixelDetector() = delete;

        /**
         * Default destructor
         */
        ~HexagonalPixelDetector() = default;

        /**
         * @brief Constructs a detector in the geometry
         * @param config Configuration object describing the detector
         */
        HexagonalPixelDetector(const Configuration& config);

        // Function to check if a track intercepts with a plane
        bool hasIntercept(const Track* track, double pixelTolerance = 0.) const override;

        // Function to check if a track goes through/near a masked pixel
        bool hitMasked(const Track* track, int tolerance = 0.) const override;

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        /**
         * @brief Checks if a given pixel index lies within the pixel matrix of the detector
         * @return True if pixel index is within matrix bounds, false otherwise
         */
        bool isWithinMatrix(const int col, const int row) const override;

        // Function to get local position from column (x) and row (y) coordinates
        PositionVector3D<Cartesian3D<double>> getLocalPosition(double column, double row) const override;

        // Function to get row and column of pixel
        std::pair<int, int> getInterceptPixel(PositionVector3D<Cartesian3D<double>> localPosition) const override;

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

        /**
         * @brief Get the total size of the active matrix, i.e. pitch * number of pixels in both dimensions
         * @return 2D vector with the dimensions of the pixle matrix in X and Y
         */
        XYVector getSize() const override;

        /**
         * @brief Test whether one pixel touches the cluster
         * @return true if it fulfills the condition
         * @note users should define their specific clustering method in the detector class, for pixel detector, the default
         * is 2D clustering
         */
        bool isNeighbor(const std::shared_ptr<Pixel>&, const std::shared_ptr<Cluster>&, const int, const int) const override;

        std::set<std::pair<int, int>>
        getNeighbors(const std::shared_ptr<Pixel>& px, const size_t distance, const bool include_corners) const override;

    private:
        std::pair<int, int> round_to_nearest_hex(double x, double y) const;
        size_t hex_distance(double x1, double y1, double x2, double y2) const;

        double m_height;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_HEXAGONALDETECTOR_H
