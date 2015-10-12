#include "EventDisplay.h"
#include "TApplication.h"
#include "TPolyLine3D.h"

EventDisplay::EventDisplay(bool debugging)
: Algorithm("EventDisplay"){
  debug = debugging;
}


void EventDisplay::initialise(Parameters* par){
 
  parameters = par;
  
  // Set up histograms
  eventMap = new TH3F("eventMap","eventMap",100,-10,10,100,-10,10,105,-10,200);
  

}

int EventDisplay::run(Clipboard* clipboard){
  

  // Get the tracks
  Timepix3Tracks* tracks = (Timepix3Tracks*)clipboard->get("Timepix3","tracks");
  if(tracks == NULL){
    return 1;
  }
  
  TApplication* rootapp = new TApplication("example",0, 0);
  TCanvas* canv = new TCanvas();
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
}
  

void EventDisplay::finalise(){
  
  
}
