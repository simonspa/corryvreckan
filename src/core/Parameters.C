// local
#include <fstream>
#include <sstream>
#include <unistd.h>
#include "Parameters.h"

#include "utils/log.h"

using namespace corryvreckan;
using namespace std;

Parameters::Parameters() {

    histogramFile = "outputHistograms.root";
    conditionsFile = "cond.dat";
    dutMaskFile = "defaultMask.dat";
    inputDirectory = "";
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
        DetectorParameters* detectorParameters = this->detector[detectorID];
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

bool Parameters::readConditions() {

    // Open the conditions file to read detector information
    ifstream conditions;
    conditions.open(conditionsFile.c_str());
    string line;

    LOG(INFO) << "-------------------------------------------- Reading conditions "
                 "---------------------------------------------------";
    // Loop over file
    while(getline(conditions, line)) {

        // Ignore header
        if(line.find("DetectorID") != string::npos) {
            LOG(INFO) << "Device parameters: " << line;
            continue;
        }

        // Make default values for detector parameters
        string detectorID(""), detectorType("");
        int nPixelsX(0), nPixelsY(0);
        double pitchX(0), pitchY(0), x(0), y(0), z(0), Rx(0), Ry(0), Rz(0);
        double timingOffset(0.);

        // Grab the line and parse the data into the relevant variables
        istringstream detectorInfo(line);
        detectorInfo >> detectorID >> detectorType >> nPixelsX >> nPixelsY >> pitchX >> pitchY >> x >> y >> z >> Rx >> Ry >>
            Rz >> timingOffset;
        if(detectorID == "")
            continue;

        // Save the detector parameters in memory
        DetectorParameters* detectorSummary =
            new DetectorParameters(detectorType, nPixelsX, nPixelsY, pitchX, pitchY, x, y, z, Rx, Ry, Rz, timingOffset);
        detector[detectorID] = detectorSummary;

        // Temp: register detector
        registerDetector(detectorID);

        LOG(INFO) << "Device parameters: " << line;
    }
    LOG(INFO) << "---------------------------------------------------------------"
                 "----------------------------------------------------";

    // Now check that all devices which are registered have parameters as well
    bool unregisteredDetector = false;
    // Loop over all registered detectors
    for(auto& det : detectors) {
        if(detector.count(det) == 0) {
            // LOG(INFO) << "Detector " << detectors[det] << " has no conditions loaded";
            unregisteredDetector = true;
        }
    }
    if(unregisteredDetector)
        return false;

    // Finally, sort the list of detectors by z position (from lowest to highest)
    // FIXME reimplement
    //
    return true;
}

void Parameters::readDutMask() {
    // If no masked file set, do nothing
    if(dutMaskFile == "defaultMask.dat")
        return;
    detector[this->DUT]->setMaskFile(dutMaskFile);
    // Open the file with masked pixels
    LOG(INFO) << "Reading DUT mask file from " << dutMaskFile;
    std::fstream inputMaskFile(dutMaskFile.c_str(), std::ios::in);
    int row, col;
    std::string id;
    std::string line;
    // loop over all lines and apply masks
    while(getline(inputMaskFile, line)) {
        inputMaskFile >> id >> row >> col;
        if(id == "c")
            maskDutColumn(col); // Flag to mask a column
        if(id == "r")
            maskDutRow(row); // Flag to mask a row
        if(id == "p")
            detector[this->DUT]->maskChannel(col, row); // Flag to mask a pixel
    }
    return;
}

// The masking of pixels on the dut uses a map with unique
// id for each pixel given by column + row*numberColumns
void Parameters::maskDutColumn(int column) {
    int nRows = detector[this->DUT]->nPixelsY();
    for(int row = 0; row < nRows; row++)
        detector[this->DUT]->maskChannel(column, row);
}
void Parameters::maskDutRow(int row) {
    int nColumns = detector[this->DUT]->nPixelsX();
    for(int column = 0; column < nColumns; column++)
        detector[this->DUT]->maskChannel(column, row);
}
