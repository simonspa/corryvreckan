#include <iostream>
#include <string>

#include "TROOT.h"
#include "TApplication.h"

#include "Parameters.h"
#include "Amalgamation.h"
#include "SummaryDisplayer.h"
#include "EventDisplay.h"
#include "Analysis.h"
#include "ClusterMaker.h"
#include "NewVetraClusterMaker.h"
#include "SciFiClusterMaker.h"
#include "TrackTimestamper.h"
#include "FEI4ClusterMaker.h"
#include "RawHitMapMaker.h"
#include "NClustersMonitor.h"
#include "EfficiencyCalculator.h"
#include "PulseShapeAnalyser.h"
#include "PatternRecognition.h"
#include "TrackFitter.h"
#include "ResidualPlotter.h"
#include "EtaDistribution.h"
#include "Alignment.h"
#include "SystemOfUnits.h"
#include "PhysicalConstants.h"
#include "ClicpixAnalysis.h"

using namespace SystemOfUnits;
using namespace PhysicalConstants;
using namespace boost;

// Useful converter for output histogram names based on variable.
template <typename T>
std::string convertToString(T number) {
  std::ostringstream ss;
  ss << number;
  return ss.str();
}

//// The masking of pixels on the dut uses a map with unique
//// id for each pixel given by column + row*numberColumns
//void maskColumn(Parameters* par, int column){
//	int nColumns = par->nPixelsX[par->dut];
//	int nRows = par->nPixelsY[par->dut];
//	for(int row=0;row<nRows;row++) par->maskedPixelsDUT[column + row*nColumns] = 1.;
//}
//void maskRow(Parameters* par, int row){
//	int nColumns = par->nPixelsX[par->dut];
//	int nRows = par->nPixelsY[par->dut];
//	for(int column=0;column<nColumns;column++) par->maskedPixelsDUT[column + row*nColumns] = 1.;
//}
//void maskPixel(Parameters* par, int row, int column){
//	int nColumns = par->nPixelsX[par->dut];
//	par->maskedPixelsDUT[column + row*nColumns] = 1.;
//}

//-------------------------------------------------------------------------------
// Main algorithm defining which algorithms run and important run configurations
//-------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  bool display = false;
  TApplication app("app", 0, 0);
  gROOT->SetStyle("Plain");

  Parameters* par = new Parameters();
  // Check the command line arguments whether to run amalgamation or analysis.
  if (!par->checkCommandLine(argc, argv)) {
    par->help();
    return 1;
  }

  if (par->makeEventFile) {
    par->readCommandLine(argc, argv);
    Amalgamation amg(par);
    if (par->verbose) {
      SummaryDisplayer smd(par);
      std::cout << "'Quit ROOT' from File menu or just ^c" << std::endl;
      app.Run();
    }
    return 0; 
  }

  // =========================================================================  
  // Below one should set all parameters required. 
  // If unsure ask, bad setting == bad results!
  // =========================================================================  
  // General Information
  // =========================================================================
  // Name of the DuT
  par->dut = "CLi-CPix";
  // Name of the plane to align (for alignment methods >= 2)
  par->devicetoalign = "CLi-CPix";
  // Name of the plane defining the global frame 
  par->referenceplane = "Mim-osa02";
  // Telescope plane(s) in ToA mode
  // par->toa["I10-W0108"] = true;
  // Medipix3 planes
  // par->medipix3["K4-W4"] = true;
  // Planes to be masked (no clustering, tracking etc.)
//	par->masked["Mim-osa00"] = true;
//	par->masked["Mim-osa01"] = true;
//	par->masked["Mim-osa02"] = true;
//  par->masked["Mim-osa03"] = true;
//  par->masked["Mim-osa04"] = true;
//  par->masked["Mim-osa05"] = true;
  // Name of the output histogram file
  // par->histogramFile = "histograms" + convertToString(par->adcCutLow) + ".root";
  // Set the pixel pitch and number of pixels.
  par->pixelPitchX["default"] = 0.0184;
  par->pixelPitchY["default"] = 0.0184;
  par->nPixelsX["default"] = 1152;
  par->nPixelsY["default"] = 576;
  // Clicpix information
  par->pixelPitchX["CLi-CPix"] = 0.025;
  par->pixelPitchY["CLi-CPix"] = 0.025;
  par->nPixelsX["CLi-CPix"] = 64;
  par->nPixelsY["CLi-CPix"] = 64;
	par->pixelPitchX["Cen-tral"] = 0.025;
	par->pixelPitchY["Cen-tral"] = 0.025;
	par->nPixelsX["Cen-tral"] = 64;
	par->nPixelsY["Cen-tral"] = 64;

	// The following four lines are the recommendation for 2011 scifi data
  // par->dut = "SciFi";
  // par->devicetoalign = "SciFi";
  // par->toa["D04-W0015"] = true;
  // par->referenceplane = "I06-W0092";
       
  // =========================================================================  
  // Clustering
  // =========================================================================
  // Min./max. ADC value of a cluster
  par->adcCutLow["default"] = -1.;
  par->adcCutHigh["default"] = 10000.;
  // Max. cluster size
  par->clusterSizeCut = 140;
  // Eta correction 
  // (100: no eta correction)
  par->etacorrection = 100;
  // ToT calibration 
  // (0: no correction, 1: average surrogate function, 2: per pixel correction)
  par->pixelcalibration = 0; 
  
  // =========================================================================  
  // Pattern Recognition
  // =========================================================================
  // Minimum number of hits required for a track
  par->numclustersontrack = 6;
  // Flag to include (or not) the DuT in the pattern recognition
  par->dutinpatternrecognition = false;
  // Further planes to be excluded from pattern recognition
//  par->excludedFromPatternRecognition["Mim-osa00"] = true;
//  par->excludedFromPatternRecognition["Mim-osa01"] = true;
//  par->excludedFromPatternRecognition["Mim-osa02"] = true;
//  par->excludedFromPatternRecognition["Mim-osa03"] = true;
//  par->excludedFromPatternRecognition["Mim-osa04"] = true;
//  par->excludedFromPatternRecognition["Mim-osa05"] = true;
  // Flag to use (or not) kNN tracking algorithm (default is true)
  par->useFastTracking = true;
  // Acceptance window (in mm)                
  par->trackwindow = 0.11;
  // Extend (or not) the search window using the Highland formula
  par->molierewindow = false;
  // Angular range for search window (X * the moliere angle)
  par->molieresigmacut = 3.0;
  // Momentum of the beam (used in Highland formula)      
  par->momentum = 10. * GeV / c_light;
  // Radiation length fraction per plane (used in Highland formula)
  par->xOverX0 = 0.05 * perCent;
  // Radiation length fraction for DuT (not in use yet)
  // par->xOverX0_dut = 2.65 * perCent;
  // Particle type (used in Highland formula)
  // Default is pi (can choose e, mu, or p)
  par->particle = "p";
	
	// Restrict the reconstruction to a given window on the first plane (hard-coded at present)
	par->restrictedReconstruction = true;

  // Flag to allow (or not) clusters to be used more than once
  // (default is false - as yet no class to remove ghosts)
  par->clusterSharingFastTracking = false; 
  // Working - but as yet not as efficient.. TBC
  par->extrapolationFastTracking = false;
  // Flag to use Cellular Automaton. EXPERIMENTAL, do not use for analysis!!
  par->useCellularAutomata = false;
	
  // Flag to make tracks separately in each arm
	par->trackingPerArm = false;
	// You can analyse these separately or join them to make tracks through the whole setup
	par->joinTracksPerArm = false;
	// Alignment method 0 will align upstream tracks by default. To align the downstream arm use:
	par->alignDownstream = false;
	// By default (and for alignment) the dut in this mode is compared with upstream tracks. To use a
	// joint estimate of the intercept, from both arms, use:
	par->jointIntercept = false;
	
	// Specify which arms are upstream and downstream
//	par->upstreamPlane["Mim-osa00"] = true;
//	par->upstreamPlane["Mim-osa01"] = true;
//	par->upstreamPlane["Mim-osa02"] = true;
//	par->downstreamPlane["Mim-osa03"] = true;
//	par->downstreamPlane["Mim-osa04"] = true;
//	par->downstreamPlane["Mim-osa05"] = true;

  // =========================================================================  
  // Track Fit
  // =========================================================================
  // Flag to use or not Minuit (default is false)
  // Minuit takes a little longer, does give access to fit matrix
  par->useMinuitFit = false;
  // Planes to be excluded from track fit          
//  par->excludedFromTrackFit["Mim-osa04"] = true;
  // Error associated to cluster position for a specific plane
//  par->trackFitError["Mim-osa00"] = 0.004;
//  par->trackFitError["Mim-osa01"] = 0.004;
//  par->trackFitError["Mim-osa02"] = 0.004;
//  par->trackFitError["Mim-osa03"] = 0.004;
//  par->trackFitError["Mim-osa04"] = 0.004;
//  par->trackFitError["Mim-osa05"] = 0.004;
	// Plane at the centre/dut for downstream alignment
//	par->trackFitError["Cen-tral"] = 0.000001;
	// Clicpix DUT
  par->trackFitError["CLi-CPix"] = 0.008;
  // Remove tracks with a chi2/ndof greater than X.
  par->trackchi2NDOF = 1.e+5 ;
  // Remove tracks with a probability less than X%.
  // 5% removes rubbish but is very loose.
  par->trackprob = 0 * perCent;
	// Chi2 cut
	par->trackchi2 = 15.;
	
  // =========================================================================  
  // Track Timestamper
  // =========================================================================
  //   Settings required for the matching of tracks to TDC triggers. 
  //   Generally we run at 25ns and a TDC delay of -200
  //   However, there will be runs that differ so check.
  // Clock period (in ns)
  par->clock = 25.0;
  // TDC delay (in ns)
  par->tdcOffset = -200.;
  // the following two lines are the recommendation for 2011 scifi data
  // par->clock = 3333.0;  
  // par->tdcOffset = -2700.; 
  // the following two lines are the recommendation for medipix data
  // par->clock = 25.0;  // clock period (in ns)
  // par->tdcOffset = 0.; // TDC delay (in ns)

  // =========================================================================  
  // Efficiency Calculator
  // =========================================================================
  // Set parameters for calculating percentages of clusters 
  // used inside fiducial region and scintillator overlap.
  // Number of triggers per frame set on the NIM crate
  par->expectedTracksPerFrame = 20;
  // New & testing gives better efficiencies. Extends a quadrilateral instead of a rectangle about the hits.
  par->polyCorners = true;
  // SLOW!! Statistical test distribution of non-associated clusters. Kolmogorov-Smirnov (the prob between two planes)
  par->useKS = false;                 

  // =========================================================================  
  // Other
  // =========================================================================    
  par->residualmaxx = 0.3;
  par->residualmaxy = 0.3;

  // =========================================================================

  // Get the command line parameters (these override the above settings).      
  par->readCommandLine(argc, argv);
  // Load the alignment conditions from file.
  par->readConditions();
	// Load the masked pixel list (for the DUT) from file.
	par->readMasked();
	// If the track reconstruction is restricted, get the track window
	int check;
	if(par->restrictedReconstruction) check = par->readTrackWindow();
	if(check == 1) return 1;

	// Hack for the moment - eta correction used to pass the run number. Allows the masked pixels to be loaded
	// Eta correction is then reset to 100

	// Can define which regions will be masked in the analysis
	// using maskPixel(parameters,row,column), maskColumn(parameters, column),
	// maskRow(parameters, row) or any combination thereof
	
	// Get masked pixel file for this run, and mask all pixels
//	if(par->etacorrection != 100){
//		string maskfile = "";
//		ifstream inputMaskFile(maskfile.c_str());
//		int row,col; string id;
//		std::string line;
//		while(getline(inputMaskFile,line)){
//			inputMaskFile >> id >> row >> col;
//			if(id == "c") maskColumn(par,col); // Flag to mask a column
//			if(id == "p") maskPixel(par,row,col); // Flag to mask a pixel
//		}
//		par->etacorrection = 100;
//	}
	
	
  // =========================================================================  
  // Analysis algorithms - Running the code
  // =========================================================================
  Analysis analysis(par);
  // =========================================================================
  // Here we initialise all the algorithms that will be run. 
  // Each is added too the analysis tool which executes them in turn. 
  // Sometimes one may not want to run all of them, this is CPU intensive.
  // Simply comment out those that are not required.
  //....
  RawHitMapMaker hmm(par,display);
  // analysis.add(&hmm);
  //....
  ClusterMaker clm(par,display);
  analysis.add(&clm);
  //....
  NClustersMonitor ncm(par,display);
  // analysis.add(&ncm);
  //....
  PatternRecognition mtm(par,display);
  analysis.add(&mtm);
  //....
  TrackFitter ttf(par,display);
  analysis.add(&ttf);
  //....
  PulseShapeAnalyser psa(par,display);
  // analysis.add(&psa);
  //....
  EtaDistribution edt(par,display);
  // analysis.add(&edt);
  //....
  TrackTimestamper tts(par, display);
  //analysis.add(&tts);
  //....
  SciFiClusterMaker scm(par,display);
  // analysis.add(&scm);
  //....
  EfficiencyCalculator efc(par,display);
  // analysis.add(&efc);
  //....
  ResidualPlotter rp(par,display);
  analysis.add(&rp);
  //....
	ClicpixAnalysis cpa(par,display);
	analysis.add(&cpa);
	//....
  NewVetraClusterMaker nvcm(par,display);
  // analysis.add(&nvcm);
  //....
  FEI4ClusterMaker fcm(par,display);
  // analysis.add(&fcm);
  //....
	Alignment ali(par,display);
  if (par->align) analysis.add(&ali);
  //....
  EventDisplay evd(par,&app);
  if (par->eventDisplay) analysis.add(&evd);
  // =========================================================================
  analysis.run();
  // if(par->verbose && not par->eventDisplay) SummaryDisplayer smd(par);
  // std::cout<<"'Quit ROOT' from File menu or just ^c"<<std::endl;
//  app.Run();
  return 0;

}
