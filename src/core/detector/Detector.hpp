/** @file
 *  @brief Detector model class
 *  @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_DETECTOR_H
#define CORRYVRECKAN_DETECTOR_H

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

namespace corryvreckan {
    using namespace ROOT::Math;

    /**
     * @brief Role of the detector
     */
    enum class DetectorRole : int {
        NONE = 0x0,      ///< No specific detector role
        REFERENCE = 0x1, ///< Reference detector
        DUT = 0x2,       ///< Detector used as device under test
        AUXILIARY = 0x4, ///< Auxiliary device which should not participate in regular reconstruction but might provide
                         /// additional information
    };

    inline constexpr DetectorRole operator&(DetectorRole x, DetectorRole y) {
        return static_cast<DetectorRole>(static_cast<int>(x) & static_cast<int>(y));
    }

    inline constexpr DetectorRole operator|(DetectorRole x, DetectorRole y) {
        return static_cast<DetectorRole>(static_cast<int>(x) | static_cast<int>(y));
    }

    inline DetectorRole& operator&=(DetectorRole& x, DetectorRole y) {
        x = x & y;
        return x;
    }

    inline DetectorRole& operator|=(DetectorRole& x, DetectorRole y) {
        x = x | y;
        return x;
    }

    /**
     * @brief Detector representation in the reconstruction chain
     *
     * Contains the detector with all its properties such as type, name, position and orientation, pitch, spatial resolution
     * etc.
     */
    class Detector {
    public:
        /**
         * Delete default constructor
         */
        Detector() = delete;

        /**
         * @brief Constructs a detector in the geometry
         * @param config Configuration object describing the detector
         */
        Detector(const Configuration& config);

        /**
         * @brief Get type of the detector
         * @return Type of the detector model
         */
        std::string type() const;

        /**
         * @brief Get name of the detector
         * @return Detector name
         */
        std::string name() const;

        /**
         * @brief Check whether detector is registered as reference
         * @return Reference status
         */
        bool isReference() const;

        /**
         * @brief Check whether detector is registered as DUT
         * @return DUT status
         */
        bool isDUT() const;

        /**
         * @brief Check whether detector is registered as auxiliary device and should not parttake in the reconstruction
         * @return Auxiliary status
         */
        bool isAuxiliary() const;

        /**
         * @brief Retrieve configuration object from detector, containing all (potentially updated) parameters
         * @return Configuration object for this detector
         */
        Configuration getConfiguration() const;

        /**
         * @brief Get the total size of the active matrix, i.e. pitch * number of pixels in both dimensions
         * @return 2D vector with the dimensions of the pixle matrix in X and Y
         */
        XYVector size() const;

        /**
         * @brief Get pitch of a single pixel
         * @return Pitch of a pixel
         */
        XYVector pitch() const { return m_pitch; }

        /**
         * @brief Get intrinsic spatial resolution of the detector
         * @return Intrinsic spatial resolution in X and Y
         */
        XYVector getSpatialResolution() const { return m_spatial_resolution; }

        /**
         * @brief Get number of pixels in x and y
         * @return Number of two dimensional pixels
         */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> nPixels() const { return m_nPixels; }

        /**
         * @brief Get detector time offset from global clock, can be used to correct for constant shifts or time of flight
         * @return Time offset of respective detector
         */
        double timeOffset() const { return m_timeOffset; }

        /**
         * @brief Get detector time resolution, used for timing cuts during clustering, track formation, etc.
         * @return Time resolutiom of respective detector
         */
        double getTimeResolution() const { return m_timeResolution; }

        /**
         * @brief Update detector position in the world
         * @param displacement Vector with three position coordinates
         */
        void displacement(XYZPoint displacement) { m_displacement = displacement; }

        /**
         * @brief Get position in the world
         * @return Global position in Cartesian coordinates
         */
        XYZPoint displacement() const { return m_displacement; }

        /**
         * @brief Get orientation in the world
         * @return Vector with three rotation angles
         */
        XYZVector rotation() const { return m_orientation; }

        /**
         * @brief Update detector orientation in the world
         * @param rotation Vector with three rotation angles
         */
        void rotation(XYZVector rotation) { m_orientation = rotation; }

        /**
         * @brief Get normal vector to sensor surface
         * @return Normal vector to sensor surface
         */
        PositionVector3D<Cartesian3D<double>> normal() const { return m_normal; };

        /**
         * @brief Get path of the file with pixel mask information
         * @return Path of the pixel mask file
         */
        std::string maskFile() const { return m_maskfile; }

        /**
         * @brief Mark a detector channel as masked
         * @param chX X coordinate of the pixel to be masked
         * @param chY Y coordinate of the pixel to be masked
         */
        void maskChannel(int chX, int chY);

        /**
         * @brief Check if a detector channel is masked
         * @param chX X coordinate of the pixel to check
         * @param chY Y coordinate of the pixel to check
         * @return    Mask status of the pixel in question
         */
        bool masked(int chX, int chY) const;

        /**
         * @brief Update coordinate transformations based on currently configured position and orientation values
         */
        void update();

        // Function to get global intercept with a track
        PositionVector3D<Cartesian3D<double>> getIntercept(const Track* track) const;
        // Function to get local intercept with a track
        PositionVector3D<Cartesian3D<double>> getLocalIntercept(const Track* track) const;

        // Function to check if a track intercepts with a plane
        bool hasIntercept(const Track* track, double pixelTolerance = 0.) const;

        // Function to check if a track goes through/near a masked pixel
        bool hitMasked(Track* track, int tolerance = 0.) const;

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition) const;
        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition) const;

        // Function to get local position from column (x) and row (y) coordinates
        PositionVector3D<Cartesian3D<double>> getLocalPosition(double column, double row) const;

        /**
         * Transformation from local (sensor) coordinates to in-pixel coordinates
         * @param  column Column address ranging from int_column-0.5*pitch to int_column+0.5*pitch
         * @param  row Row address ranging from int_column-0.5*pitch to int_column+0.5*pitch
         * @return               Position within a single pixel cell, given in units of length
         */
        XYVector inPixel(const double column, const double row) const;

        /**
         * Transformation from local (sensor) coordinates to in-pixel coordinates
         * @param  localPosition Local position on the sensor
         * @return               Position within a single pixel cell, given in units of length
         */
        XYVector inPixel(PositionVector3D<Cartesian3D<double>> localPosition) const;

        /**
         * @brief Transform local coordinates of this detector into global coordinates
         * @param  local Local coordinates in the reference frame of this detector
         * @return       Global coordinates
         */
        XYZPoint localToGlobal(XYZPoint local) const { return m_localToGlobal * local; };

        /**
         * @brief Transform global coordinates into detector-local coordinates
         * @param  global Global coordinates
         * @return        Local coordinates in the reference frame of this detector
         */
        XYZPoint globalToLocal(XYZPoint global) const { return m_globalToLocal * global; };

        /**
         * @brief Check whether given track is within the detector's region-of-interest
         * @param  track The track to be checked
         * @return       Boolean indicating cluster affiliation with region-of-interest
         */
        bool isWithinROI(const Track* track) const;

        /**
         * @brief Check whether given cluster is within the detector's region-of-interest
         * @param  cluster The cluster to be checked
         * @return         Boolean indicating cluster affiliation with region-of-interest
         */
        bool isWithinROI(Cluster* cluster) const;

        /**
         * @brief Return the thickness of the senosr assembly layer (sensor+support) in fractions of radiation length
         * @return thickness in fractions of radiation length
         */
        double materialBudget() const { return m_materialBudget; }

    private:
        // Roles of the detector
        DetectorRole m_role;

        // Initialize coordinate transformations
        void initialise();

        // Functions to set and check channel masking
        void setMaskFile(std::string file);
        void processMaskFile();

        // Detector information
        std::string m_detectorType;
        std::string m_detectorName;
        XYVector m_pitch{};
        XYVector m_spatial_resolution{};
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> m_nPixels{};
        double m_timeOffset;
        double m_timeResolution;
        double m_materialBudget;

        std::vector<std::vector<int>> m_roi{};
        static int winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon);
        inline static int isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2);

        // Displacement and rotation in x,y,z
        ROOT::Math::XYZPoint m_displacement;
        ROOT::Math::XYZVector m_orientation;
        std::string m_orientation_mode;

        // Transforms from local to global and back
        Transform3D m_localToGlobal;
        Transform3D m_globalToLocal;

        // Normal to the detector surface and point on the surface
        PositionVector3D<Cartesian3D<double>> m_normal;
        PositionVector3D<Cartesian3D<double>> m_origin;

        // List of masked channels
        std::map<int, bool> m_masked;
        std::string m_maskfile;
        std::string m_maskfile_name;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DETECTOR_H
