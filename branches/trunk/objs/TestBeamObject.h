#ifndef TESTBEAMOBJECT_H
#define TESTBEAMOBJECT_H 1

// Include files
#include <vector>
#include <string>
#include "TObject.h"

//-------------------------------------------------------------------------------
// Generic base class. Every class which inherits from TestBeamObject can be
// placed on the clipboard and written out to file.
//-------------------------------------------------------------------------------

class TestBeamObject : public TObject{

public:
  
  // Constructors and destructors
  TestBeamObject(){}
  virtual ~TestBeamObject(){}
  
  // Methods to get member variables
  std::string getDetectorID(){return m_detectorID;}
//  double getTimeStamp(){return m_timeStamp;}

  // Methods to set member variables
  void setDetectorID(std::string detectorID){m_detectorID = detectorID;}
  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(TestBeamObject,1)
    
  // Member variables
  std::string m_detectorID;
  
};

// Vector type declaration
typedef std::vector<TestBeamObject*> TestBeamObjects;

#endif // TESTBEAMOBJECT_H
