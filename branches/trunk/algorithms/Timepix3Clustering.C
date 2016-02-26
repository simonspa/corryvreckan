#include "Timepix3Clustering.h"

Timepix3Clustering::Timepix3Clustering(bool debugging)
: Algorithm("Timepix3Clustering"){
  timingCut = 0.0000001; // 100 ns
  debug = debugging;
}


void Timepix3Clustering::initialise(Parameters* par){
 
  parameters = par;
  timingCutInt = (timingCut * 4096. * 40000000.);
  
}

// Sort function for pixels from low to high times
bool sortByTime(Pixel* pixel1, Pixel* pixel2){
  return (pixel1->m_timestamp < pixel2->m_timestamp);
}

StatusCode Timepix3Clustering::run(Clipboard* clipboard){

  // Loop over all Timepix3 and for each device perform the clustering
  for(int det = 0; det<parameters->nDetectors; det++){
   
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    
    // Get the pixels
    Pixels* pixels = (Pixels*)clipboard->get(detectorID,"pixels");
    if(pixels == NULL){
      if(debug) tcout<<"Detector "<<detectorID<<" does not have any pixels on the clipboard"<<endl;
      continue;
    }
    if(debug) tcout<<"Picked up "<<pixels->size()<<" pixels for device "<<detectorID<<endl;
    
//    if(pixels->size() > 500.){
//      if(debug) tcout<<"Skipping large event with "<<pixels->size()<<" pixels for device "<<detectorID<<endl;
//      continue;
//    }
    
    // Sort the pixels from low to high timestamp
    std::sort(pixels->begin(),pixels->end(),sortByTime);
    int totalPixels = pixels->size();
    
    // Make the cluster storage
    Clusters* deviceClusters = new Clusters();
   
    // Keep track of which pixels are used
    map<Pixel*, bool> used;
    
    // Start to cluster
    for(int iP=0;iP<pixels->size();iP++){
      Pixel* pixel = (*pixels)[iP];

      // Check if pixel is used
      if(used[pixel]) continue;
      
      // Make the new cluster object
      Cluster* cluster = new Cluster();
      if(debug) tcout<<"==== New cluster"<<endl;
      
      // Keep adding hits to the cluster until no more are found
      cluster->addPixel(pixel);
      long long int clusterTime = pixel->m_timestamp;
      used[pixel] = true;
      if(debug) tcout<<"Adding pixel: "<<pixel->m_row<<","<<pixel->m_column<<endl;
      int nPixels = 0;
      while(cluster->size() != nPixels){
        
        nPixels = cluster->size();
        // Loop over all pixels
        for(int iNeighbour=(iP+1);iNeighbour<totalPixels;iNeighbour++){
          Pixel* neighbour = (*pixels)[iNeighbour];
          // Check if they are compatible in time with the cluster pixels
          if( (neighbour->m_timestamp - clusterTime) > timingCutInt ) break;
//          if(!closeInTime(neighbour,cluster)) break;
         // Check if they have been used
          if(used[neighbour]) continue;
          // Check if they are touching cluster pixels
          if(!touching(neighbour,cluster)) continue;
           // Add to cluster
          cluster->addPixel(neighbour);
          clusterTime = neighbour->m_timestamp;
          used[neighbour] = true;
          if(debug) tcout<<"Adding pixel: "<<neighbour->m_row<<","<<neighbour->m_column<<endl;
        }
        
      }
      
      // Finalise the cluster and save it
      calculateClusterCentre(cluster);
      deviceClusters->push_back(cluster);
      
    }
    
    // Put the clusters on the clipboard
    if(deviceClusters->size() > 0) clipboard->put(detectorID,"clusters",(TestBeamObjects*)deviceClusters);
    if(debug) tcout<<"Made "<<deviceClusters->size()<<" clusters for device "<<detectorID<<endl;

  }

  return Success;
}

// Check if a pixel touches any of the pixels in a cluster
bool Timepix3Clustering::touching(Pixel* neighbour,Cluster* cluster){
  
  bool Touching = false;
  Pixels pixels = cluster->pixels();
  for(int iPix=0;iPix<pixels.size();iPix++){
    
    if( abs(pixels[iPix]->m_row - neighbour->m_row) <= 1 &&
       abs(pixels[iPix]->m_column - neighbour->m_column) <= 1 ){
      Touching = true;
      break;
    }
  }
  return Touching;
}

// Check if a pixel is close in time to the pixels of a cluster
bool Timepix3Clustering::closeInTime(Pixel* neighbour,Cluster* cluster){
  
  bool CloseInTime = false;
  
  Pixels pixels = cluster->pixels();
  for(int iPix=0;iPix<pixels.size();iPix++){

    long long int timeDifference = abs(neighbour->m_timestamp - pixels[iPix]->m_timestamp);
    if(timeDifference < timingCutInt) CloseInTime = true;
    
  }
  return CloseInTime;
}


void Timepix3Clustering::calculateClusterCentre(Cluster* cluster){
  
  // Empty variables to calculate cluster position
  double row(0), column(0), tot(0);
  long long int timestamp;

	// Get the pixels on this cluster
  Pixels pixels = cluster->pixels();
  string detectorID = pixels[0]->m_detectorID;
  timestamp = pixels[0]->m_timestamp;
  
  // Loop over all pixels
  for(int pix=0; pix<pixels.size(); pix++){
    tot += pixels[pix]->m_adc;
    row += (pixels[pix]->m_row * pixels[pix]->m_adc);
    column += (pixels[pix]->m_column * pixels[pix]->m_adc);
    if(pixels[pix]->m_timestamp < timestamp) timestamp = pixels[pix]->m_timestamp;
  }
  // Row and column positions are tot-weighted
  row /= tot;
  column /= tot;
  
  // Create object with local cluster position
  PositionVector3D<Cartesian3D<double> > positionLocal(parameters->detector[detectorID]->pitchX() *
                                                       (column-parameters->detector[detectorID]->nPixelsX()/2),
                                                       parameters->detector[detectorID]->pitchY() *
                                                       (row-parameters->detector[detectorID]->nPixelsY()/2),
                                                       0);
  // Calculate global cluster position
  PositionVector3D<Cartesian3D<double> > positionGlobal = *(parameters->detector[detectorID]->m_localToGlobal) * positionLocal;
  
  // Set the cluster parameters
  cluster->setRow(row);
  cluster->setColumn(column);
  cluster->setTot(tot);
  cluster->setError(0.004);
  cluster->setTimestamp(timestamp);
  cluster->setDetectorID(detectorID);
  cluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(),positionGlobal.Z());
  cluster->setClusterCentreLocal(positionLocal.X(), positionLocal.Y(),positionLocal.Z());
 
}

void Timepix3Clustering::finalise(){
  
  
}
