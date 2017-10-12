#ifndef PARAMETERS_H
#define PARAMETERS_H 1

// Include files
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
// Root includes
#include "Math/Point3D.h"
#include "Math/Rotation3D.h"
#include "Math/RotationZYX.h"
#include "Math/Transform3D.h"
#include "Math/Translation3D.h"
#include "Math/Vector3D.h"
// Local includes
#include "objects/Track.h"

#include "DetectorParameters.h"

using namespace ROOT::Math;

//-------------------------------------------------------------------------------
// Two classes: DetectorParameters and Parameters. The conditions file read at
// the beginning of each run contains a set of information (like pitch, number
// of
// channels, etc.) that is held for each detector in its own DetectorParameters
// object. These are retrieved through the global Parameters class, which gives
// access to variables/information at any point through the event processing.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    bool sortByZ(std::string detector1, std::string detector2);

    class Parameters {

    public:
        // Constructors and destructors
        Parameters();
        virtual ~Parameters(){};

        // Functions
        bool writeConditions();
        void registerDetector(std::string detectorID) {
            nDetectors++;
            detectors.push_back(detectorID);
        }

        // Member variables
        std::vector<std::string> detectors;
        int nDetectors{0};
        double currentTime;
        std::map<std::string, bool> excludedFromTracking;
        std::map<std::string, bool> masked;
        std::string detectorToAlign;

        // Parameters for each detector (stored by detector ID)
        std::map<std::string, Detector*> detector;
    };
}
#endif // PARAMETERS_H
