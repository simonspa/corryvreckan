#ifndef DET_PARAMETERS_H
#define DET_PARAMETERS_H 1

#include <fstream>
#include <map>
#include <string>

// Root includes
#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include "Math/Point3D.h"
#include "Math/Rotation3D.h"
#include "Math/RotationZYX.h"
#include "Math/RotationX.h"
#include "Math/RotationY.h"
#include "Math/RotationZ.h"
#include "Math/Transform3D.h"
#include "Math/Translation3D.h"
#include "Math/Vector3D.h"

#include "config/Configuration.hpp"
#include "objects/Track.h"
#include "utils/ROOT.h"
#include "utils/log.h"

using namespace ROOT::Math;

namespace corryvreckan {
    // Class containing just the detector parameters
    class Detector {
    public:
        // Constructors and desctructors
        Detector();
        Detector(const Configuration& config);
        ~Detector() {}

        // Functions to retrieve basic information
        std::string type() { return m_detectorType; }
        std::string name() { return m_detectorName; }

        Configuration getConfiguration();

        double pitchX() { return m_pitchX; }
        double pitchY() { return m_pitchY; }
        int nPixelsX() { return m_nPixelsX; }
        int nPixelsY() { return m_nPixelsY; }
        double timingOffset() { return m_timingOffset; }

        // Functions to set and retrieve basic translation parameters
        void displacementX(double x) { m_displacementX = x; }
        void displacementY(double y) { m_displacementY = y; }
        void displacementZ(double z) { m_displacementZ = z; }
        double displacementX() { return m_displacementX; }
        double displacementY() { return m_displacementY; }
        double displacementZ() { return m_displacementZ; }

        // Functions to set and retrieve basic rotation parameters
        void rotationX(double rx) { m_rotationX = rx; }
        void rotationY(double ry) { m_rotationY = ry; }
        void rotationZ(double rz) { m_rotationZ = rz; }
        double rotationX() { return m_rotationX; }
        double rotationY() { return m_rotationY; }
        double rotationZ() { return m_rotationZ; }

        // Functions to set and check channel masking
        void setMaskFile(std::string file);
        void processMaskFile();

        std::string maskFile() { return m_maskfile; }
        void maskChannel(int chX, int chY);
        bool masked(int chX, int chY);

        // Function to initialise transforms
        void initialise();

        // Function to update transforms (such as during alignment)
        void update();

        // Function to get global intercept with a track
        PositionVector3D<Cartesian3D<double>> getIntercept(Track* track);

        // Function to check if a track intercepts with a plane
        bool hasIntercept(Track* track, double pixelTolerance = 0.);

        // Function to check if a track goes through/near a masked pixel
        bool hitMasked(Track* track, int tolerance = 0.);

        // Functions to get row and column from local position
        double getRow(PositionVector3D<Cartesian3D<double>> localPosition);
        double getColumn(PositionVector3D<Cartesian3D<double>> localPosition);

        // Function to get local position from row and column
        PositionVector3D<Cartesian3D<double>> getLocalPosition(double row, double column);

        // Function to get in-pixel position (value returned in microns)
        double inPixelX(PositionVector3D<Cartesian3D<double>> localPosition);
        double inPixelY(PositionVector3D<Cartesian3D<double>> localPosition);

        // Member variables

        // Detector information
        std::string m_detectorType;
        std::string m_detectorName;
        double m_pitchX;
        double m_pitchY;
        int m_nPixelsX;
        int m_nPixelsY;
        double m_timingOffset;

        // Displacement and rotation in x,y,z
        double m_displacementX;
        double m_displacementY;
        double m_displacementZ;
        double m_rotationX;
        double m_rotationY;
        double m_rotationZ;

        // Rotation and translation objects
        Translation3D* m_translations;
        Rotation3D* m_rotations;

        // Transforms from local to global and back
        Transform3D* m_localToGlobal;
        Transform3D* m_globalToLocal;

        // Normal to the detector surface and point on the surface
        PositionVector3D<Cartesian3D<double>> m_normal;
        PositionVector3D<Cartesian3D<double>> m_origin;

        // List of masked channels
        std::map<int, bool> m_masked;
        std::string m_maskfile;
        std::string m_maskfile_name;
    };
}

#endif // DET_PARAMETERS_H
