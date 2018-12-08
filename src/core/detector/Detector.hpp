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
     * Contains the detector with all its properties such as type, name, position and orientation, pitch, resolution etc.
     */
    class Detector {
    public:
        // Constructors and desctructors
        Detector() = delete;
        Detector(const Configuration& config);
        ~Detector() {}

        // Functions to retrieve basic information
        const std::string type() const { return m_detectorType; }
        const std::string name() const { return m_detectorName; }

        // Detector role and helper functions
        bool isReference() const;
        bool isDUT() const;

        Configuration getConfiguration();

        XYVector size() const { return XYVector(m_pitch.X() * m_nPixelsX, m_pitch.Y() * m_nPixelsY); }
        XYVector pitch() const { return m_pitch; }
        XYVector resolution() const { return m_resolution; }

        int nPixelsX() const { return m_nPixelsX; }
        int nPixelsY() const { return m_nPixelsY; }
        double timingOffset() const { return m_timingOffset; }

        // Functions to set and retrieve basic translation parameters
        void displacementX(double x) { m_displacement.SetX(x); }
        void displacementY(double y) { m_displacement.SetY(y); }
        void displacementZ(double z) { m_displacement.SetZ(z); }
        void displacement(XYZPoint displacement) { m_displacement = displacement; }
        XYZPoint displacement() const { return m_displacement; }

        // Functions to set and retrieve basic rotation parameters
        void rotationX(double rx) { m_orientation.SetX(rx); }
        void rotationY(double ry) { m_orientation.SetY(ry); }
        void rotationZ(double rz) { m_orientation.SetZ(rz); }
        XYZVector rotation() const { return m_orientation; }
        void rotation(XYZVector rotation) { m_orientation = rotation; }

        PositionVector3D<Cartesian3D<double>> normal() { return m_normal; };

        std::string maskFile() { return m_maskfile; }
        void maskChannel(int chX, int chY);
        bool masked(int chX, int chY);

        // Function to initialise transforms
        void initialise();

        // Function to update transforms (such as during alignment)
        void update();

        // Function to get global intercept with a track
        PositionVector3D<Cartesian3D<double>> getIntercept(const Track* track);
        // Function to get local intercept with a track
        PositionVector3D<Cartesian3D<double>> getLocalIntercept(const Track* track);

        // Function to check if a track intercepts with a plane
        bool hasIntercept(const Track* track, double pixelTolerance = 0.);

        // Function to check if a track goes through/near a masked pixel
        bool hitMasked(Track* track, int tolerance = 0.);

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition);
        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition);

        // Function to get local position from row and column
        PositionVector3D<Cartesian3D<double>> getLocalPosition(double row, double column);

        // Function to get in-pixel position
        double inPixelX(PositionVector3D<Cartesian3D<double>> localPosition);
        double inPixelY(PositionVector3D<Cartesian3D<double>> localPosition);

        XYZPoint localToGlobal(XYZPoint local) { return m_localToGlobal * local; };
        XYZPoint globalToLocal(XYZPoint global) { return m_globalToLocal * global; };

        bool isWithinROI(const Track* track);
        bool isWithinROI(Cluster* cluster);

    private:
        DetectorRole m_role;

        // Functions to set and check channel masking
        void setMaskFile(std::string file);
        void processMaskFile();

        // Detector information
        std::string m_detectorType;
        std::string m_detectorName;
        XYVector m_pitch;
        XYVector m_resolution;
        int m_nPixelsX;
        int m_nPixelsY;
        double m_timingOffset;

        std::vector<std::vector<int>> m_roi;
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
