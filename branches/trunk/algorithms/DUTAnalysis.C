#include "DUTAnalysis.h"
#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"
#include "SpidrSignal.h"

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
  
  tracksVersusPowerOnTime = new TH1F("tracksVersusPowerOnTime","tracksVersusPowerOnTime",1200000,-0.01,0.11);
  associatedTracksVersusPowerOnTime = new TH1F("associatedTracksVersusPowerOnTime","associatedTracksVersusPowerOnTime",1200000,-0.01,0.11);

  // Initialise member variables
  m_eventNumber = 0;
  m_nAlignmentClusters = 0;
  m_powerOnTime = 0;
  m_powerOffTime = 0;

}

StatusCode DUTAnalysis::run(Clipboard* clipboard){
  
//  tcout<<"Power on time: "<<m_powerOnTime/(4096. * 40000000.)<<endl;
//  tcout<<"Power off time: "<<m_powerOffTime/(4096. * 40000000.)<<endl;
//  tcout<<endl;
  
  // Timing cut for association
  double timingCut = 200./1000000000.; // 200 ns
  long long int timingCutInt = (timingCut * 4096. * 40000000.);
  
  // Spatial cut
  double spatialCut = 0.2; // 200 um

  // Track chi2/ndof cut
  double chi2ndofCut = 7.;

  // Power pulsing variable initialisation - get signals from SPIDR for this device
  double timeSincePowerOn = 0.;
  
  // If the power was switched off/on in the last event we no longer have a power on/off time
  if(m_powerOffTime != 0 && m_powerOffTime > m_powerOnTime) m_powerOnTime = 0;
  if(m_powerOnTime != 0 && m_powerOnTime > m_powerOffTime) m_powerOffTime = 0;
  
  // Now update the power pulsing with any new signals
  SpidrSignals* spidrData = (SpidrSignals*)clipboard->get(parameters->DUT,"SpidrSignals");
  // If there are new signals
  if(spidrData != NULL){
    // Loop over all signals registered
    int nSignals = spidrData->size();
    for(int iSig=0;iSig<nSignals;iSig++){
      // Get the signal
      SpidrSignal* signal = (*spidrData)[iSig];
    	// Register the power on or power off time
      if(signal->type() == "powerOn"){
        m_powerOnTime = signal->timestamp();
//        tcout<<"Power on time: "<<m_powerOnTime/(4096. * 40000000.)<<endl;
      }
      if(signal->type() == "powerOff"){
        m_powerOffTime = signal->timestamp();
//        tcout<<"Power off time: "<<m_powerOffTime/(4096. * 40000000.)<<endl;
      }
    }
  }
  
  // Get the tracks from the clipboard
  Timepix3Tracks* tracks = (Timepix3Tracks*)clipboard->get("Timepix3","tracks");
  if(tracks == NULL){
    if(debug) tcout<<"No tracks on the clipboard"<<endl;
    return Success;
  }

  // Get the DUT clusters from the clipboard
  Timepix3Clusters* clusters = (Timepix3Clusters*)clipboard->get(parameters->DUT,"clusters");
  if(clusters == NULL){
    if(debug) tcout<<"No DUT clusters on the clipboard"<<endl;
    return Success;
  }

  // Loop over all tracks
  for(int itTrack=0;itTrack<tracks->size();itTrack++){
    
    // Get the track pointer
    Timepix3Track* track = (*tracks)[itTrack];
    
    // Check if it intercepts the DUT
//    if(!intercept(track,parameters->DUT)) continue;
    
    // Cut on the chi2/ndof
    if(track->chi2ndof() > chi2ndofCut) continue;
    tracksVersusTime->Fill( (double)track->timestamp() / (4096.*40000000.) );
    
    // Check time since power on (if power pulsing).
    // If power off time not known it will be 0. If it is known, then the track should arrive before the power goes off
    if( m_powerOnTime != 0 && m_powerOffTime == 0 ||
        m_powerOnTime != 0 && (m_powerOffTime - track->timestamp()) > 0){
      timeSincePowerOn = (double)(track->timestamp() - m_powerOnTime) / (4096.*40000000.);
      tracksVersusPowerOnTime->Fill(timeSincePowerOn);
    }
    
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
      
      // Fill power pulsing response
      if( m_powerOnTime != 0 && m_powerOffTime == 0 ||
         m_powerOnTime != 0 && (m_powerOffTime - track->timestamp()) > 0){
        associatedTracksVersusPowerOnTime->Fill(timeSincePowerOn);
      }

      // Only allow one associated cluster per track
      break;
      
    }
    
  }

  // Increment event counter
  m_eventNumber++;
  
//  if(m_nAlignmentClusters > 10000) return Failure;
  // Return value telling analysis to keep running
  return Success;
}

void DUTAnalysis::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}

// Function to check if a track goes through a given device
//bool DUTAnalysis::intercept(Timepix3Track*, string device){
//  
//  // Get the global intercept of the track and the device
//  ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
//
//  // Transform to the local co-ordinates
//  
//  // Check if the row/column number is outside the acceptable range
//  
//  
//}
