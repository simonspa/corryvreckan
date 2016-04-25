// Global includes
#include <iostream>
#include <string>
#include <signal.h>

// ROOT includes
#include "TROOT.h"
#include "TApplication.h"
//#include "TPad.h"

// Local includes
#include "Parameters.h"
#include "Analysis.h"

// Algorithm list
#include "TestAlgorithm.h"
#include "Timepix3EventLoader.h"
#include "Timepix3Clustering.h"
#include "Timepix1Clustering.h"
#include "Timepix3MaskCreator.h"
#include "BasicTracking.h"
#include "SpatialTracking.h"
#include "Alignment.h"
#include "EventDisplay.h"
#include "GUI.h"
#include "DUTAnalysis.h"
#include "FileWriter.h"
#include "FileReader.h"
#include "Timepix1Correlator.h"
#include "ClicpixAnalysis.h"

//-------------------------------------------------------------------------------
// The Steering is effectively the executable. It reads command line
// parameters and initialises all other parameters, then runs each algorithm
// added. Algorithms have 3 steps: initialise, run and finalise.
//-------------------------------------------------------------------------------

// The analysis object to be used
Analysis* analysis;

// Handle user interruption
// This allows you to ^C at any point in a controlled way
void userException(int sig){
  cout<<endl<<"User interrupted"<<endl;
  analysis->finaliseAll();
  exit(1);
}

int main(int argc, char *argv[]) {
 
  // Register escape behaviour
  signal(SIGINT, userException);

  // New parameters object
  Parameters* parameters = new Parameters();
  
  // Global debug flag
  bool debug = false;

  // Algorithm list
  Timepix3EventLoader*	tpix3EventLoader	= new Timepix3EventLoader(debug);
  Timepix3Clustering*		tpix3Clustering		= new Timepix3Clustering(debug);
  Timepix1Clustering*		tpix1Clustering		= new Timepix1Clustering(debug);
  TestAlgorithm* 				testAlgorithm			= new TestAlgorithm(debug);
  Timepix3MaskCreator*	tpix3MaskCreator	= new Timepix3MaskCreator(debug);
  BasicTracking* 				basicTracking			= new BasicTracking(debug);
  SpatialTracking* 			spatialTracking		= new SpatialTracking(debug);
  Alignment*	 					alignment					= new Alignment(debug);
  EventDisplay*	 				eventDisplay			= new EventDisplay(debug);
  GUI*	 								gui								= new GUI(debug);
  DUTAnalysis*	 				dutAnalysis				= new DUTAnalysis(debug);
  FileWriter*	 					fileWriter				= new FileWriter(debug);
  FileReader*	 					fileReader				= new FileReader(debug);
  Timepix1Correlator*	 	correlator				= new Timepix1Correlator(debug);
  ClicpixAnalysis*	 		clicpixAnalysis		= new ClicpixAnalysis(debug);

  // =========================================================================
  // Steering file begins
  // =========================================================================
  
  // General parameters
  parameters->reference = "W0013_G03";
  parameters->DUT = "W0005_E02";
//  parameters->reference = "W0013_G03";
//  parameters->DUT = "W0019_L08";
//  parameters->DUT = "W0019_F07";

  parameters->detectorToAlign = parameters->DUT;
  parameters->excludedFromTracking[parameters->DUT] = true;
  
  parameters->excludedFromTracking["W0005_E02"] = true;
  parameters->excludedFromTracking["W0005_F01"] = true;
  parameters->excludedFromTracking["W0019_C07"] = true;
  parameters->excludedFromTracking["W0019_G07"] = true;
  parameters->excludedFromTracking["W0019_F07"] = true;
  parameters->excludedFromTracking["W0019_L08"] = true;
  parameters->excludedFromTracking["W0005_H03"] = true;

  parameters->excludedFromTracking["W0013_F09"] = true;
  //tpix3EventLoader->debug = true;
  //testAlgorithm->makeCorrelations = true;  
  // =========================================================================
  // Steering file ends
  // =========================================================================
  
  // Overwrite steering file values from command line
  parameters->readCommandLineOptions(argc,argv);

  // Load alignment parameters
  if(!parameters->readConditions()) return 0;
  
  // Load mask file for the dut (if specified)
  parameters->readDutMask();
  
  // Initialise the analysis object and add algorithms to run
  analysis = new Analysis(parameters);
//  analysis->add(tpix1EventLoader);
//  analysis->add(fileReader);
//  analysis->add(tpix1Clustering);
//  analysis->add(spatialTracking);
//  analysis->add(correlator);
  analysis->add(tpix3EventLoader);
  analysis->add(tpix3Clustering);
  analysis->add(testAlgorithm);
  analysis->add(basicTracking);
//  analysis->add(dutAnalysis);
//  analysis->add(clicpixAnalysis);
//  analysis->add(fileWriter);
  
  if(parameters->align) analysis->add(alignment);
  if(parameters->produceMask) analysis->add(tpix3MaskCreator);
  if(parameters->eventDisplay) analysis->add(eventDisplay);
  if(parameters->gui) analysis->add(gui);
  
  // Run the analysis
  analysis->run();

  return 0;

}
