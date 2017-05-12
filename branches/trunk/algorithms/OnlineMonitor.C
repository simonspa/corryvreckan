#include "OnlineMonitor.h"
#include <TVirtualPadEditor.h>

OnlineMonitor::OnlineMonitor(bool debugging)
: Algorithm("OnlineMonitor"){
  debug = debugging;
  updateNumber = 500;
}

void OnlineMonitor::initialise(Parameters* par){
  
  // Make the local pointer to the global parameters
  parameters = par;

  // TApplication keeps the canvases persistent
  app = new TApplication("example",0, 0);
  
  // Make the GUI
  gui = new GuiDisplay();
  
  // Make the main window object and set the attributes
  gui->m_mainFrame = new TGMainFrame(gClient->GetRoot(), 400, 400);
  gui->m_mainFrame->SetCleanup(kDeepCleanup);
  gui->m_mainFrame->DontCallClose();
  
  // Add canvases and histograms

  // Track canvas
//  AddCanvas("TrackCanvas");
//  AddButton("Tracking","TrackCanvas");
//  AddHisto("TrackCanvas","/tbAnalysis/BasicTracking/trackChi2");
//  AddHisto("TrackCanvas","/tbAnalysis/BasicTracking/trackAngleX");

  // Hitmap canvas
  AddCanvas("HitmapCanvas");
  AddButton("HitMap","HitmapCanvas");
  
//  AddHisto("HitmapCanvas","/tbAnalysis/TestAlgorithm/clusterTot_W0013_G03");
//  AddHisto("HitmapCanvas","/tbAnalysis/TestAlgorithm/eventTimes_W0013_G03");
//  AddHisto("HitmapCanvas","/tbAnalysis/TestAlgorithm/hitmap_W0013_G03","colz");

  // Per detector histograms
  for(int det = 0; det<parameters->nDetectors; det++){
    string detectorID = parameters->detectors[det];
    
    string hitmap = "/tbAnalysis/TestAlgorithm/hitmap_"+detectorID;
    AddHisto("HitmapCanvas",hitmap,"colz");
    
//    string chargeHisto = "/tbAnalysis/TestAlgorithm/clusterTot_"+detectorID;
//    AddHisto("HitmapCanvas",chargeHisto);
    
//    if(parameters->excludedFromTracking[detectorID]) continue;
//    string residualHisto = "/tbAnalysis/BasicTracking/residualsX_"+detectorID;
//    AddHisto(residualsCanvas,residualHisto);
    
  }

  // Set up the main frame before drawing
  gui->m_mainFrame->SetWindowName("CLIC Tesbeam Monitoring");
  gui->m_mainFrame->MapSubwindows();
  gui->m_mainFrame->Resize();
  
  // Draw the main frame
  gui->m_mainFrame->MapWindow();
  
  // Initialise member variables
  eventNumber = 0;
}

StatusCode OnlineMonitor::run(Clipboard* clipboard){
  
  // Draw all histograms
  if(eventNumber%updateNumber == 0){
    int nCanvases = gui->canvasVector.size();
    for(int i=0;i<nCanvases;i++){
      gui->canvasVector[i]->GetCanvas()->Paint();
      gui->canvasVector[i]->GetCanvas()->Update();
    }
  }
  gSystem->ProcessEvents();
  
  eventNumber++;
  return Success;

}

void OnlineMonitor::finalise(){
  
  if(debug) tcout<<"Analysed "<<eventNumber<<" events"<<endl;
  
}

void OnlineMonitor::AddCanvas(string canvasName){
  
  gui->canvases[canvasName] = new TRootEmbeddedCanvas(canvasName.c_str(), gui->m_mainFrame, 600, 400);
  gui->canvasVector.push_back(gui->canvases[canvasName]);
  gui->m_mainFrame->AddFrame(gui->canvases[canvasName],
                             new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

}

void OnlineMonitor::AddStackedCanvas(string canvasName){
  
  this->AddCanvas(canvasName);
  gui->stackedCanvas[gui->canvases[canvasName]] = true;
  
}

void OnlineMonitor::AddHisto(string canvasName, string histoName, string style){
  
  gui->histograms[gui->canvases[canvasName]].push_back((TH1*)gDirectory->Get(histoName.c_str()));
  gui->styles[gui->histograms[gui->canvases[canvasName]].back()] = style;

}

void OnlineMonitor::AddButton(string buttonName, string canvasName){
  gui->buttons[buttonName] = new TGTextButton(gui->m_mainFrame, buttonName.c_str());
  gui->m_mainFrame->AddFrame(gui->buttons[buttonName], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 1));
  string command = "Display(=\"" + canvasName + "\")";
  tcout<<"Connecting button with command "<<command.c_str()<<endl;
  gui->buttons[buttonName]->Connect("Pressed()", "GuiDisplay", gui, command.c_str());
}


