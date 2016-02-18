#ifndef PARAMETERS_H 
#define PARAMETERS_H 1

// Include files
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <vector>
// Root includes
#include "Math/Translation3D.h"
#include "Math/Rotation3D.h"
#include "Math/RotationZYX.h"
#include "Math/Transform3D.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
// Local includes
#include "Track.h"

using namespace std;
using namespace ROOT::Math;

//-------------------------------------------------------------------------------
// Two classes: DetectorParameters and Parameters. The conditions file read at
// the beginning of each run contains a set of information (like pitch, number of
// channels, etc.) that is held for each detector in its own DetectorParameters
// object. These are retrieved through the global Parameters class, which gives
// access to variables/information at any point through the event processing.
//-------------------------------------------------------------------------------

// Class containing just the detector parameters
class DetectorParameters {
public:
  
  // Constructors and desctructors
  DetectorParameters(){}
  DetectorParameters(string detectorType, double nPixelsX, double nPixelsY, double pitchX, double pitchY,
                     double x, double y, double z, double Rx, double Ry, double Rz, double timingOffset){
    
    m_detectorType = detectorType;
    m_nPixelsX = nPixelsX;
    m_nPixelsY = nPixelsY;
    m_pitchX = pitchX/1000.;
    m_pitchY = pitchY/1000.;
    m_displacementX = x;
    m_displacementY = y;
    m_displacementZ = z;
    m_rotationX = Rx;
    m_rotationY = Ry;
    m_rotationZ = Rz;
    m_timingOffset = timingOffset;
    
    this->initialise();
  }
  ~DetectorParameters(){}
  
  // Functions to retrieve basic information
  string type(){return m_detectorType;}
  double pitchX(){return m_pitchX;}
  double pitchY(){return m_pitchY;}
  int nPixelsX(){return m_nPixelsX;}
  int nPixelsY(){return m_nPixelsY;}
  double timingOffset(){return m_timingOffset;}
  
  // Functions to set and retrieve basic translation parameters
  void displacementX(double x){m_displacementX=x;}
  void displacementY(double y){m_displacementY=y;}
  void displacementZ(double z){m_displacementZ=z;}
  double displacementX(){return m_displacementX;}
  double displacementY(){return m_displacementY;}
  double displacementZ(){return m_displacementZ;}
  
  // Functions to set and retrieve basic rotation parameters
  void rotationX(double rx){m_rotationX=rx;}
  void rotationY(double ry){m_rotationY=ry;}
  void rotationZ(double rz){m_rotationZ=rz;}
  double rotationX(){return m_rotationX;}
  double rotationY(){return m_rotationY;}
  double rotationZ(){return m_rotationZ;}
  
  // Functions to set and check channel masking
  void setMaskFile(string file){m_maskfile = file;}
  string maskFile(){return m_maskfile;}
  void maskChannel(int chX, int chY){
    int channelID = chX + m_nPixelsX*chY;
    m_masked[channelID] = true;
  }
  bool masked(int chX, int chY){
    int channelID = chX + m_nPixelsX*chY;
    if(m_masked.count(channelID) > 0) return true;
    return false;
  }
  
  // Function to initialise transforms
  void initialise(){
    
    // Make the local to global transform, built from a displacement and rotation
    m_translations = new Translation3D(m_displacementX,m_displacementY,m_displacementZ);
    RotationZYX zyxRotation(m_rotationZ, m_rotationY, m_rotationX);
    m_rotations = new Rotation3D(zyxRotation);
    m_localToGlobal = new Transform3D(*m_rotations,*m_translations);
    m_globalToLocal = new Transform3D();
    (*m_globalToLocal) = m_localToGlobal->Inverse();
    
    // Find the normal to the detector surface. Build two points, the origin and a unit step in z,
    // transform these points to the global co-ordinate frame and then make a vector pointing between them
    m_origin = PositionVector3D<Cartesian3D<double> >(0.,0.,0.);
    m_origin = (*m_localToGlobal) * m_origin;
    PositionVector3D<Cartesian3D<double> > localZ(0.,0.,1.);
    localZ = (*m_localToGlobal) * localZ;
    m_normal = PositionVector3D<Cartesian3D<double> >( localZ.X()-m_origin.X(), localZ.Y()-m_origin.Y(), localZ.Z()-m_origin.Z() );

  }
  
  // Function to update transforms (such as during alignment)
  void update(){
    delete m_translations;
    delete m_rotations;
    delete m_localToGlobal;
    delete m_globalToLocal;
    this->initialise();
  }
  
  // Function to get global intercept with a track
  PositionVector3D<Cartesian3D<double> > getIntercept(Track* track){
    
    // Get the distance from the plane to the track initial state
    double distance = (m_origin.X() - track->m_state.X()) * m_normal.X();
    distance += (m_origin.Y() - track->m_state.Y()) * m_normal.Y();
    distance += (m_origin.Z() - track->m_state.Z()) * m_normal.Z();
    distance /= (track->m_direction.X()*m_normal.X() + track->m_direction.Y()*m_normal.Y() + track->m_direction.Z()*m_normal.Z());
    
    // Propagate the track
    PositionVector3D< Cartesian3D<double> > globalIntercept(track->m_state.X() + distance*track->m_direction.X(),
                                                            track->m_state.Y() + distance*track->m_direction.Y(),
                                                            track->m_state.Z() + distance*track->m_direction.Z());
    return globalIntercept;
  }
  
  // Functions to get row and column from local position
  double getRow(PositionVector3D<Cartesian3D<double> > localPosition){
    double row = (localPosition.Y()/m_pitchY) + m_nPixelsY/2.;
    return row;
  }
  double getColumn(PositionVector3D<Cartesian3D<double> > localPosition){
    double column = (localPosition.X()/m_pitchX) + m_nPixelsX/2.;
    return column;
  }
  
  // Function to get local position from row and column
  PositionVector3D<Cartesian3D<double> > getLocalPosition(double row, double column){
    
    PositionVector3D<Cartesian3D<double> > positionLocal(m_pitchX * (column-m_nPixelsX/2.),
                                                         m_pitchY * (row-m_nPixelsY/2.),
                                                         0.);
    
    return positionLocal;
  }
  
  // Functiona to get in-pixel position
  double inPixelX(PositionVector3D<Cartesian3D<double> > localPosition){
    double column = getColumn(localPosition);
    double inPixelX = m_pitchX * (column + m_pitchX/2. - floor(column + m_pitchX/2.));
    return 1000.*inPixelX;
  }
  double inPixelY(PositionVector3D<Cartesian3D<double> > localPosition){
    double row = getRow(localPosition);
    double inPixelY = m_pitchY * (row + m_pitchY/2. - floor(row + m_pitchY/2.));
    return 1000.*inPixelY;
  }
  
  // Member variables
  
  // Detector information
  string m_detectorType;
  double m_pitchX;
  double m_pitchY;
  double m_nPixelsX;
  double m_nPixelsY;
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
  PositionVector3D<Cartesian3D<double> > m_normal;
  PositionVector3D<Cartesian3D<double> > m_origin;
  
  // List of masked channels
  map<int,bool> m_masked;
  string m_maskfile;
};

class Parameters {
  
public:
  
  // Constructors and destructors
  Parameters();
  virtual ~Parameters(){};
  void help();
	
  // Functions
  bool readConditions();
  bool writeConditions();
  void readCommandLineOptions(int, char**);
  void registerDetector(string detectorID){
    nDetectors++;
    detectors.push_back(detectorID);
  }
  
  // Member variables
  string histogramFile;
  string inputFile;
  string inputDirectory;
  string conditionsFile;
  string outputTupleFile;
  vector<string> detectors;
  int nDetectors;
  string reference;
  string dut;
  double currentTime;
  double eventLength;
  int nEvents;
  bool align;
  bool eventDisplay;
  bool gui;
  bool produceMask;
  string DUT;
  map<string, bool> excludedFromTracking;
  map<string, bool> masked;
  string detectorToAlign;
  int alignmentMethod;
  
  // Parameters for each detector (stored by detector ID)
  map<string,DetectorParameters*> detector;
  
};

#endif // PARAMETERS_H
