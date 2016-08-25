#include "ClicpixAnalysis.h"
#include "Pixel.h"
#include "Cluster.h"
#include "Track.h"

ClicpixAnalysis::ClicpixAnalysis(bool debugging)
: Algorithm("ClicpixAnalysis"){
  debug = debugging;
  m_associationCut = 0.1; // 100 um
  m_proximityCut = 0.125; // 125 um
  timepix3Telescope = false;
}


void ClicpixAnalysis::initialise(Parameters* par){
 
  parameters = par;
  
  // Cluster/pixel histograms
  hHitPixels = new TH2F("hHitPixels","hHitPixels",64,0,64,64,0,64);
  hColumnHits = new TH1F("hColumnHits","hColumnHits",64,0,64);
  hRowHits = new TH1F("hRowHits","hRowHits",64,0,64);
  
  hClusterSizeAll = new TH1F("hClusterSizeAll","hClusterSizeAll",20,0,20);
  hClusterTOTAll = new TH1F("hClusterTOTAll","hClusterTOTAll",50,0,50);
  hClustersPerEvent = new TH1F("hClustersPerEvent","hClustersPerEvent",50,0,50);
  hClustersVersusEventNo = new TH1F("hClustersVersusEventNo","hClustersVersusEventNo",60000,0,60000);
  hGlobalClusterPositions = new TH2F("hGlobalClusterPositions","hGlobalClusterPositions",200,-2.0,2.0,300,-1.,2);
  
  hClusterWidthRow = new TH1F("hClusterWidthRow","hClusterWidthRow",20,0,20);
  hClusterWidthCol = new TH1F("hClusterWidthCol","hClusterWidthCol",20,0,20);
  
  // Track histograms
  hGlobalTrackDifferenceX = new TH1F("hGlobalTrackDifferenceX","hGlobalTrackDifferenceX",1000,-10.,10.);
  hGlobalTrackDifferenceY = new TH1F("hGlobalTrackDifferenceY","hGlobalTrackDifferenceY",1000,-10.,10.);
  
  hGlobalResidualsX = new TH1F("hGlobalResidualsX","hGlobalResidualsX",600,-0.3,0.3);
  hGlobalResidualsY = new TH1F("hGlobalResidualsY","hGlobalResidualsY",600,-0.3,0.3);
  hAbsoluteResiduals = new TH1F("hAbsoluteResiduals","hAbsoluteResiduals",600,0.,0.600);
  hGlobalResidualsXversusX = new TH2F("hGlobalResidualsXversusX","hGlobalResidualsXversusX",600,-3.,3.,600,-0.3,0.3);
  hGlobalResidualsXversusY = new TH2F("hGlobalResidualsXversusY","hGlobalResidualsXversusY",600,-3.,3.,600,-0.3,0.3);
  hGlobalResidualsYversusY = new TH2F("hGlobalResidualsYversusY","hGlobalResidualsYversusY",600,-3.,3.,600,-0.3,0.3);
  hGlobalResidualsYversusX = new TH2F("hGlobalResidualsYversusX","hGlobalResidualsYversusX",600,-3.,3.,600,-0.3,0.3);
  
  hGlobalResidualsXversusColWidth = new TH2F("hGlobalResidualsXversusColWidth","hGlobalResidualsXversusColWidth",30,0,30,600,-0.3,0.3);
  hGlobalResidualsXversusRowWidth = new TH2F("hGlobalResidualsXversusRowWidth","hGlobalResidualsXversusRowWidth",30,0,30,600,-0.3,0.3);
  hGlobalResidualsYversusColWidth = new TH2F("hGlobalResidualsYversusColWidth","hGlobalResidualsYversusColWidth",30,0,30,600,-0.3,0.3);
  hGlobalResidualsYversusRowWidth = new TH2F("hGlobalResidualsYversusRowWidth","hGlobalResidualsYversusRowWidth",30,0,30,600,-0.3,0.3);
  
  hAbsoluteResidualMap = new TH2F("hAbsoluteResidualMap","hAbsoluteResidualMap",50,0,50,25,0,25);
  hXresidualVersusYresidual = new TH2F("hXresidualVersusYresidual","hXresidualVersusYresidual",600,-0.3,0.3,600,-0.3,0.3);
  
  hAssociatedClustersPerEvent = new TH1F("hAssociatedClustersPerEvent","hAssociatedClustersPerEvent",50,0,50);;
  hAssociatedClustersVersusEventNo = new TH1F("hAssociatedClustersVersusEventNo","hAssociatedClustersVersusEventNo",60000,0,60000);
  hAssociatedClustersVersusTriggerNo = new TH1F("hAssociatedClustersVersusTriggerNo","hAssociatedClustersVersusTriggerNo",50,0,50);
  hAssociatedClusterRow = new TH1F("hAssociatedClusterRow","hAssociatedClusterRow",64,0,64);
  hAssociatedClusterColumn = new TH1F("hAssociatedClusterColumn","hAssociatedClusterColumn",64,0,64);
  hFrameEfficiency = new TH1F("hFrameEfficiency","hFrameEfficiency",6000,0,6000);
  hFrameTracks = new TH1F("hFrameTracks","hFrameTracks",6000,0,6000);
  hFrameTracksAssociated = new TH1F("hFrameTracksAssociated","hFrameTracksAssociated",6000,0,6000);
  
  hClusterSizeAssociated = new TH1F("hClusterSizeAssociated","hClusterSizeAssociated",20,0,20);
  hClusterWidthRowAssociated = new TH1F("hClusterWidthRowAssociated","hClusterWidthRowAssociated",20,0,20);
  hClusterWidthColAssociated = new TH1F("hClusterWidthColAssociated","hClusterWidthColAssociated",20,0,20);
  
  hClusterTOTAssociated = new TH1F("hClusterTOTAssociated","hClusterTOTAssociated",50,0,50);
  hClusterTOTAssociated1pix = new TH1F("hClusterTOTAssociated1pix","hClusterTOTAssociated1pix",50,0,50);
  hClusterTOTAssociated2pix = new TH1F("hClusterTOTAssociated2pix","hClusterTOTAssociated2pix",50,0,50);
  hClusterTOTAssociated3pix = new TH1F("hClusterTOTAssociated3pix","hClusterTOTAssociated3pix",50,0,50);
  hClusterTOTAssociated4pix = new TH1F("hClusterTOTAssociated4pix","hClusterTOTAssociated4pix",50,0,50);
  
  hPixelResponseX = new TH1F("hPixelResponseX","hPixelResponseX",600,-0.3,0.3);
  hPixelResponseY = new TH1F("hPixelResponseY","hPixelResponseY",600,-0.3,0.3);
  
  hPixelResponseGlobalX = new TH1F("hPixelResponseGlobalX","hPixelResponseGlobalX",600,-0.3,0.3);
  hPixelResponseGlobalY = new TH1F("hPixelResponseGlobalY","hPixelResponseGlobalY",600,-0.3,0.3);
  
  hPixelResponseXOddCol = new TH1F("hPixelResponseXOddCol","hPixelResponseXOddCol",600,-0.3,0.3);
  hPixelResponseXEvenCol = new TH1F("hPixelResponseXEvenCol","hPixelResponseXEvenCol",600,-0.3,0.3);
  hPixelResponseYOddCol = new TH1F("hPixelResponseYOddCol","hPixelResponseYOddCol",600,-0.3,0.3);
  hPixelResponseYEvenCol = new TH1F("hPixelResponseYEvenCol","hPixelResponseYEvenCol",600,-0.3,0.3);
  
  hEtaDistributionX = new TH2F("hEtaDistributionX","hEtaDistributionX",25,0,25,25,0,25);
  hEtaDistributionY = new TH2F("hEtaDistributionY","hEtaDistributionY",25,0,25,25,0,25);
  
  hResidualsLocalRow2pix = new TH1F("hResidualsLocalRow2pix","hResidualsLocalRow2pix",600,-0.3,0.3);
  hResidualsLocalCol2pix = new TH1F("hResidualsLocalCol2pix","hResidualsLocalCol2pix",600,-0.3,0.3);
  hClusterTOTRow2pix = new TH1F("hClusterTOTRow2pix","hClusterTOTRow2pix",50,0,50);
  hClusterTOTCol2pix = new TH1F("hClusterTOTCol2pix","hClusterTOTCol2pix",50,0,50);
  hClusterTOTRatioRow2pix = new TH1F("hClusterTOTRatioRow2pix","hClusterTOTRatioRow2pix",100,0,1);
  hClusterTOTRatioCol2pix = new TH1F("hClusterTOTRatioCol2pix","hClusterTOTRatioCol2pix",100,0,1);
  hPixelTOTRow2pix = new TH1F("hPixelTOTRow2pix","hPixelTOTRow2pix",50,0,50);
  hPixelTOTCol2pix = new TH1F("hPixelTOTCol2pix","hPixelTOTCol2pix",50,0,50);
  
  hResidualsLocalRow2pixClusterTOT = new TH2F("hResidualsLocalRow2pixClusterTOT","hResidualsLocalRow2pixClusterTOT",50,0,50,600,-0.3,0.3);
  hResidualsLocalRow2pixPixelIntercept = new TH2F("hResidualsLocalRow2pixPixelIntercept","hResidualsLocalRow2pixPixelIntercept",50,0,25,600,-0.3,0.3);
  
  // Maps
  hTrackIntercepts = new TH2F("hTrackIntercepts","hTrackIntercepts",200,-2.0,2.0,300,-1.,2);
  hTrackInterceptsAssociated = new TH2F("hTrackInterceptsAssociated","hTrackInterceptsAssociated",200,-2.0,2.0,300,-1.,2);
  hGlobalAssociatedClusterPositions = new TH2F("hGlobalAssociatedClusterPositions","hGlobalAssociatedClusterPositions",200,-2.0,2.0,300,-1.,2);
  hTrackInterceptsPixel = new TH2F("hTrackInterceptsPixel","hTrackInterceptsPixel",50,0,50,25,0,25);
  hTrackInterceptsPixelAssociated = new TH2F("hTrackInterceptsPixelAssociated","hTrackInterceptsPixelAssociated",50,0,50,25,0,25);
  hTrackInterceptsChip = new TH2F("hTrackInterceptsChip","hTrackInterceptsChip",65,-0.5,64.5,65,-0.5,64.5);
  hTrackInterceptsChipAssociated = new TH2F("hTrackInterceptsChipAssociated","hTrackInterceptsChipAssociated",65,-0.5,64.5,65,-0.5,64.5);
  hTrackInterceptsChipLost = new TH2F("hTrackInterceptsChipLost","hTrackInterceptsChipLost",65,-0.5,64.5,65,-0.5,64.5);
  
  hPixelEfficiencyMap = new TH2F("hPixelEfficiencyMap","hPixelEfficiencyMap",50,0,50,25,0,25);
  hChipEfficiencyMap = new TH2F("hChipEfficiencyMap","hChipEfficiencyMap",65,-0.5,64.5,65,-0.5,64.5);
  hGlobalEfficiencyMap = new TH2F("hGlobalEfficiencyMap","hGlobalEfficiencyMap",200,-2.0,2.0,300,-1.,2);
  
  hInterceptClusterSize1 = new TH2F("hInterceptClusterSize1","hInterceptClusterSize1",50,0,50,25,0,25);
  hInterceptClusterSize2 = new TH2F("hInterceptClusterSize2","hInterceptClusterSize2",50,0,50,25,0,25);
  hInterceptClusterSize3 = new TH2F("hInterceptClusterSize3","hInterceptClusterSize3",50,0,50,25,0,25);
  hInterceptClusterSize4 = new TH2F("hInterceptClusterSize4","hInterceptClusterSize4",50,0,50,25,0,25);
  
  // Initialise member variables
  m_eventNumber = 0; m_triggerNumber = 0; dutID = parameters->DUT; m_lostHits = 0.;

}

StatusCode ClicpixAnalysis::run(Clipboard* clipboard){
  
	// Get the tracks in this event
  Tracks* tracks = (Tracks*)clipboard->get("tracks");
  if(tracks == NULL){
    if(debug) tcout<<"No tracks on the clipboard"<<endl;
    return Success;
  }

  // Get the clicpix clusters in this event
  Clusters* clusters = (Clusters*)clipboard->get(dutID,"clusters");
  if(clusters == NULL){
    if(debug) tcout<<"No clusters for "<<dutID<<" on the clipboard"<<endl;
    return Success;
  }
  
  // If this is the first or last trigger don't use the data
  if( (m_triggerNumber == 0 || m_triggerNumber == 19) && !timepix3Telescope){
    m_eventNumber++;
    m_triggerNumber++;
    return Success;
  }
  
  // Fill the histograms that only need clusters/pixels
  fillClusterHistos(clusters);
  
  //Set counters
  double nClustersAssociated=0, nValidTracks=0;
  
  // Loop over tracks, and compare with Clicpix clusters
  Tracks::iterator itTrack;
  for (itTrack = tracks->begin(); itTrack != tracks->end(); itTrack++) {
    
    // Get the track
    Track* track = (*itTrack);
    if (!track) continue;
    
    // Get the track intercept with the clicpix plane (global and local co-ordinates)
    PositionVector3D< Cartesian3D<double> > trackIntercept = parameters->detector[dutID]->getIntercept(track);
    PositionVector3D< Cartesian3D<double> > trackInterceptLocal = *(parameters->detector[dutID]->m_globalToLocal) * trackIntercept;
    
    // Plot the difference between track intercepts and all clicpix clusters
    // Also record which cluster is the closest
    Clusters::iterator itCorrelate;
    bool matched=false; Clusters::iterator bestCluster;
    double xresidualBest = 10000.; double yresidualBest = 10000.;
    double absoluteresidualBest = sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest);

    for (itCorrelate = clusters->begin(); itCorrelate != clusters->end(); ++itCorrelate) {
     
      // Get the distance between this cluster and the track intercept (global)
      double xcorr = (*itCorrelate)->globalX()-trackIntercept.X();
      double ycorr = (*itCorrelate)->globalY()-trackIntercept.Y();
      
      // Fill histograms on correlations
      hGlobalTrackDifferenceX->Fill(xcorr);
      hGlobalTrackDifferenceY->Fill(ycorr);
      
      // Look if this cluster can be considered associated.
      // Cut on the distance from the track in x and y
      if ( fabs(ycorr) > m_associationCut || fabs(xcorr) > m_associationCut) continue;
      
      // Found a matching cluster
      matched=true;
      
      // Check if this is a better match than the previously found cluster, and store the information
      double absoluteresidual = sqrt(xcorr*xcorr + ycorr*ycorr);
      if( absoluteresidual <= absoluteresidualBest){
        absoluteresidualBest=absoluteresidual;
        xresidualBest=xcorr;
        yresidualBest=ycorr;
        bestCluster=itCorrelate;
      }

    }
  
    // Get the track intercept position along the chip
    double chipInterceptCol = parameters->detector[dutID]->getColumn(trackInterceptLocal);
    double chipInterceptRow = parameters->detector[dutID]->getRow(trackInterceptLocal);

    // Get the track intercept position along the pixel
    double pixelInterceptX = parameters->detector[dutID]->inPixelX(trackInterceptLocal);
    double pixelInterceptY = parameters->detector[dutID]->inPixelY(trackInterceptLocal);
    
    // Cut on the track intercept - this makes sure that it actually went through the chip
    if( chipInterceptCol < 0.5 ||
       chipInterceptRow < 0.5 ||
       chipInterceptCol > (parameters->detector[dutID]->nPixelsX()-0.5) ||
       chipInterceptRow > (parameters->detector[dutID]->nPixelsY()-0.5) ) continue;
    
    // Check if the hit is near a masked pixel
    bool hitMasked = checkMasked(chipInterceptRow,chipInterceptCol);
    if(hitMasked) continue;
    
    // Check there are no other tracks nearby
    bool proximityCut = checkProximity(track,tracks);
    if(proximityCut) continue;
    
    // Now have tracks that went through the device
    hTrackIntercepts->Fill(trackIntercept.X(),trackIntercept.Y());
    hFrameTracks->Fill(m_eventNumber);
    nValidTracks++;
    hTrackInterceptsChip->Fill(chipInterceptCol,chipInterceptRow);
    hTrackInterceptsPixel->Fill(pixelInterceptX,pixelInterceptY);
    
    if(matched){
      
      // Some test plots of pixel response function
      fillResponseHistos(trackIntercept.X(),trackIntercept.Y(),(*bestCluster));
      
      // Add this cluster to the list of associated clusters held by this track. This will allow alignment to take place
      track->addAssociatedCluster(*bestCluster);
      
      // Now fill all of the histograms/efficiency counters that we want
      hTrackInterceptsAssociated->Fill(trackIntercept.X(),trackIntercept.Y());
      hGlobalResidualsX->Fill(xresidualBest);
      hGlobalResidualsY->Fill(yresidualBest);
      hGlobalAssociatedClusterPositions->Fill((*bestCluster)->globalX(),(*bestCluster)->globalY());
      nClustersAssociated++;
      hAssociatedClusterRow->Fill((*bestCluster)->row());
      hAssociatedClusterColumn->Fill((*bestCluster)->column());
      hTrackInterceptsChipAssociated->Fill(chipInterceptCol,chipInterceptRow);
      hTrackInterceptsPixelAssociated->Fill(pixelInterceptX,pixelInterceptY);
      hFrameTracksAssociated->Fill(m_eventNumber);
      hGlobalResidualsXversusX->Fill(trackIntercept.X(),xresidualBest);
      hGlobalResidualsXversusY->Fill(trackIntercept.Y(),xresidualBest);
      hGlobalResidualsYversusY->Fill(trackIntercept.Y(),yresidualBest);
      hGlobalResidualsYversusX->Fill(trackIntercept.X(),yresidualBest);
//      hGlobalResidualsXversusColWidth->Fill((*bestCluster)->colWidth(),xresidualBest);
//      hGlobalResidualsXversusRowWidth->Fill((*bestCluster)->rowWidth(),xresidualBest);
//      hGlobalResidualsYversusColWidth->Fill((*bestCluster)->colWidth(),yresidualBest);
//      hGlobalResidualsYversusRowWidth->Fill((*bestCluster)->rowWidth(),yresidualBest);
      hAbsoluteResidualMap->Fill(pixelInterceptX,pixelInterceptY,sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest));
      hXresidualVersusYresidual->Fill(xresidualBest,yresidualBest);
      hAbsoluteResiduals->Fill(sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest));
      hClusterTOTAssociated->Fill((*bestCluster)->tot());
      hClusterSizeAssociated->Fill((*bestCluster)->size());
//      hClusterWidthColAssociated->Fill((*bestCluster)->colWidth());
//      hClusterWidthRowAssociated->Fill((*bestCluster)->rowWidth());
      
      if((*bestCluster)->size() == 1){
        hClusterTOTAssociated1pix->Fill((*bestCluster)->tot());
        hInterceptClusterSize1->Fill(pixelInterceptX,pixelInterceptY);
      }
      if((*bestCluster)->size() == 2){
        hClusterTOTAssociated2pix->Fill((*bestCluster)->tot());
        hInterceptClusterSize2->Fill(pixelInterceptX,pixelInterceptY);
      }
      if((*bestCluster)->size() == 3){
        hClusterTOTAssociated3pix->Fill((*bestCluster)->tot());
        hInterceptClusterSize3->Fill(pixelInterceptX,pixelInterceptY);
      }
      if((*bestCluster)->size() == 4){
        hClusterTOTAssociated4pix->Fill((*bestCluster)->tot());
        hInterceptClusterSize4->Fill(pixelInterceptX,pixelInterceptY);
      }
    }else{
      
      // Search for lost hits. Basically loop through all pixels in all clusters and see if any are close.
      // Large clusters (such as from deltas) can pull the cluster centre sufficiently far from the track
      bool pixelMatch = false; int size=0;
      for (itCorrelate = clusters->begin(); itCorrelate != clusters->end(); ++itCorrelate) {
        if(pixelMatch) break;
        // Loop over pixels
        Pixels::const_iterator itPixel;
        for (itPixel = (*itCorrelate)->pixels().begin(); itPixel != (*itCorrelate)->pixels().end(); itPixel++) {
          
          // Get the pixel global position
          PositionVector3D<Cartesian3D<double> > pixelPositionLocal = parameters->detector[dutID]->getLocalPosition((*itPixel)->m_row,(*itPixel)->m_column);
          PositionVector3D<Cartesian3D<double> > pixelPositionGlobal = *(parameters->detector[dutID]->m_localToGlobal) * pixelPositionLocal;
          
          // Check if it is close to the track
          if( fabs( pixelPositionGlobal.X() - trackIntercept.X() ) > m_associationCut ||
              fabs( pixelPositionGlobal.Y() - trackIntercept.Y() ) > m_associationCut ) continue;
          
          pixelMatch=true;
          break;
        }
      }
      // Increment counter for number of hits found this way
      if(pixelMatch){
        m_lostHits++;
        hTrackInterceptsChipLost->Fill(chipInterceptCol,chipInterceptRow);
      }
    }
  }
  
  // Histograms relating to this mimosa frame
  hAssociatedClustersPerEvent->Fill(nClustersAssociated);
  hAssociatedClustersVersusEventNo->Fill(m_eventNumber,nClustersAssociated);
  hAssociatedClustersVersusTriggerNo->Fill(m_triggerNumber,nClustersAssociated);
  if(nValidTracks != 0) hFrameEfficiency->Fill(m_eventNumber,nClustersAssociated/nValidTracks);

	// Increment event counter and trigger number
  m_eventNumber++; m_triggerNumber++;
  
  // Return value telling analysis to keep running
  return Success;
}

void ClicpixAnalysis::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
  // Compare number of valid tracks, and number of clusters associated in order to get global efficiency
  double nTracks = hTrackIntercepts->GetEntries();
  double nClusters = hGlobalAssociatedClusterPositions->GetEntries();
  double efficiency = nClusters/nTracks;
  double errorEfficiency = (1./nTracks) * sqrt(nClusters*(1-efficiency));
  
  if(nTracks == 0){ efficiency = 0.; errorEfficiency = 1.;}
  
  tcout<<"***** Clicpix efficiency calculation *****"<<endl;
  tcout<<"***** ntracks: "<<(int)nTracks<<", nclusters "<<(int)nClusters<<endl;
  tcout<<"***** Efficiency: "<<100.*efficiency<<" +/- "<<100.*errorEfficiency<<" %"<<endl;
  tcout<<"***** If including the "<<(int)m_lostHits<<" lost pixel hits, this becomes "<<100.*(m_lostHits+nClusters)/nTracks<<" %"<<endl;
  tcout<<endl;

}

// Check if a track has gone through or near a masked pixel
bool ClicpixAnalysis::checkMasked(double chipInterceptRow, double chipInterceptCol){

  // Get the pixel row and column number
  int rowNumber = ceil(chipInterceptRow);
  int colNumber = ceil(chipInterceptCol);
  
  // Check if that pixel is masked, or the neighbour to the left, right, etc...
  if(parameters->detector[dutID]->masked(colNumber-1,rowNumber-1)) return true;
  if(parameters->detector[dutID]->masked(colNumber,rowNumber-1)) return true;
  if(parameters->detector[dutID]->masked(colNumber+1,rowNumber-1)) return true;
  if(parameters->detector[dutID]->masked(colNumber-1,rowNumber)) return true;
  if(parameters->detector[dutID]->masked(colNumber,rowNumber)) return true;
  if(parameters->detector[dutID]->masked(colNumber+1,rowNumber)) return true;
  if(parameters->detector[dutID]->masked(colNumber-1,rowNumber+1)) return true;
  if(parameters->detector[dutID]->masked(colNumber,rowNumber+1)) return true;
  if(parameters->detector[dutID]->masked(colNumber+1,rowNumber+1)) return true;

  // If not masked
  return false;
}

// Check if there is another track close to the selected track.
// "Close" is defined as the intercept at the clicpix
bool ClicpixAnalysis::checkProximity(Track* track, Tracks* tracks){
  
  // Get the intercept of the interested track at the dut
  bool close = false;
  PositionVector3D< Cartesian3D<double> > trackIntercept = parameters->detector[dutID]->getIntercept(track);
  
  // Loop over all other tracks and check if they intercept close to the track we are considering
  Tracks::iterator itTrack;
  for (itTrack = tracks->begin(); itTrack != tracks->end(); itTrack++) {
    
    // Get the track
    Track* track2 = (*itTrack);
    // Get the track intercept with the clicpix plane (global co-ordinates)
    PositionVector3D< Cartesian3D<double> > trackIntercept2 = parameters->detector[dutID]->getIntercept(track2);
    // If track == track2 do nothing
    if(trackIntercept.X() == trackIntercept2.X() && trackIntercept.Y() == trackIntercept2.Y()) continue;
    if( fabs(trackIntercept.X() - trackIntercept2.X()) <= m_proximityCut ||
       fabs(trackIntercept.Y() - trackIntercept2.Y()) <= m_proximityCut ) close = true;
    
  }
  return close;
}

// Small sub-routine to fill histograms that only need clusters
void ClicpixAnalysis::fillClusterHistos(Clusters* clusters){
  
	// Pick up column to generate unique pixel id
  int nCols = parameters->detector[dutID]->nPixelsX();
  Clusters::iterator itc;
  
  // Check if this is a new clicpix frame (each frame may be in several events) and
  // fill histograms
  bool newFrame = false;
  for (itc = clusters->begin(); itc != clusters->end(); ++itc) {

    // Loop over pixels and check if there are pixels not known
    Pixels pixels = (*itc)->pixels();
    Pixels::iterator itp;
    for (itp = pixels.begin(); itp != pixels.end(); itp++) {
      // Check if this clicpix frame is still the current
      int pixelID = (*itp)->m_column + nCols*(*itp)->m_row;
      if( m_hitPixels[pixelID] != (*itp)->m_adc){
        // New frame! Reset the stored pixels and trigger number
        if(!newFrame){m_hitPixels.clear(); newFrame=true;}
        m_hitPixels[pixelID] = (*itp)->m_adc;
        m_triggerNumber=0;
      }
      hHitPixels->Fill((*itp)->m_column,(*itp)->m_row);
      hColumnHits->Fill((*itp)->m_column);
      hRowHits->Fill((*itp)->m_row);
    }
    
    // Fill cluster histograms
    hClusterSizeAll->Fill((*itc)->size());
    hClusterTOTAll->Fill((*itc)->tot());
    hGlobalClusterPositions->Fill((*itc)->globalX(),(*itc)->globalY());
    
  }
  
  hClustersPerEvent->Fill(clusters->size());
  hClustersVersusEventNo->Fill(m_eventNumber,clusters->size());
  
  return;
}

// Sub-routine to look at pixel response, ie. how far from the pixel is the track intercept for the pixel to still see charge
void ClicpixAnalysis::fillResponseHistos(double trackInterceptX, double trackInterceptY, Cluster* cluster){
  
  // Loop over pixels in the cluster and show their distance from the track intercept
  Pixels pixels = cluster->pixels();
  Pixels::iterator itp;
  for (itp = pixels.begin(); itp != pixels.end(); itp++) {
    
    // Get the pixel
    Pixel* pixel = (*itp);
    // Get the pixel local then global position
    PositionVector3D<Cartesian3D<double> > pixelPositionLocal = parameters->detector[dutID]->getLocalPosition(pixel->m_row,pixel->m_column);
    PositionVector3D<Cartesian3D<double> > pixelPositionGlobal = *(parameters->detector[dutID]->m_localToGlobal) * pixelPositionLocal;

    // Fill the response histograms
    hPixelResponseX->Fill( pixelPositionGlobal.X() - trackInterceptX );
    hPixelResponseY->Fill( pixelPositionGlobal.Y() - trackInterceptY );
    
  }
  
  return;
}


