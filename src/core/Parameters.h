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

using namespace std;
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
    class Parameters {

    public:
        // Constructors and destructors
        Parameters();
        virtual ~Parameters(){};

        // Functions
        bool readConditions();
        bool writeConditions();
        void readCommandLineOptions(int, char**);
        void readDutMask();
        void maskDutColumn(int);
        void maskDutRow(int);
        void registerDetector(string detectorID) {
            nDetectors++;
            detectors.push_back(detectorID);
        }

        // Member variables
        string conditionsFile;
        string inputTupleFile;
        string inputDirectory;
        string outputTupleFile;
        string histogramFile;
        string dutMaskFile;
        vector<string> detectors;
        int nDetectors;
        string reference;
        string dut;
        double currentTime;
        double eventLength;
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
        map<string, DetectorParameters*> detector;
    };
}
#endif // PARAMETERS_H
