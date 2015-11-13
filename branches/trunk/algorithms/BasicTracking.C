#include "BasicTracking.h"
#include "TCanvas.h"

BasicTracking::BasicTracking(bool debugging)
: Algorithm("BasicTracking"){
  debug = debugging;
  
  // Default values for cuts
  timinigCut = 200./1000000000.;		// 200 ns
  spatialCut = 0.2; 							// 200 um
  minHitsOnTrack = 5;

}


void BasicTracking::initialise(Parameters* par){
 
  parameters = par;
  
  // Set up histograms
  trackChi2 = new TH1F("trackChi2","trackChi2",150,0,150);
  trackChi2ndof = new TH1F("trackChi2ndof","trackChi2ndof",100,0,50);
  clustersPerTrack = new TH1F("clustersPerTrack","clustersPerTrack",10,0,10);
  tracksPerEvent = new TH1F("tracksPerEvent","tracksPerEvent",100,0,100);
  trackAngleX = new TH1F("trackAngleX","trackAngleX",2000,-1.,1.);
  trackAngleY = new TH1F("trackAngleY","trackAngleY",2000,-1.,1.);
  
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
      if(debug) tcout<<"Detector "<<detectorID<<" does not have any clusters on the clipboard"<<endl;
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
    track->setTimestamp(cluster->timestamp());
    used[cluster] = true;
		// Get the cluster time
    long long int timestamp = cluster->timestamp();
    
//    // Loop over each subsequent plane and look for the closest cluster in a given time window
//    for(int det=0; det<detectors.size(); det++){
//      string detectorID = detectors[det];
//      // Don't look for clusters if there are none, or if it is the reference device
//      if(detectorID == reference) continue;
//      if(clusters[detectorID] == NULL) continue;
//      // If the detector is excluded from tracking ignore it
//      if(parameters->excludedFromTracking.count(detectorID) != 0) continue;
//      // Get the closest cluster
//      Timepix3Cluster* newCluster = getNearestCluster(cluster, used, clusters[detectorID]);
//      if(newCluster == NULL) continue;
//			// Add the cluster to the track
//      track->addCluster(newCluster);
//      used[newCluster] = true;
//    }

    // Loop over each subsequent plane and look for a cluster within 100 ns
    for(int det=0; det<detectors.size(); det++){
      if(detectors[det] == reference) continue;
      if(clusters[detectors[det]] == NULL) continue;
      Timepix3Cluster* newCluster = getNearestCluster(timestamp, (*clusters[detectors[det]]) );
      if( ((newCluster->timestamp() - timestamp) / (4096.*40000000.)) > (10./1000000000.) ) continue;
      // Check if spatially more than 200 um
      if( abs(cluster->globalX() - newCluster->globalX()) > spatialCut || abs(cluster->globalY() - newCluster->globalY()) > spatialCut ) continue;

      // If excluded from tracking, add as an associated cluster, otherwise add it as a real cluster
      if(parameters->excludedFromTracking.count(detectors[det]) != 0) continue;
      track->addCluster(newCluster);
    }

    // Now should have a track with one cluster from each plane
    if(track->nClusters() < minHitsOnTrack){
      delete track;
      continue;
    }
    
    // Fit the track and save it
    track->fit();
    tracks->push_back(track);
    
    // Fill histograms
    trackChi2->Fill(track->chi2());
    clustersPerTrack->Fill(track->nClusters());
    trackChi2ndof->Fill(track->chi2ndof());
    trackAngleX->Fill(atan(track->m_direction.X()));
    trackAngleY->Fill(atan(track->m_direction.Y()));
    
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
  
//  tcout<<"Made "<<tracks->size()<<" tracks"<<endl;
  if(tracks->size() > 0){
    clipboard->put("Timepix3","tracks",(TestBeamObjects*)tracks);
    tracksPerEvent->Fill(tracks->size());
  }

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

//Timepix3Cluster* BasicTracking::getNearestCluster(Timepix3Cluster* cluster, map<Timepix3Cluster*, bool> used, Timepix3Clusters* clusters){
//  
//  // Loop over all clusters and return the closest in space with an acceptable time cut
//  Timepix3Cluster* bestCluster = NULL;
//  double closestApproach = 1000000.;
//  
//  // Loop over all clusters
//  for(int iCluster=0;iCluster<clusters->size();iCluster++){
//    Timepix3Cluster* candidate = (*clusters)[iCluster];
//    // Check if within time window
//    if( abs((double)((candidate->timestamp() - cluster->timestamp()) / (4096.*40000000.))) > timinigCut ) continue;
//    // Check how close it is (2D - assumes z-axis parallel to beam)
//    if(bestCluster == NULL){ bestCluster = candidate; continue; }
//    double distanceX = candidate->globalX() - cluster->globalX();
//    double distanceY = candidate->globalY() - cluster->globalY();
//    double approach = sqrt(distanceX*distanceX + distanceY*distanceY);
//    // Check if it is closer than previous clusters, and apply spatial cut
//    if( approach < closestApproach && approach < spatialCut){
//      bestCluster = candidate;
//      closestApproach = approach;
//    }
//  }
//
//  return bestCluster;
//}

void BasicTracking::finalise(){
  
  
}
