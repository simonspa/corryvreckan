#include "SpatialTracking.h"
#include "Timepix1Cluster.h"
#include "Timepix1Track.h"
#include "KDTreeTimepix1.h"

SpatialTracking::SpatialTracking(bool debugging)
: Algorithm("SpatialTracking"){
  debug = debugging;
  spatialCut = 0.05;
  minHitsOnTrack = 6;
}

/*
 
 This algorithm performs the track finding using only spatial information 
 (no timing). It is based on a linear extrapolation along the z axis, followed
 by a nearest neighbour search, and should be well adapted to testbeam 
 reconstruction with a mostly colinear beam.
 
 */

void SpatialTracking::initialise(Parameters* par){
 
  parameters = par;

  // Set up histograms
  trackChi2 = new TH1F("trackChi2","trackChi2",150,0,150);
  trackChi2ndof = new TH1F("trackChi2ndof","trackChi2ndof",100,0,50);
  clustersPerTrack = new TH1F("clustersPerTrack","clustersPerTrack",10,0,10);
  tracksPerEvent = new TH1F("tracksPerEvent","tracksPerEvent",100,0,100);
  trackAngleX = new TH1F("trackAngleX","trackAngleX",2000,-0.01,0.01);
  trackAngleY = new TH1F("trackAngleY","trackAngleY",2000,-0.01,0.01);
  
  // Loop over all Timepix3
  for(int det = 0; det<parameters->nDetectors; det++){
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix1") continue;
    string name = "residualsX_"+detectorID;
    residualsX[detectorID] = new TH1F(name.c_str(),name.c_str(),400,-0.2,0.2);
    name = "residualsY_"+detectorID;
    residualsY[detectorID] = new TH1F(name.c_str(),name.c_str(),400,-0.2,0.2);
  }
  
  // Initialise member variables
  m_eventNumber = 0;
}

StatusCode SpatialTracking::run(Clipboard* clipboard){
  
  // Container for all clusters, and detectors in tracking
  map<string,KDTreeTimepix1> trees;
  vector<string> detectors;
  Timepix1Clusters* referenceClusters;

  // Output track container
  Timepix1Tracks* tracks = new Timepix1Tracks();
  
  // Loop over all Timepix1 and get clusters
  double minZ = 1000.;
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix1
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix1") continue;
    
    // Get the clusters
    Timepix1Clusters* tempClusters = (Timepix1Clusters*)clipboard->get(detectorID,"clusters");
    if(tempClusters == NULL){
      if(debug) tcout<<"Detector "<<detectorID<<" does not have any clusters on the clipboard"<<endl;
    }else{
      // Store the clusters of the first plane in Z as the reference
      if(parameters->detector[detectorID]->displacementZ() < minZ){
        referenceClusters = tempClusters;
        minZ = parameters->detector[detectorID]->displacementZ();
      }
      if(tempClusters->size() == 0) continue;
      
      // Make trees of the clusters on each plane
      KDTreeTimepix1 clusterTree(*tempClusters);
      trees[detectorID] = clusterTree;
      detectors.push_back(detectorID);
      if(debug) tcout<<"Picked up "<<tempClusters->size()<<" clusters on device "<<detectorID<<endl;
    }
  }
  
  // If there are no detectors then stop trying to track
  if(detectors.size() == 0) return Success;
  
  // Keep a note of which clusters have been used
  map<Timepix1Cluster*, bool> used;
  
  // Loop over all clusters
  int nSeedClusters = referenceClusters->size();
  for(int iSeedCluster=0;iSeedCluster<nSeedClusters;iSeedCluster++){
    
    if(debug) tcout<<"==> seed cluster "<<iSeedCluster<<endl;

    // Make a new track
    Timepix1Track* track = new Timepix1Track();
    
    // Get the cluster
    Timepix1Cluster* cluster = (*referenceClusters)[iSeedCluster];
    
    // Add the cluster to the track
    track->addCluster(cluster);
    used[cluster] = true;
    
    if(cluster->tot() == 0) continue;
    // Loop over each subsequent planes. For each plane, if extrapolate
    // the hit from the previous plane along the z axis, and look for
    // a neighbour on the new plane. We started on the most upstream
    // plane, so first detector is 1 (not 0)
    for(int det=1; det<detectors.size(); det++){
      
      if(trees.count(detectors[det]) == 0) continue;
      
      // If excluded from tracking ignore this plane
      if(parameters->excludedFromTracking.count(detectors[det]) != 0) continue;
      
      // Get the closest neighbour
      Timepix1Cluster* closestCluster = trees[detectors[det]].getClosestNeighbour(cluster);

      // If it is used do nothing
      if(used[closestCluster]) continue;
      
      // Check if it is within the spatial window
      double distance = sqrt((cluster->globalX() - closestCluster->globalX())*(cluster->globalX() - closestCluster->globalX()) + (cluster->globalY() - closestCluster->globalY())*(cluster->globalY() - closestCluster->globalY()));
      
      if(distance > spatialCut) continue;
    
      // Add the cluster to the track
      track->addCluster(closestCluster);
      cluster = closestCluster;
      if(debug) tcout<<"- added cluster to track"<<endl;
      
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
    Timepix1Clusters trackClusters = track->clusters();
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Timepix1Cluster* trackCluster = trackClusters[iTrackCluster];
      string detectorID = trackCluster->detectorID();
      ROOT::Math::XYZPoint intercept = track->intercept(trackCluster->globalZ());
      residualsX[detectorID]->Fill(intercept.X() - trackCluster->globalX());
      residualsY[detectorID]->Fill(intercept.Y() - trackCluster->globalY());
    }
    
  }
  
  // Save the tracks on the clipboard
  if(debug) tcout<<"- produced "<<tracks->size()<<" tracks"<<endl;
  if(tracks->size() > 0){
    clipboard->put("Timepix1","tracks",(TestBeamObjects*)tracks);
    tracksPerEvent->Fill(tracks->size());
  }

  return Success;

}

void SpatialTracking::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
