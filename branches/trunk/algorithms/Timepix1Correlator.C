#include "Timepix1Correlator.h"
#include "Timepix1Pixel.h"
#include "Timepix1Cluster.h"
#include "Timepix1Track.h"

Timepix1Correlator::Timepix1Correlator(bool debugging)
: Algorithm("Timepix1Correlator"){
  debug = debugging;
}


void Timepix1Correlator::initialise(Parameters* par){
 
  parameters = par;

  // Initialise histograms per device
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix1") continue;
    
    // Simple histogram per device
    string name = "correlationsX_"+detectorID;
    correlationPlotsX[detectorID] = new TH1F(name.c_str(),name.c_str(),1000,-5,5);

    name = "correlationsY_"+detectorID;
    correlationPlotsY[detectorID] = new TH1F(name.c_str(),name.c_str(),1000,-5,5);

  }
  
  // Initialise member variables
  m_eventNumber = 0;
}

StatusCode Timepix1Correlator::run(Clipboard* clipboard){

  // Get the clusters for the reference detector
  string referenceDetector = parameters->reference;
  Timepix1Clusters* referenceClusters = (Timepix1Clusters*)clipboard->get(referenceDetector,"clusters");
  if(referenceClusters == NULL){
    if(debug) tcout<<"Detector "<<referenceDetector<<" does not have any clusters on the clipboard"<<endl;
    return Success;
  }

  // Loop over all Timepix1 and make plots
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix1") continue;
    
    // Get the clusters
    Timepix1Clusters* clusters = (Timepix1Clusters*)clipboard->get(detectorID,"clusters");
    if(clusters == NULL){
      if(debug) tcout<<"Detector "<<detectorID<<" does not have any clusters on the clipboard"<<endl;
      continue;
    }
    
    // Loop over all clusters and make correlations
    for(int itCluster=0;itCluster<clusters->size();itCluster++){
      for(int itRefCluster=0;itRefCluster<referenceClusters->size();itRefCluster++){
      
      	// Get the clusters
        Timepix1Cluster* cluster = (*clusters)[itCluster];
        Timepix1Cluster* refCluster = (*referenceClusters)[itRefCluster];
      
        // Fill the plots for this device
        correlationPlotsX[detectorID]->Fill(cluster->globalX()-refCluster->globalX());
        correlationPlotsY[detectorID]->Fill(cluster->globalY()-refCluster->globalY());

      }
    }
  }
  
  // Increment event counter
  m_eventNumber++;
  
  // Return value telling analysis to keep running
  return Success;
}

void Timepix1Correlator::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
