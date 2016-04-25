#include "DUTAnalysis.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"
#include "SpidrSignal.h"

DUTAnalysis::DUTAnalysis(bool debugging)
: Algorithm("DUTAnalysis"){
  debug = debugging;
}


void DUTAnalysis::initialise(Parameters* par){
 
  // Pick up a copy of the parameters
  parameters = par;

  // Initialise single histograms
  tracksVersusTime = new TH1F("tracksVersusTime","tracksVersusTime",300000,0,300);
  associatedTracksVersusTime = new TH1F("associatedTracksVersusTime","associatedTracksVersusTime",300000,0,300);
  residualsX = new TH1F("residualsX","residualsX",400,-0.2,0.2);
  residualsY = new TH1F("residualsY","residualsY",400,-0.2,0.2);
  residualsTime = new TH1F("residualsTime","residualsTime",2000,-0.001,0.001);
  
  hTrackCorrelationX = new TH1F("hTrackCorrelationX","hTrackCorrelationX",4000,-10.,10.);
  hTrackCorrelationY = new TH1F("hTrackCorrelationY","hTrackCorrelationY",4000,-10.,10.);
  hTrackCorrelationTime = new TH1F("hTrackCorrelationTime","hTrackCorrelationTime",2000000,-0.005,0.005);
  clusterToTVersusTime = new TH2F("clusterToTVersusTime","clusterToTVersusTime",300000,0.,300.,200,0,1000);
  
  residualsTimeVsTime = new TH2F("residualsTimeVsTime","residualsTimeVsTime",20000,0,200,400,-0.0005,0.0005);

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
  Tracks* tracks = (Tracks*)clipboard->get("tracks");
  if(tracks == NULL){
    if(debug) tcout<<"No tracks on the clipboard"<<endl;
    return Success;
  }
  
  // Get the DUT clusters from the clipboard
  Clusters* clusters = (Clusters*)clipboard->get(parameters->DUT,"clusters");
  if(clusters == NULL){
    if(debug) tcout<<"No DUT clusters on the clipboard"<<endl;
    return Success;
  }

  // Loop over all tracks
  for(int itTrack=0;itTrack<tracks->size();itTrack++){
    
    // Get the track pointer
    Track* track = (*tracks)[itTrack];
    
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

    // Correlation plot
    for(int itCluster=0;itCluster<clusters->size();itCluster++){
      
      // Get the cluster pointer
      Cluster* cluster = (*clusters)[itCluster];
      
      // Check if the cluster is close in time
//      if( abs(cluster->timestamp() - track->timestamp()) > timingCutInt ) continue;

      // Check distance between track and cluster
      ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());

      // Fill the correlation plot
      hTrackCorrelationX->Fill(intercept.X() - cluster->globalX());
      hTrackCorrelationY->Fill(intercept.Y() - cluster->globalY());
      hTrackCorrelationTime->Fill( (double)(track->timestamp() - cluster->timestamp()) / (4096.*40000000.));
      
      if( fabs(intercept.X() - cluster->globalX()) < 0.1 &&
          fabs(intercept.Y() - cluster->globalY()) < 0.1){
        residualsTime->Fill((double)(track->timestamp() - cluster->timestamp()) / (4096.*40000000.));
        residualsTimeVsTime->Fill( (double)track->timestamp() / (4096.*40000000.), (double)(track->timestamp() - cluster->timestamp()) / (4096.*40000000.));
      }
    }
    
    // Loop over all DUT clusters
    for(int itCluster=0;itCluster<clusters->size();itCluster++){

      // Get the cluster pointer
      Cluster* cluster = (*clusters)[itCluster];
      
      // Fill the tot histograms on the first run
      if(itTrack == 0) clusterToTVersusTime->Fill((double)cluster->timestamp() / (4096.*40000000.), cluster->tot());
      
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
//bool DUTAnalysis::intercept(Track*, string device){
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
