#include "BasicTracking.h"
#include "TCanvas.h"

BasicTracking::BasicTracking(bool debugging)
: Algorithm("BasicTracking"){
  debug = debugging;
}


void BasicTracking::initialise(Parameters* par){
 
  parameters = par;
  
  // Set up histograms
  trackChi2 = new TH1F("trackChi2","trackChi2",150,0,150);
  
  // Loop over all Timepix3
  for(int det = 0; det<parameters->nDetectors; det++){
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    string name = "residualsX_"+detectorID;
    residualsX[detectorID] = new TH1F(name.c_str(),name.c_str(),400,-0.2,0.2);
    name = "residualsY_"+detectorID;
    residualsY[detectorID] = new TH1F(name.c_str(),name.c_str(),400,-0.2,0.2);
  }

}

int BasicTracking::run(Clipboard* clipboard){
  
  // Container for all clusters
  map<string,Timepix3Clusters*> clusters;
  vector<string> detectors;
  
  // Track container
  Timepix3Tracks* tracks = new Timepix3Tracks();

  // Loop over all Timepix3 and get clusters
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
		
		// Get the clusters
    Timepix3Clusters* tempClusters = (Timepix3Clusters*)clipboard->get(detectorID,"clusters");
    if(tempClusters == NULL){
      tcout<<"Detector "<<detectorID<<" does not have any clusters on the clipboard"<<endl;
    }else{
    	// Store them
      clusters[detectorID] = tempClusters;
      detectors.push_back(detectorID);
    }
  }
  
  // If there are no detectors then stop trying to track
  if(detectors.size() == 0) return 1;
  
  // Use the first plane as a seeding plane. For something quick, look a cluster in < 100 ns in the next plane, and continue
  string reference = parameters->reference;
  map<Timepix3Cluster*, bool> used;
  
  // If no clusters on reference plane, stop
  if(clusters[reference] == NULL) return 1;

  // Loop over all clusters
  for(int iSeedCluster=0;iSeedCluster<clusters[reference]->size();iSeedCluster++){

    // Make a new track
    Timepix3Track* track = new Timepix3Track();
    // Get the cluster
    Timepix3Cluster* cluster = (*clusters[reference])[iSeedCluster];
    // Add the cluster to the track
    track->addCluster(cluster);
    used[cluster] = true;
		// Get the cluster time
    long long int timestamp = cluster->timestamp();
    
    // Loop over each subsequent plane and look for a cluster within 100 ns
    for(int det=0; det<detectors.size(); det++){
      if(detectors[det] == reference) continue;
      Timepix3Cluster* newCluster = getNearestCluster(timestamp, (*clusters[detectors[det]]) );
      if( ((newCluster->timestamp() - timestamp) / (4096.*40000000.)) > (10./1000000000.) ) continue;
      // Check if spatially more than 200 um
      if( abs(cluster->globalX() - newCluster->globalX()) > 0.3 || abs(cluster->globalY() - newCluster->globalY()) > 0.3 ) continue;
      track->addCluster(newCluster);
    }
   
    // Now should have a track with one cluster from each plane
    if(track->nClusters() < 5){
      delete track;
      continue;
    }
    
    // Fit the track and save it
    track->fit();
    tracks->push_back(track);
    
    // Fill histograms
    trackChi2->Fill(track->chi2());
    
    // Make residuals
    Timepix3Clusters trackClusters = track->clusters();
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Timepix3Cluster* trackCluster = trackClusters[iTrackCluster];
      string detectorID = trackCluster->detectorID();
      ROOT::Math::XYZPoint intercept = track->intercept(trackCluster->globalZ());
      residualsX[detectorID]->Fill(intercept.X() - trackCluster->globalX());
      residualsY[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
    }
    
  }
  
  tcout<<"Made "<<tracks->size()<<" tracks"<<endl;
  if(tracks->size() > 0) clipboard->put("Timepix3","tracks",(TestBeamObjects*)tracks);

  return 1;
}
  
Timepix3Cluster* BasicTracking::getNearestCluster(long long int timestamp, Timepix3Clusters clusters){
  
  Timepix3Cluster* bestCluster = NULL;
  // Loop over all clusters and return the one with the closest timestamp
  for(int iCluster=0;iCluster<clusters.size();iCluster++){
    Timepix3Cluster* cluster = clusters[iCluster];
    if(bestCluster == NULL) bestCluster = cluster;
    if(abs(cluster->timestamp() - timestamp) < abs(bestCluster->timestamp()-timestamp)) bestCluster = cluster;
  }

  return bestCluster;
}

void BasicTracking::finalise(){
  
  
}
