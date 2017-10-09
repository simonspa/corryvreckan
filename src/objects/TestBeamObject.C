// local
#include "Cluster.h"
#include "Pixel.h"
#include "TestBeamObject.h"
#include "Track.h"

ClassImp(TestBeamObject)

    // Return class type for fixed object types (that don't depend on detector
    // type)
    TestBeamObject* TestBeamObject::Factory(std::string objectType, TestBeamObject* object) {

    // Track class
    if(objectType == "tracks")
        return (object == NULL) ? new Track() : new Track(*(Track*)object);
}

// Return class type for objects which change with detector type
TestBeamObject* TestBeamObject::Factory(std::string detectorType, std::string objectType, TestBeamObject* object) {

    // Timepix types both use generic classes
    if(detectorType == "Timepix1" || detectorType == "Timepix3") {
        if(objectType == "pixels")
            return (object == NULL) ? new Pixel() : new Pixel(*(Pixel*)object);
        if(objectType == "clusters")
            return (object == NULL) ? new Cluster() : new Cluster(*(Cluster*)object);
    }
}
