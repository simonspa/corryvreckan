// local
#include "Parameters.h"
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "utils/log.h"

using namespace corryvreckan;
using namespace std;

Parameters::Parameters() {

    histogramFile = "outputHistograms.root";
    currentTime = 0.; // seconds
}

bool Parameters::writeConditions() {

    // Open the conditions file to write detector information
    ofstream conditions;
    conditions.open("alignmentOutput.dat");

    // Write the file header
    conditions << std::left << setw(12) << "DetectorID" << setw(14) << "DetectorType" << setw(10) << "nPixelsX" << setw(10)
               << "nPixelsY" << setw(8) << "PitchX" << setw(8) << "PitchY" << setw(11) << "X" << setw(11) << "Y" << setw(11)
               << "Z" << setw(11) << "Rx" << setw(11) << "Ry" << setw(11) << "Rz" << setw(14) << "tOffset" << endl;

    // Loop over all detectors
    for(auto& detectorID : this->detectors) {
        Detector* detectorParameters = this->detector[detectorID];
        // Write information to file
        conditions << std::left << setw(12) << detectorID << setw(14) << detectorParameters->type() << setw(10)
                   << detectorParameters->nPixelsX() << setw(10) << detectorParameters->nPixelsY() << setw(8)
                   << 1000. * detectorParameters->pitchX() << setw(8) << 1000. * detectorParameters->pitchY() << setw(11)
                   << std::fixed << detectorParameters->displacementX() << setw(11) << detectorParameters->displacementY()
                   << setw(11) << detectorParameters->displacementZ() << setw(11) << detectorParameters->rotationX()
                   << setw(11) << detectorParameters->rotationY() << setw(11) << detectorParameters->rotationZ() << setw(14)
                   << std::setprecision(10) << detectorParameters->timingOffset() << resetiosflags(ios::fixed)
                   << std::setprecision(6) << endl;
    }

    // Close the file
    conditions.close();
    return true;
}
