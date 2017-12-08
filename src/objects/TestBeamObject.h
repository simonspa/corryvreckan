#ifndef TESTBEAMOBJECT_H
#define TESTBEAMOBJECT_H 1

// Include files
#include <string>
#include <vector>
#include "TObject.h"
#include "TTree.h"

//-------------------------------------------------------------------------------
// Generic base class. Every class which inherits from TestBeamObject can be
// placed on the clipboard and written out to file.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    class TestBeamObject : public TObject {

    public:
        // Constructors and destructors
        TestBeamObject() : m_detectorID(), m_timestamp(0) {}
        TestBeamObject(std::string detectorID) : m_detectorID(detectorID), m_timestamp(0) {}
        TestBeamObject(double timestamp) : m_detectorID(), m_timestamp(timestamp) {}
        TestBeamObject(std::string detectorID, double timestamp) : m_detectorID(detectorID), m_timestamp(timestamp) {}
        virtual ~TestBeamObject() { m_timestamp = 0; }

        // Methods to get member variables
        std::string getDetectorID() { return m_detectorID; }
        std::string detectorID() { return getDetectorID(); }

        double timestamp() { return m_timestamp; }
        void timestamp(double time) { m_timestamp = time; }
        void setTimestamp(double time) { timestamp(time); }

        // Methods to set member variables
        void setDetectorID(std::string detectorID) { m_detectorID = detectorID; }

        // Function to get instantiation of inherited class (given a string, give back
        // an object of type 'daughter')
        static TestBeamObject* Factory(std::string, TestBeamObject* object = NULL);
        static TestBeamObject* Factory(std::string, std::string, TestBeamObject* object = NULL);

    protected:
        // Member variables
        std::string m_detectorID;
        double m_timestamp;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(TestBeamObject, 2)
    };

    // Vector type declaration
    typedef std::vector<TestBeamObject*> TestBeamObjects;
} // namespace corryvreckan
#endif // TESTBEAMOBJECT_H
