// Global includes
#include <iostream>
#include <string>
#include <signal.h>

// ROOT includes
#include "TROOT.h"
#include "TApplication.h"

// Local includes
#include "Parameters.h"
#include "Analysis.h"

// Algorithm list
#include "EventLoader.h"
#include "TestAlgorithm.h"
#include "Timepix3EventLoader.h"
#include "Timepix3Clustering.h"
#include "Timepix3MaskCreator.h"
#include "BasicTracking.h"
#include "Alignment.h"
#include "EventDisplay.h"

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
  
  // =========================================================================
  // Steering file begins
  // =========================================================================

  // Global debug flag
  bool debug = false;
  
  // General parameters
  parameters->reference = "W0013_G03";
  
  // Algorithm list
  Timepix3EventLoader*	tpix3EventLoader	= new Timepix3EventLoader(debug);
  Timepix3Clustering*		tpix3Clustering		= new Timepix3Clustering(debug);
  TestAlgorithm* 				testAlgorithm			= new TestAlgorithm(debug);
  Timepix3MaskCreator*	tpix3MaskCreator	= new Timepix3MaskCreator(debug);
  BasicTracking* 				basicTracking			= new BasicTracking(debug);
  Alignment*	 					alignment					= new Alignment(debug);
  EventDisplay*	 				eventDisplay			= new EventDisplay(debug);
  
  // =========================================================================
  // Steering file ends
  // =========================================================================
  
  // Overwrite steering file values from command line
  parameters->readCommandLineOptions(argc,argv);

  // Load alignment parameters
  if(!parameters->readConditions()) return 0;
  
  // Initialise the analysis object and add algorithms to run
  analysis = new Analysis(parameters);
  analysis->add(tpix3EventLoader);
  analysis->add(tpix3Clustering);
  analysis->add(testAlgorithm);
  analysis->add(basicTracking);
  
  if(parameters->align) analysis->add(alignment);
  if(parameters->produceMask) analysis->add(tpix3MaskCreator);
  analysis->run();

  return 0;

}
