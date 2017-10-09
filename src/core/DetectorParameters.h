#ifndef DETECTORPARAMETERS_H
#define DETECTORPARAMETERS_H 1

// Include files
#include <iostream>
#include <list>
#include <map>
#include <string>
#include "Math/Rotation3D.h"
#include "Math/RotationZYX.h"
#include "Math/Transform3D.h"
#include "Math/Translation3D.h"

using namespace ROOT::Math;
using namespace std;

// Class containing just the alignment parameters
class DetectorParameters {
public:
    // Constructors and desctructors
    DetectorParameters() {}
    DetectorParameters(string detectorType,
                       int nPixelsX,
                       int nPixelsY,
                       double pitchX,
                       double pitchY,
                       double x,
                       double y,
                       double z,
                       double Rx,
                       double Ry,
                       double Rz) {

        m_detectorType = detectorType;
        m_nPixelsX = nPixelsX;
        m_nPixelsY = nPixelsY;
        m_pitchX = pitchX;
        m_pitchY = pitchY;
        m_displacementX = x;
        m_displacementY = y;
        m_displacementZ = z;
        m_rotationX = Rx;
        m_rotationY = Ry;
        m_rotationZ = Rz;

        //    this->initialise();
    }
    ~DetectorParameters() {}

    // Functions to retrieve basic information
    string type() { return m_detectorType; }
    double pitchX() { return m_pitchX; }
    double pitchY() { return m_pitchY; }
    int nPixelsX() { return m_nPixelsX; }
    int nPixelsY() { return m_nPixelsY; }

    // Functions to set and retrieve basic translation parameters
    void displacementX(float x) { m_displacementX = x; }
    void displacementY(float y) { m_displacementY = y; }
    void displacementZ(float z) { m_displacementZ = z; }
    float displacementX() { return m_displacementX; }
    float displacementY() { return m_displacementY; }
    float displacementZ() { return m_displacementZ; }

    // Functions to set and retrieve basic rotation parameters
    void rotationX(float rx) { m_rotationX = rx; }
    void rotationY(float ry) { m_rotationY = ry; }
    void rotationZ(float rz) { m_rotationZ = rz; }
    float rotationX() { return m_rotationX; }
    float rotationY() { return m_rotationY; }
    float rotationZ() { return m_rotationZ; }

    // Function to initialise transforms
    void initialise() {
        m_translations = new Translation3D(m_displacementX, m_displacementY, m_displacementZ);
        RotationZYX zyxRotation(m_rotationZ, m_rotationY, m_rotationX);
        m_rotations = new Rotation3D(zyxRotation);
        m_localToGlobal = new Transform3D(*m_rotations, *m_translations);
        m_globalToLocal = new Transform3D();
        (*m_globalToLocal) = m_localToGlobal->Inverse();
    }

    // Member variables

    // Detector information
    string m_detectorType;
    double m_pitchX;
    double m_pitchY;
    int m_nPixelsX;
    int m_nPixelsY;

    // Displacement and rotation in x,y,z
    float m_displacementX;
    float m_displacementY;
    float m_displacementZ;
    float m_rotationX;
    float m_rotationY;
    float m_rotationZ;

    // Rotation and translation objects
    Translation3D* m_translations;
    Rotation3D* m_rotations;

    // Transforms from local to global and back
    Transform3D* m_localToGlobal;
    Transform3D* m_globalToLocal;
};

#endif // DETECTORPARAMETERS_H
