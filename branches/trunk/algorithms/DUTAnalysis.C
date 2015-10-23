#include "DUTAnalysis.h"
#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

DUTAnalysis::DUTAnalysis(bool debugging)
: Algorithm("DUTAnalysis"){
  debug = debugging;
}


void DUTAnalysis::initialise(Parameters* par){
 
  parameters = par;

  // Initialise histograms per device
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    
  }

  // Initialise single histograms
  tracksVersusTime = new TH1F("tracksVersusTime","tracksVersusTime",6000000,0,60);
  associatedTracksVersusTime = new TH1F("associatedTracksVersusTime","associatedTracksVersusTime",6000000,0,60);
  residualsX = new TH1F("residualsX","residualsX",400,-0.2,0.2);
  residualsY = new TH1F("residualsY","residualsY",400,-0.2,0.2);
  residualsTime = new TH1F("residualsTime","residualsTime",2000,-0.000001,0.000001);
  
  // Initialise member variables
  m_eventNumber = 0;
  m_nAlignmentClusters = 0;
}

int DUTAnalysis::run(Clipboard* clipboard){
  
  // Timing cut for association
  double timingCut = 100./1000000000.;
  long long int timingCutInt = (timingCut * 4096. * 40000000.);
  
  // Spatial cut
  double spatialCut = 0.2; // 75 um

  // Get the tracks from the clipboard
  Timepix3Tracks* tracks = (Timepix3Tracks*)clipboard->get("Timepix3","tracks");
  if(tracks == NULL){
    if(debug) tcout<<"No tracks on the clipboard"<<endl;
    return 1;
  }

  // Get the DUT clusters from the clipboard
  Timepix3Clusters* clusters = (Timepix3Clusters*)clipboard->get(parameters->DUT,"clusters");
  if(clusters == NULL){
    if(debug) tcout<<"No DUT clusters on the clipboard"<<endl;
    return 1;
  }

  // Loop over all tracks
  for(int itTrack=0;itTrack<tracks->size();itTrack++){
    
    // Get the track pointer
    Timepix3Track* track = (*tracks)[itTrack];
    tracksVersusTime->Fill( (double)track->timestamp() / (4096.*40000000.) );
    
    // Loop over all DUT clusters
    for(int itCluster=0;itCluster<clusters->size();itCluster++){

      // Get the cluster pointer
      Timepix3Cluster* cluster = (*clusters)[itCluster];
      
      // Check if the cluster is close in time
      if( abs(cluster->timestamp() - track->timestamp()) > timingCutInt ) continue;
      
      // Check distance between track and cluster
      ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
      double xdistance = intercept.X() - cluster->globalX();
      double ydistance = intercept.Y() - cluster->globalY();
      if( abs(xdistance) > spatialCut) continue;
      if( abs(ydistance) > spatialCut) continue;
 
      // We now have an associated cluster! Fill plots
      associatedTracksVersusTime->Fill( (double)track->timestamp() / (4096.*40000000.) );
      residualsX->Fill(xdistance);
      residualsY->Fill(ydistance);
      track->addAssociatedCluster(cluster);
      m_nAlignmentClusters++;
      
      // Only allow one associated cluster per track
      break;
      
    }
    
  }

  // Increment event counter
  m_eventNumber++;
  
//  if(m_nAlignmentClusters > 10000) return 0;
  // Return value telling analysis to keep running
  return 1;
}

void DUTAnalysis::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
