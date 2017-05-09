#include "GUI.h"
#include "TApplication.h"
#include "TPolyLine3D.h"
#include "TSystem.h"
#include "TROOT.h"

GUI::GUI(bool debugging)
: Algorithm("GUI"){
  debug = debugging;
  updateNumber = 500;
}

void GUI::initialise(Parameters* par){
 
  // Make the local pointer to the global parameters
  parameters = par;
  
  // Check the number of devices
  nDetectors = parameters->nDetectors;
  
  // TApplication keeps the canvases persistent
  app = new TApplication("example",0, 0);
    
  //========= Add each canvas that is wanted =========//
  
  TCanvas* trackCanvas = new TCanvas("TrackCanvas","Track canvas");
  addCanvas(trackCanvas);

  TCanvas* hitmapCanvas = new TCanvas("HitMapCanvas","Hit map canvas");
  addCanvas(hitmapCanvas);

  TCanvas* residualsCanvas = new TCanvas("ResidualsCanvas","Residuals canvas");
  addCanvas(residualsCanvas);

  //========= Add each histogram =========//
  
  // Individual plots
  TH1F* trackChi2 = (TH1F*)gDirectory->Get("/tbAnalysis/BasicTracking/trackChi2");
  addPlot(trackCanvas,(TH1*)trackChi2);
    
  // Per detector histograms
  for(int det = 0; det<parameters->nDetectors; det++){
    string detectorID = parameters->detectors[det];
    
    string hitmapHisto = "/tbAnalysis/TestAlgorithm/hitmap_"+detectorID;
    TH2F* hitmap = (TH2F*)gDirectory->Get(hitmapHisto.c_str());
    addPlot(hitmapCanvas,hitmap,"colz");
    
    string residualHisto = "/tbAnalysis/BasicTracking/residualsX_"+detectorID;
    TH1F* residuals = (TH1F*)gDirectory->Get(residualHisto.c_str());
    addPlot(residualsCanvas,residuals);

  }

  // Divide the canvases if needed
  for(int iCanvas=0;iCanvas<canvases.size();iCanvas++){
    int nHistograms = histograms[canvases[iCanvas]].size();
    if(nHistograms == 1) continue;
    if(nHistograms < 4) canvases[iCanvas]->Divide(nHistograms);
    else canvases[iCanvas]->Divide(ceil(nHistograms/2.),2);
  }
  
  // Draw all histograms
  for(int iCanvas=0;iCanvas<canvases.size();iCanvas++){
    canvases[iCanvas]->cd();
    vector<TH1*> histos = histograms[canvases[iCanvas]];
    int nHistograms = histos.size();
    for(int iHisto=0;iHisto<nHistograms;iHisto++){
      canvases[iCanvas]->cd(iHisto+1);
      string style = styles[histos[iHisto]];
      histos[iHisto]->Draw(style.c_str());
    }
  }
  
  // Set event counter
  eventNumber = 0;
  
}

StatusCode GUI::run(Clipboard* clipboard){

  // Draw all histograms
  if(eventNumber%updateNumber == 0){
    for(int iCanvas=0;iCanvas<canvases.size();iCanvas++){
      canvases[iCanvas]->Paint();
      canvases[iCanvas]->Update();
    }
  }
  gSystem->ProcessEvents();
  
  eventNumber++;
  return Success;
  
}
  

void GUI::finalise(){
}

void GUI::addCanvas(TCanvas* canvas){
  canvases.push_back(canvas);
}

void GUI::addPlot(TCanvas* canvas, TH1* histogram, std::string style){
  histograms[canvas].push_back(histogram);
  styles[histogram] = style;
}







