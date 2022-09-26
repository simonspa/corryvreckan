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
    class PixelDetector : public Detector {
    public:
        /**
         * Delete default constructor
         */
        PixelDetector() = delete;

        /**
         * Default destructor
         */
        ~PixelDetector() override = default;

        /**
         * @brief Constructs a detector in the geometry
         * @param config Configuration object describing the detector
         */
        explicit PixelDetector(const Configuration& config);

        /**
         * @brief Set position and orientation from configuration file
         */
        void SetPostionAndOrientation(const Configuration& config);

        /**
         * @brief Checks if a given pixel index lies within the pixel matrix of the detector
         * @return True if pixel index is within matrix bounds, false otherwise
         */
        bool isWithinMatrix(const int col, const int row) const override;

        /**
         * @brief Update detector position in the world
         * @param displacement Vector with three position coordinates
         */
        void displacement(XYZPoint displacement) override { m_displacement = displacement; }

        /**
         * @brief Get position in the world
         * @return Global position in Cartesian coordinates
         */
        XYZPoint displacement() const override { return m_displacement; }

        /**
         * @brief Get orientation in the world
         * @return Vector with three rotation angles
         */
        XYZVector rotation() const override { return m_orientation; }

        /**
         * @brief Update detector orientation in the world
         * @param rotation Vector with three rotation angles
         */
        void rotation(XYZVector rotation) override { m_orientation = rotation; }

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
        bool hitMasked(const Track* track, int tolerance = 0.) const override;

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition) const override;

        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition) const override;

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
         * @brief Get pitch of a single pixel
         * @return Pitch of a pixel
         */
        XYVector getPitch() const override { return m_pitch; }

        /**
         * @brief Get intrinsic spatial resolution of the detector
         * @return Intrinsic spatial resolution in X and Y
         */
        XYVector getSpatialResolution() const override { return m_spatial_resolution; }

        /*
         * @brief Get number of pixels in x and y
         * @return Number of two dimensional pixels
         */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> nPixels() const override { return m_nPixels; }

        /**
         * @brief Test whether one pixel touches the cluster
         * @return true if it fulfills the condition
         * @note users should define their specific clustering method in the detector class, for pixel detector, the default
         * is 2D clustering
         */
        bool isNeighbor(const std::shared_ptr<Pixel>&, const std::shared_ptr<Cluster>&, const int, const int) const override;

        std::set<std::pair<int, int>>
        getNeighbors(const std::shared_ptr<Pixel>& px, const size_t distance, const bool include_corners) const override;

    protected:
        // Initialize coordinate transformations
        void initialise() override;

        // Build axis, for devices which are not auxiliary
        // Different in Pixel/Strip Detector
        void build_axes(const Configuration& config) override;

        // Config detector, for devices which are not auxiliary
        // Different in Pixel/Strip Detector
        void configure_detector(Configuration& config) const override;

        // Config position, orientation, mode of detector
        // Different in Pixel/Strip Detector
        void configure_pos_and_orientation(Configuration& config) const override;

        // Functions to set and check channel masking
        void process_mask_file() override;

        // Seems to be used in other coordinate
        inline static int isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2);
        static int winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon);

        // For planar detector
        XYVector m_pitch{};
        XYVector m_spatial_resolution{};
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> m_nPixels{};
        std::vector<std::vector<int>> m_roi{};
        // Displacement and rotation in x,y,z
        ROOT::Math::XYZPoint m_displacement;
        ROOT::Math::XYZVector m_orientation;
        std::string m_orientation_mode;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DETECTOR_H
