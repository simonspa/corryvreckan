// local
#include "TestBeamObject.h"
#include "Timepix1Cluster.h"
#include "Timepix1Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Pixel.h"
#include "Timepix3Track.h"

ClassImp(TestBeamObject)

// Return class type for fixed object types (that don't depend on detector type)
TestBeamObject* TestBeamObject::Factory(std::string objectType){
  
  // Timepix3 track class
  if(objectType == "timepix3track") return new Timepix3Track();
  
}

//Return class type for objects which change with detector type
TestBeamObject* TestBeamObject::Factory(std::string detectorType, std::string objectType, TestBeamObject* object){
  
  // Timepix1 class types
  if(detectorType == "Timepix1"){
    if(objectType == "pixels") return (object == NULL) ? new Timepix1Pixel() : new Timepix1Pixel(*(Timepix1Pixel*)object);
    if(objectType == "clusters") return (object == NULL) ? new Timepix1Cluster() : new Timepix1Cluster(*(Timepix1Cluster*)object);
  }
  
  // Timepix3 class types
  if(detectorType == "Timepix3"){
    if(objectType == "pixels") return (object == NULL) ? new Timepix3Pixel() : new Timepix3Pixel(*(Timepix3Pixel*)object);
    if(objectType == "clusters") return (object == NULL) ? new Timepix3Cluster() : new Timepix3Cluster(*(Timepix3Cluster*)object);
  }
  
}

