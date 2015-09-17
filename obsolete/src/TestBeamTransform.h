// $Id: TestBeamTransform.h,v 1.2 2009-07-17 15:56:21 gligorov Exp $
#ifndef TESTBEAMTRANSFORM_H 
#define TESTBEAMTRANSFORM_H 1

// Include files
#include "TestBeamObject.h"
#include "Parameters.h"
#include "TestBeamEventElement.h"

#include "TMatrixD.h"
#include "TVectorD.h"
#include "TMath.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/Rotation3D.h"
#include "Math/EulerAngles.h"
#include "Math/AxisAngle.h"
#include "Math/Quaternion.h"
#include "Math/RotationX.h"
#include "Math/RotationY.h"
#include "Math/RotationZ.h"
#include "Math/RotationZYX.h"
#include "Math/LorentzRotation.h"
#include "Math/Boost.h"
#include "Math/BoostX.h"
#include "Math/BoostY.h"
#include "Math/BoostZ.h"
#include "Math/Transform3D.h"
#include "Math/Plane3D.h"
#include "Math/VectorUtil.h"
#include "Math/Translation3D.h"
#include "Math/PositionVector3D.h"

using namespace ROOT::Math;
using namespace std;

/** @class TestBeamTransform TestBeamTransform.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

class TestBeamTransform : public TestBeamObject {
public: 
  TestBeamTransform(float,float,float,float,float,float); 
  TestBeamTransform(Parameters*, std::string);
  TestBeamTransform(TestBeamTransform&,bool=COPY);
  ~TestBeamTransform(){delete m_transform;}
 
  Transform3D localToGlobalTransform(){return *m_transform;};
  Transform3D globalToLocalTransform(){return m_transform->Inverse();};	 
 
protected:

private:
  Transform3D* m_transform;
 
};

inline TestBeamTransform::TestBeamTransform(Parameters* parameters, std::string dID) :
  m_transform(0) {
  
  // Constructor from a pointer to parameters and the ID of the chip being transformed
  const float tx = parameters->alignment[dID]->displacementX();
  const float ty = parameters->alignment[dID]->displacementY();
  const float tz = parameters->alignment[dID]->displacementZ();
  const float rx = parameters->alignment[dID]->rotationX();
  const float ry = parameters->alignment[dID]->rotationY();
  const float rz = parameters->alignment[dID]->rotationZ();
  Translation3D move(tx, ty, tz);
  RotationZYX twist(rz, ry, rx);
  Rotation3D twister(twist);
  m_transform = new Transform3D(twister, move);
 
}

inline TestBeamTransform::TestBeamTransform(float translationX, float translationY, float translationZ, float rotationX, float rotationY, float rotationZ)
{
  //The constructor defines the transform, which for us is from the local to the global coordinate system
  //and will be read from the alignment file. To transform from the global to the local, we simply
  //get the inverse later on.  
  m_transform = NULL;
  Translation3D move(translationX, translationY, translationZ);
  RotationZYX twist(rotationZ, rotationY, rotationX);
  Rotation3D twister(twist);
  m_transform = new Transform3D(twister,move);
  
} 

inline TestBeamTransform::TestBeamTransform(TestBeamTransform& c,bool action) : TestBeamObject()
{
  if (action == COPY) return;
  m_transform = c.m_transform;
} 

#endif // TESTBEAMTRANSFORM_H
