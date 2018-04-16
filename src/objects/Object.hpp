#ifndef CORRYVRECKANOBJECT_H
#define CORRYVRECKANOBJECT_H 1

// Include files
#include <string>
#include <vector>
#include "TObject.h"
#include "TTree.h"

//-------------------------------------------------------------------------------
// Generic base class. Every class which inherits from Object can be
// placed on the clipboard and written out to file.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    class Object : public TObject {

    public:
        // Constructors and destructors
        Object();
        Object(std::string detectorID);
        Object(double timestamp);
        Object(std::string detectorID, double timestamp);
        virtual ~Object();

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
        static Object* Factory(std::string, Object* object = NULL);
        static Object* Factory(std::string, std::string, Object* object = NULL);

    protected:
        // Member variables
        std::string m_detectorID;
        double m_timestamp;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Object, 1)
    };

    // Vector type declaration
    typedef std::vector<Object*> Objects;
} // namespace corryvreckan

#endif // CORRYVRECKANOBJECT_H
