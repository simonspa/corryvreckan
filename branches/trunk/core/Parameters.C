// local
#include "Parameters.h"
#include <fstream>
#include <sstream>
#include <unistd.h>

Parameters::Parameters(){
  
  histogramFile = "output.root";
  inputFile = "G02.dat";
  conditionsFile = "cond.dat";
	inputDirectory = "";
  nEvents = 0;
  align = false;
  produceMask = false;
}

void Parameters::help()
{
  cout << "********************************************************************" << endl;
  cout << "Typical 'tbAnalysis' executions are:" << endl;
  cout << " ~> bin/tbAnalysis -d directory" << endl;
  cout << endl;
}


// Read command line options and set appropriate variables
void Parameters::readCommandLineOptions(int argc, char *argv[]){
  
  cout<<endl;
  cout<<"===================| Reading Parameters  |===================="<<endl<<endl;
  int option; char temp[256];
  while ( (option = getopt(argc, argv, "amd:c:n:h:")) != -1) {
    switch (option) {
      case 'a':
        align = true;
        cout<<"Alignment flagged to be run"<<endl;
        break;
      case 'm':
        produceMask = true;
        cout<<"Will update masked pixel files"<<endl;
        break;
      case 'd':
        sscanf( optarg, "%s", &temp);
        inputDirectory = (string)temp;
        cout<<"Taking data from: "<<inputDirectory<<endl;
        break;
      case 'c':
        sscanf( optarg, "%s", &temp);
        conditionsFile = (string)temp;
        cout<<"Picking up conditions file: "<<conditionsFile<<endl;
        break;
      case 'h':
        sscanf( optarg, "%s", &temp);
        histogramFile = (string)temp;
        cout<<"Writing histograms to: "<<histogramFile<<endl;
        break;
      case 'n':
        sscanf( optarg, "%d", &nEvents);
        cout<<"Running over "<<nEvents<<" events"<<endl;
        break;
    }
  }
  cout<<endl;
}

bool Parameters::writeConditions(){
  
  // Open the conditions file to write detector information
  ofstream conditions;
  conditions.open("alignmentOutput.dat");

  // Write the file header
  conditions << std::left << setw(12) << "DetectorID" << setw(14) << "DetectorType" << setw(10) << "nPixelsX" << setw(10) << "nPixelsY" << setw(8) << "PitchX" << setw(8) << "PitchY" << setw(11) << "X" << setw(11) << "Y" << setw(11) << "Z" << setw(11) << "Rx" << setw(11) << "Ry" << setw(11) << "Rz" << setw(14) << "tOffset"<<endl;
  
  // Loop over all detectors
  for(int det = 0; det<this->nDetectors; det++){
    string detectorID = this->detectors[det];
    DetectorParameters* detectorParameters = this->detector[detectorID];
    // Write information to file
    conditions << std::left << setw(12) << detectorID << setw(14) << detectorParameters->type() << setw(10) << detectorParameters->nPixelsX() << setw(10) << detectorParameters->nPixelsY() << setw(8) << 1000.*detectorParameters->pitchX() << setw(8) << 1000.*detectorParameters->pitchY() << setw(11) << std::fixed << detectorParameters->displacementX() << setw(11) << detectorParameters->displacementY() << setw(11) << detectorParameters->displacementZ() << setw(11) << detectorParameters->rotationX() << setw(11) << detectorParameters->rotationY() << setw(11) << detectorParameters->rotationZ() << setw(14) << std::setprecision(10) << detectorParameters->timingOffset() << resetiosflags( ios::fixed ) << std::setprecision(6) << endl;

  }
  
  // Close the file
  conditions.close();
  
}


bool Parameters::readConditions(){
 
  // Open the conditions file to read detector information
  ifstream conditions;
  conditions.open(conditionsFile.c_str());
  string line;
  
  cout<<"----------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
  // Loop over file
  while(getline(conditions,line)){
    
    // Ignore header
    if( line.find("DetectorID") != string::npos ){
      cout<<"Device parameters: "<<line<<endl;
      continue;
    }
    
    // Make default values for detector parameters
    string detectorID(""), detectorType(""); int nPixelsX(0), nPixelsY(0); double pitchX(0), pitchY(0), x(0), y(0), z(0), Rx(0), Ry(0), Rz(0);
    double timingOffset(0.);
    
    // Grab the line and parse the data into the relevant variables
    istringstream detectorInfo(line);
    detectorInfo >> detectorID >> detectorType >> nPixelsX >> nPixelsY >> pitchX >> pitchY >> x >> y >> z >> Rx >> Ry >> Rz >> timingOffset;
    if(detectorID == "") continue;

    // Save the detector parameters in memory
    DetectorParameters* detectorSummary = new DetectorParameters(detectorType,nPixelsX,nPixelsY,pitchX,pitchY,x,y,z,Rx,Ry,Rz,timingOffset);
    detector[detectorID] = detectorSummary;
    
    cout<<"Device parameters: "<<line<<endl;
    
  }
  cout<<"----------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
  
  // Now check that all devices which are registered have parameters as well
  bool unregisteredDetector=false;
  // Loop over all registered detectors
  for(int det=0;det<nDetectors;det++){
    if(detector.count(detectors[det]) == 0){
      cout<<"Detector "<<detectors[det]<<" has no conditions loaded"<<endl;
      unregisteredDetector = true;
    }
  }
  if(unregisteredDetector) return false;
  return true;
  
}


