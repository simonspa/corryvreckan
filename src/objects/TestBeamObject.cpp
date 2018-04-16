// local
#include "TestBeamObject.h"
#include "Cluster.h"
#include "MCParticle.h"
#include "Pixel.h"
#include "Track.h"

#include "core/utils/exceptions.h"

using namespace corryvreckan;

TestBeamObject::TestBeamObject() : m_detectorID(), m_timestamp(0) {}
TestBeamObject::TestBeamObject(std::string detectorID) : m_detectorID(detectorID), m_timestamp(0) {}
TestBeamObject::TestBeamObject(double timestamp) : m_detectorID(), m_timestamp(timestamp) {}
TestBeamObject::TestBeamObject(std::string detectorID, double timestamp)
    : m_detectorID(detectorID), m_timestamp(timestamp) {}
TestBeamObject::~TestBeamObject() {}

// Return class type for fixed object types (that don't depend on detector type)
TestBeamObject* TestBeamObject::Factory(std::string objectType, TestBeamObject* object) {
    // Track class
    if(objectType == "tracks") {
        return (object == NULL) ? new Track() : new Track(*static_cast<Track*>(object));
    }

    return new TestBeamObject();
}

// Return class type for objects which change with detector type
TestBeamObject* TestBeamObject::Factory(std::string detectorType, std::string objectType, TestBeamObject* object) {
    // Timepix types both use generic classes
    if(detectorType == "Timepix1" || detectorType == "Timepix3") {
        if(objectType == "pixels")
            return (object == NULL) ? new Pixel() : new Pixel(*static_cast<Pixel*>(object));
        if(objectType == "clusters")
            return (object == NULL) ? new Cluster() : new Cluster(*static_cast<Cluster*>(object));
        if(objectType == "mcparticles")
            return (object == NULL) ? new MCParticle() : new MCParticle(*static_cast<MCParticle*>(object));
    }

    return new TestBeamObject();
}

ClassImp(TestBeamObject)
