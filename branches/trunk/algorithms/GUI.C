#include "GUI.h"
#include "TApplication.h"
#include "TPolyLine3D.h"
#include "TSystem.h"
#include "TROOT.h"

GUI::GUI(bool debugging)
: Algorithm("GUI"){
  debug = debugging;
  updateNumber = 1;
}

void startDisplay(void* gui){
  
  // Make the TApplicaitons to allow canvases to stay open (for some reason we need two instances for it to work...)
  TApplication* app1 = new TApplication("example",0, 0);
  TApplication* app2 = new TApplication("example",0, 0);
  
  // Create the new canvases
  double nDetectors = ((GUI*)gui)->nDetectors;
  ((GUI*)gui)->trackCanvas = new TCanvas("TrackCanvas","Track canvas");
//  ((GUI*)gui)->trackCanvas->Divide();
  ((GUI*)gui)->hitmapCanvas = new TCanvas("HitMapCanvas","Hit map canvas");
  ((GUI*)gui)->hitmapCanvas->Divide(ceil(nDetectors/2.),2);
  ((GUI*)gui)->globalHitmapCanvas = new TCanvas("GlobalHitmapCanvas","Global hit map canvas");
  ((GUI*)gui)->globalHitmapCanvas->Divide(ceil(nDetectors/2.),2);
  ((GUI*)gui)->residualsCanvas = new TCanvas("ResidualsCanvas","Residuals canvas");
  ((GUI*)gui)->residualsCanvas->Divide(ceil(nDetectors/2.),2);

  // Run the TApplication
  app2->Run(true);
  
}

void GUI::initialise(Parameters* par){
 
  // Make the local pointer to the global parameters
  parameters = par;
  
  // Check the number of devices
  nDetectors = parameters->nDetectors;
  
	// Make the thread which will run the display
  displayThread = new TThread("displayThread", startDisplay, (void*) this);
  displayThread->Run();
  sleep(2);

  // Loop over all detectors and load the histograms that we will use
  for(int det = 0; det<parameters->nDetectors; det++){
    string detectorID = parameters->detectors[det];
    string hitmapHisto = "/tbAnalysis/TestAlgorithm/hitmap_"+detectorID;
    hitmap[detectorID] = (TH2F*)gDirectory->Get(hitmapHisto.c_str());
    string residualHisto = "/tbAnalysis/BasicTracking/residualsX_"+detectorID;
    residuals[detectorID] = (TH1F*)gDirectory->Get(residualHisto.c_str());
    string globalHitmapHisto = "/tbAnalysis/TestAlgorithm/clusterPositionGlobal_"+detectorID;
    globalHitmap[detectorID] = (TH2F*)gDirectory->Get(globalHitmapHisto.c_str());
  }

  // Set event counter
  eventNumber = 0;
}

StatusCode GUI::run(Clipboard* clipboard){

  gSystem->ProcessEvents();
  
  //-----------------------------------------
  // Draw the objects on the tracking canvas
  //-----------------------------------------

  trackCanvas->cd();
  TH1F* trackChi2 = (TH1F*)gDirectory->Get("/tbAnalysis/BasicTracking/trackChi2");
  trackChi2->DrawCopy();

  // Update the canvas
  if(eventNumber%updateNumber == 0){
//    sleep(0.5);
    trackCanvas->Update();
  }
  
  //-----------------------------------------
  // Draw the objects on the hitmap canvas
  //-----------------------------------------
  
  // Update the canvas
  if(eventNumber == 0){
    for(int det = 0; det<parameters->nDetectors; det++){
      string detectorID = parameters->detectors[det];
    	hitmap[detectorID]->SetTitle(detectorID.c_str());
    	hitmapCanvas->cd(det+1);
      hitmap[detectorID]->Draw("colz");
    }
  }
  // Update the canvas
  if(eventNumber%updateNumber == 0) {
    hitmapCanvas->Paint();
    hitmapCanvas->Update();
  }

  //-----------------------------------------
  // Draw the objects on the globalHitmap canvas
  //-----------------------------------------
  
  // Update the canvas
  if(eventNumber == 0){
    for(int det = 0; det<parameters->nDetectors; det++){
      string detectorID = parameters->detectors[det];
      globalHitmap[detectorID]->SetTitle(detectorID.c_str());
      globalHitmapCanvas->cd(det+1);
      globalHitmap[detectorID]->Draw("colz");
    }
  }
  // Update the canvas
  if(eventNumber%updateNumber == 0) {
    globalHitmapCanvas->Paint();
    globalHitmapCanvas->Update();
  }

  //-----------------------------------------
  // Draw the objects on the residuals canvas
  //-----------------------------------------
  
  // Loop over all detectors and load the hitmap
  if(eventNumber == 0){
    for(int det = 0; det<parameters->nDetectors; det++){
      string detectorID = parameters->detectors[det];
      residuals[detectorID]->SetTitle(detectorID.c_str());
      residualsCanvas->cd(det+1);
      residuals[detectorID]->Draw();
    }
  }
  // Update the canvas
  if(eventNumber%updateNumber == 0){
    residualsCanvas->Paint();
    residualsCanvas->Update();
  }

  eventNumber++;
  return Success;
  
  // Old code to allow updating of a TBrowser (TBrowser in 5.34 not thread safe, so removed)
  //  ((TCanvas*)gROOT->GetListOfCanvases()->At(0))->Paint();
  //  ((TCanvas*)gROOT->GetListOfCanvases()->At(0))->Update();
  //  gSystem->ProcessEvents();

}
  

void GUI::finalise(){

  // Kill the display thread
//  displayThread->Kill();

}

