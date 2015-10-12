#include "GUI.h"
#include "TApplication.h"
#include "TPolyLine3D.h"
#include "TSystem.h"

GUI::GUI(bool debugging)
: Algorithm("GUI"){
  debug = debugging;
}

void startBrowser(void* gui){
  
  cout<<"I still run"<<endl;

//  TApplication* testApp = new TApplication("example",0, 0);
//  TBrowser* test = new TBrowser();
//  testApp->Run();
  
  ((GUI*)gui)->rootapp = new TApplication("example",0, 0);
  ((GUI*)gui)->browser = new TBrowser();
//  gSystem->Sleep(1);
  ((GUI*)gui)->rootapp->Run(true);
  
}

void update(void*){
  
  while(1){
	  gSystem->ProcessEvents();
	  gSystem->Sleep(1);
  }
  
}

void GUI::initialise(Parameters* par){
 
  parameters = par;
  
  // Set up histograms
  eventMap = new TH3F("eventMap","eventMap",100,-10,10,100,-10,10,105,-10,200);
  
//  TApplication* rootapp = new TApplication("example",0, 0);
//  browser = new TBrowser();
//  rootapp->Run();

//  rootapp = new TApplication("example",0, 0);
//  browser = new TBrowser();
  browserThread = new TThread("browserThread", startBrowser, (void*) this);
//  browserThread->Run();
  updateThread = new TThread("updateThread", update, (void*) 0);
//  updateThread->Run();

}

int GUI::run(Clipboard* clipboard){
  
//  updateThread->Run();

  // Get the tracks
//  Timepix3Tracks* tracks = (Timepix3Tracks*)clipboard->get("Timepix3","tracks");
//  if(tracks == NULL){
//    return 1;
//  }
  
//  rootapp = new TApplication("example",0, 0);
//  browser = new TBrowser();
//  rootapp->Run(false);
  
//  browser->Refresh();
/*  TCanvas* canv = new TCanvas();
  eventMap->DrawCopy("");

  // Loop over all tracks. Add the cluster points to the histogram and draw a line for the tracks
  for(int iTrack=0; iTrack<tracks->size(); iTrack++){
    
    // Get the track
    Timepix3Track* track = (*tracks)[iTrack];
    
    // Get all clusters on the track
    Timepix3Clusters trackClusters = track->clusters();
    
    // Fill the event map
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Timepix3Cluster* trackCluster = trackClusters[iTrackCluster];
      eventMap->Fill(trackCluster->globalX(), trackCluster->globalY(), trackCluster->globalZ());
    }
    
    ROOT::Math::XYZPoint linestart = track->intercept(0);
    ROOT::Math::XYZPoint linestop = track->intercept(200);
    
    TPolyLine3D *line = new TPolyLine3D(2);
    line->SetPoint(0,linestart.X(),linestart.Y(),linestart.Z());
    line->SetPoint(1,linestop.X(),linestop.Y(),linestop.Z());
    line->SetLineColor(2);
    line->Draw("same");

  }
  
    eventMap->DrawCopy("same,box");
		canv->Update();
    rootapp->Run();
    return 1;
 */
  
  return 1;
}
  

void GUI::finalise(){
  
  
}

