// Include files
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TH1F.h"

// local
#include "Clipboard.h"
#include "ClicpixAnalysis.h"
#include "TestBeamCluster.h"
#include "TestBeamTransform.h"
#include "SystemOfUnits.h"
#include "PhysicalConstants.h"

using namespace std;

//-----------------------------------------------------------------------------
// Implementation file for class : Clicpix Analysis
//
// 2014-10-21 : Daniel Hynds
//
//-----------------------------------------------------------------------------

ClicpixAnalysis::ClicpixAnalysis(Parameters* p, bool d)
: Algorithm("ClicpixAnalysis") {
	
	parameters = p;
	display = d;
	TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
	TTree* tbtree = (TTree*)inputFile->Get("tbtree");
	summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
	m_debug = false;
	
}

std::string ClicpixAnalysis::makestring(int number) {
	std::ostringstream ss;
	ss << number;
	return ss.str();
}

void ClicpixAnalysis::initial()
{
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
	hGlobalResidualsXversusX = new TH2F("hGlobalResidualsXversusX","hGlobalResidualsXversusX",500,-5.,5.,600,-0.3,0.3);
	hGlobalResidualsXversusY = new TH2F("hGlobalResidualsXversusY","hGlobalResidualsXversusY",500,-5.,5.,600,-0.3,0.3);
	hGlobalResidualsYversusY = new TH2F("hGlobalResidualsYversusY","hGlobalResidualsYversusY",500,-5.,5.,600,-0.3,0.3);
	hGlobalResidualsYversusX = new TH2F("hGlobalResidualsYversusX","hGlobalResidualsYversusX",500,-5.,5.,600,-0.3,0.3);

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
	
	hClusterTOTAssociated = new TH1F("hClusterTOTAssociated","hClusterTOTAssociated",50,0,50);
	hClusterTOTAssociated1pix = new TH1F("hClusterTOTAssociated1pix","hClusterTOTAssociated1pix",50,0,50);
	hClusterTOTAssociated2pix = new TH1F("hClusterTOTAssociated2pix","hClusterTOTAssociated2pix",50,0,50);
	hClusterTOTAssociated3pix = new TH1F("hClusterTOTAssociated3pix","hClusterTOTAssociated3pix",50,0,50);
	hClusterTOTAssociated4pix = new TH1F("hClusterTOTAssociated4pix","hClusterTOTAssociated4pix",50,0,50);
	
	hPixelResponseX = new TH1F("hPixelResponseX","hPixelResponseX",600,-0.3,0.3);
	hPixelResponseY = new TH1F("hPixelResponseY","hPixelResponseY",600,-0.3,0.3);
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
	hTrackInterceptsChip = new TH2F("hTrackInterceptsChip","hTrackInterceptsChip",66,-1,65,66,-1,65);
	hTrackInterceptsChipAssociated = new TH2F("hTrackInterceptsChipAssociated","hTrackInterceptsChipAssociated",66,-1,65,66,-1,65);
	hTrackInterceptsChipLost = new TH2F("hTrackInterceptsChipLost","hTrackInterceptsChipLost",66,-1,65,66,-1,65);

	hPixelEfficiencyMap = new TH2F("hPixelEfficiencyMap","hPixelEfficiencyMap",50,0,50,25,0,25);
	hChipEfficiencyMap = new TH2F("hChipEfficiencyMap","hChipEfficiencyMap",66,-1,65,66,-1,65);
	hGlobalEfficiencyMap = new TH2F("hGlobalEfficiencyMap","hGlobalEfficiencyMap",200,-2.0,2.0,300,-1.,2);

	hInterceptClusterSize1 = new TH2F("hInterceptClusterSize1","hInterceptClusterSize1",50,0,50,25,0,25);
	hInterceptClusterSize2 = new TH2F("hInterceptClusterSize2","hInterceptClusterSize2",50,0,50,25,0,25);
	hInterceptClusterSize3 = new TH2F("hInterceptClusterSize3","hInterceptClusterSize3",50,0,50,25,0,25);
	hInterceptClusterSize4 = new TH2F("hInterceptClusterSize4","hInterceptClusterSize4",50,0,50,25,0,25);
	
	// Variables
	eventnumber=0;
	m_triggerNumber=0;
	m_lostHits=0;
	m_totMapBinsX=32;
	m_totMapBinsY=32;
	
	// Make the histogrmas to map the ToT (global xy map)
	for(int x=0;x<m_totMapBinsX;x++){
		for(int y=0;y<m_totMapBinsY;y++){
			int id = x+y*m_totMapBinsX;
			std::string name = "hClusterTOTAssociated1pixMap"+makestring(id);
			TH1F* hClusterTOTAssociated1pixMap = new TH1F(name.c_str(),name.c_str(),50,0,50);
			totMap[id] = hClusterTOTAssociated1pixMap;
		}
	}

  // Make the histogrmas to map the ToT (per pixel)
  for(int xpix=0;xpix<64;xpix++){
    for(int ypix=0;ypix<64;ypix++){
      int id = xpix+ypix*64;
      std::string name = "hClusterTOTAssociated1pixPixel"+makestring(id);
      TH1F* hClusterTOTAssociated1pixPixel = new TH1F(name.c_str(),name.c_str(),50,0,50);
      totMapPerPixel[id] = hClusterTOTAssociated1pixPixel;
    }
  }
  
  
  m_responseBins=100;
  m_responseWidth=0.05; 
  // Make the histogrmas to map the ToT (pixel response)
  for(int responseID=0;responseID<m_responseBins;responseID++){
    std::string name = "hClusterTOTAssociated1pixResponseX"+makestring(responseID);
    TH1F* hClusterTOTAssociated1pixResponseX = new TH1F(name.c_str(),name.c_str(),50,0,50);
    totMapPixelResponseX[responseID] = hClusterTOTAssociated1pixResponseX;
    std::string name2 = "hClusterTOTAssociated1pixResponseY"+makestring(responseID);
    TH1F* hClusterTOTAssociated1pixResponseY = new TH1F(name2.c_str(),name2.c_str(),50,0,50);
    totMapPixelResponseY[responseID] = hClusterTOTAssociated1pixResponseY;
  }

  
	
																							
}

void ClicpixAnalysis::run(TestBeamEvent *event, Clipboard *clipboard)
{
	
	// Pick up tracks
	std::vector<TestBeamTrack*>* tracks = (TestBeamTracks*)clipboard->get("Tracks");
	if (!tracks) {
		std::cerr << m_name << std::endl;
		std::cerr << "    no clusters on clipboard" << std::endl;
		return;
	}
	
	// Pick up all clusters
	TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
	if (!clusters) {
		std::cerr << m_name << std::endl;
		std::cerr << "    no clusters on clipboard" << std::endl;
		return;
	}
	
	// Fill the histograms that only need clusters/pixels
	fillClusterHistos(clusters);
	
	// If this is the first or last trigger don't use the data (trigger number set in fillClusterHistos routine)
	if( m_triggerNumber == 0 || m_triggerNumber == 19){
		eventnumber++;
		m_triggerNumber++;
		return;
	}
	
	// Grab the limits on x and y distance for clusters to tracks
	float residualmaxx = parameters->residualmaxx;
	float residualmaxy = parameters->residualmaxy;
	
	//Set counters
	double nClustersAssociated=0; double nValidTracks=0;
	
	// Loop over tracks, and compare with Clicpix clusters
	TestBeamTracks::iterator it;
	for (it = tracks->begin(); it != tracks->end(); ++it) {
		// Got the track
		TestBeamTrack* track = (*it);
		if (!track) continue;
		
		// Get the track intercept with the clicpix plane (global co-ordinates)
		PositionVector3D< Cartesian3D<double> > trackIntercept = getTrackIntercept(track,"CLi-CPix",true);
		
		// Plot the difference between track intercepts and all clicpix clusters
		TestBeamClusters::iterator itCorrelate;
		for (itCorrelate = clusters->begin(); itCorrelate != clusters->end(); ++itCorrelate) {
			std::string chip = (*itCorrelate)->detectorId();
			// Only look at the clicpix
			if(chip != "CLi-CPix") continue;
			// Get the distance between this cluster and the track intercept (global)
			double xcorr = (*itCorrelate)->globalX()-trackIntercept.X();
			double ycorr = (*itCorrelate)->globalY()-trackIntercept.Y();
			hGlobalTrackDifferenceX->Fill(xcorr);
			hGlobalTrackDifferenceY->Fill(ycorr);
		}
		
		
		// Get the local co-ordinates of the track, both within the chip and within the pixel
		PositionVector3D< Cartesian3D<double> > trackInterceptLocal = getTrackIntercept(track,"CLi-CPix",false);
		double chipInterceptCol = ((trackInterceptLocal.X() + parameters->pixelPitchX["CLi-CPix"]/2.) /
															 parameters->pixelPitchX["CLi-CPix"]) + (parameters->nPixelsX["CLi-CPix"]/2);
		double chipInterceptRow = ((trackInterceptLocal.Y() + parameters->pixelPitchY["CLi-CPix"]/2.) /
															 parameters->pixelPitchY["CLi-CPix"]) + (parameters->nPixelsY["CLi-CPix"]/2);
		double pixelInterceptX = 1000.*(parameters->pixelPitchX["CLi-CPix"] * (chipInterceptCol - 2.*floor(chipInterceptCol/2.)) );
		double pixelInterceptY = 1000.*(parameters->pixelPitchY["CLi-CPix"] * (chipInterceptRow - 1.*floor(chipInterceptRow/1.)) );
	
		// Cut on the track intercept - this makes sure that it actually went through the chip
		if( chipInterceptCol < 0.5 ||
			  chipInterceptRow < 0.5 ||
			  chipInterceptCol > (parameters->nPixelsX["CLi-CPix"]-0.5) ||
				chipInterceptRow > (parameters->nPixelsY["CLi-CPix"]-0.5) ) continue;
		
		// Check if the hit is near a masked pixel
		bool hitMasked = checkMasked(chipInterceptRow,chipInterceptCol);
		if(hitMasked) continue;
		
		// Check there are no other tracks nearby
		bool proximityCut = checkProximity(track,tracks);
		if(proximityCut) continue;
		
		// Now have tracks that went through the device
		hTrackIntercepts->Fill(trackIntercept.X(),trackIntercept.Y());
		hFrameTracks->Fill(eventnumber);
		nValidTracks++;
		hTrackInterceptsChip->Fill(chipInterceptCol,chipInterceptRow);
		hTrackInterceptsPixel->Fill(pixelInterceptX,pixelInterceptY);
		
		bool matched=false;
		double xresidualBest = residualmaxx; double yresidualBest = residualmaxy;
		double absoluteresidualBest = sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest);
		
		// Loop over clusters
		TestBeamClusters::iterator itc,itcMatched;
		for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
			if (!(*itc)) continue; //if(matched) continue;
			std::string chip = (*itc)->detectorId();
			// Only look at the clicpix
			if(chip != "CLi-CPix") continue;
			
			// Get the residuals
			double xresidual = (*itc)->globalX()-trackIntercept.X();
			double yresidual = (*itc)->globalY()-trackIntercept.Y();
			double absoluteresidual = sqrt(xresidual*xresidual + yresidual*yresidual);
		
			// Cut on the distance from the track in x and y (association)
			if ( fabs(yresidual) > residualmaxy || fabs(xresidual) > residualmaxx) continue;
			
			// Found a matching cluster
			matched=true;
			// Check if this is a better match than the previously found cluster
			if( absoluteresidual <= absoluteresidualBest){
	
				absoluteresidualBest=absoluteresidual;
				xresidualBest=xresidual;
				yresidualBest=yresidual;
				itcMatched=itc;
				
			}
		}
		
		if(matched){
			
			xresidualBest = (*itcMatched)->globalX()-trackIntercept.X();
			yresidualBest = (*itcMatched)->globalY()-trackIntercept.Y();
			
			// Some test plots of pixel response function
			fillResponseHistos(chipInterceptRow,chipInterceptCol,(*itcMatched));
			
			// Now fill all of the histograms/efficiency counters that we want
			hTrackInterceptsAssociated->Fill(trackIntercept.X(),trackIntercept.Y());
			hGlobalResidualsX->Fill(xresidualBest);
			hGlobalResidualsY->Fill(yresidualBest);
			hGlobalAssociatedClusterPositions->Fill((*itcMatched)->globalX(),(*itcMatched)->globalY());
			nClustersAssociated++;
			hAssociatedClusterRow->Fill((*itcMatched)->rowPosition());
			hAssociatedClusterColumn->Fill((*itcMatched)->colPosition());
			hTrackInterceptsChipAssociated->Fill(chipInterceptCol,chipInterceptRow);
			hTrackInterceptsPixelAssociated->Fill(pixelInterceptX,pixelInterceptY);
			hFrameTracksAssociated->Fill(eventnumber);
			hGlobalResidualsXversusX->Fill(trackIntercept.X(),xresidualBest);
			hGlobalResidualsXversusY->Fill(trackIntercept.Y(),xresidualBest);
			hGlobalResidualsYversusY->Fill(trackIntercept.Y(),yresidualBest);
			hGlobalResidualsYversusX->Fill(trackIntercept.X(),yresidualBest);
			hGlobalResidualsXversusColWidth->Fill((*itcMatched)->colWidth(),xresidualBest);
			hGlobalResidualsXversusRowWidth->Fill((*itcMatched)->rowWidth(),xresidualBest);
			hGlobalResidualsYversusColWidth->Fill((*itcMatched)->colWidth(),yresidualBest);
			hGlobalResidualsYversusRowWidth->Fill((*itcMatched)->rowWidth(),yresidualBest);
			hAbsoluteResidualMap->Fill(pixelInterceptX,pixelInterceptY,sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest));
			hXresidualVersusYresidual->Fill(xresidualBest,yresidualBest);
			hAbsoluteResiduals->Fill(sqrt(xresidualBest*xresidualBest + yresidualBest*yresidualBest));
			hClusterTOTAssociated->Fill((*itcMatched)->totalADC());
			
			if((*itcMatched)->size() == 1){
				hClusterTOTAssociated1pix->Fill((*itcMatched)->totalADC());
				hInterceptClusterSize1->Fill(pixelInterceptX,pixelInterceptY);
				
				int id = floor(chipInterceptCol*m_totMapBinsX/parameters->nPixelsX["CLi-CPix"])+floor(chipInterceptRow*m_totMapBinsY/parameters->nPixelsY["CLi-CPix"])*m_totMapBinsX;
				totMap[id]->Fill((*itcMatched)->totalADC());
				
        int pixID = (*itcMatched)->colPosition() + 64*(*itcMatched)->rowPosition();
        totMapPerPixel[pixID]->Fill((*itcMatched)->totalADC());
        
			}
			
			if((*itcMatched)->size() == 2){
				hClusterTOTAssociated2pix->Fill((*itcMatched)->totalADC());
				hInterceptClusterSize2->Fill(pixelInterceptX,pixelInterceptY);

				double trackColFrac = (trackInterceptLocal.X()/parameters->pixelPitchX["CLi-CPix"]) + (parameters->nPixelsX["CLi-CPix"]/2);
				double trackRowFrac = (trackInterceptLocal.Y()/parameters->pixelPitchY["CLi-CPix"]) + (parameters->nPixelsY["CLi-CPix"]/2);
				
				double etaTrackCol = 1000.*parameters->pixelPitchX["CLi-CPix"] * (trackColFrac - floor(trackColFrac)) ;
				double etaClusterCol = 1000.*parameters->pixelPitchX["CLi-CPix"]*((*itcMatched)->colPosition()-floor((*itcMatched)->colPosition()));
				double etaTrackRow = 1000.*parameters->pixelPitchY["CLi-CPix"] * (trackRowFrac - floor(trackRowFrac))  ;
				double etaClusterRow = 1000.*parameters->pixelPitchY["CLi-CPix"]*((*itcMatched)->rowPosition()-floor((*itcMatched)->rowPosition()));
				
				std::vector<RowColumnEntry*>::const_iterator ith2pix;
				double tot1,tot2; int hit=0;
				for (ith2pix = (*itcMatched)->hits()->begin(); ith2pix != (*itcMatched)->hits()->end(); ith2pix++) {
					hit++;
					if(hit==1) tot1 = (*ith2pix)->value();
					if(hit==2) tot2 = (*ith2pix)->value();
				}
				double ratio=0;
				if(tot1>=tot2) ratio = tot2/tot1;
				if(tot1<tot2) ratio = tot1/tot2;

				if((*itcMatched)->colWidth() == 2){
					hEtaDistributionX->Fill(etaTrackCol,etaClusterCol);
					hResidualsLocalCol2pix->Fill(parameters->pixelPitchX["CLi-CPix"]*(trackColFrac-(*itcMatched)->colPosition()));
					hClusterTOTCol2pix->Fill((*itcMatched)->totalADC());
					hClusterTOTRatioCol2pix->Fill(ratio);
					hPixelTOTCol2pix->Fill(tot1);
					hPixelTOTCol2pix->Fill(tot2);
				}
				if((*itcMatched)->rowWidth() == 2){
					hEtaDistributionY->Fill(etaTrackRow,etaClusterRow);
					hResidualsLocalRow2pix->Fill(parameters->pixelPitchY["CLi-CPix"]*(trackRowFrac-(*itcMatched)->rowPosition()));
					hClusterTOTRow2pix->Fill((*itcMatched)->totalADC());
					hClusterTOTRatioRow2pix->Fill(ratio);
					hPixelTOTRow2pix->Fill(tot1);
					hPixelTOTRow2pix->Fill(tot2);
					hResidualsLocalRow2pixClusterTOT->Fill((*itcMatched)->totalADC(),parameters->pixelPitchY["CLi-CPix"]*(trackRowFrac-(*itcMatched)->rowPosition()));
					hResidualsLocalRow2pixPixelIntercept->Fill(pixelInterceptY,parameters->pixelPitchY["CLi-CPix"]*(trackRowFrac-(*itcMatched)->rowPosition()));
				}

			}
			if((*itcMatched)->size() == 3){
				hClusterTOTAssociated3pix->Fill((*itcMatched)->totalADC());
				hInterceptClusterSize3->Fill(pixelInterceptX,pixelInterceptY);
			}
			if((*itcMatched)->size() == 4){
				hClusterTOTAssociated4pix->Fill((*itcMatched)->totalADC());
				hInterceptClusterSize4->Fill(pixelInterceptX,pixelInterceptY);
			}
		}else{

			// Search for lost hits. Basically loop through all pixels in all clusters and see if any are close
			bool pixelMatch = false; int size=0;
			for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
				if (!(*itc)) continue; //if(matched) continue;
				std::string chip = (*itc)->detectorId();
				// Only look at the clicpix
				if(chip != "CLi-CPix") continue;
				// Loop over pixels
				std::vector<RowColumnEntry*>::const_iterator ith;
				for (ith = (*itc)->hits()->begin(); ith != (*itc)->hits()->end(); ++ith) {
					
					// Check if this pixel is within the search window
					if( fabs((*ith)->column()-chipInterceptCol) > (residualmaxx/parameters->pixelPitchX["CLi-CPix"]) ||
						  fabs((*ith)->row()-chipInterceptRow)    > (residualmaxy/parameters->pixelPitchY["CLi-CPix"]) ) continue;
					
					pixelMatch=true;
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
	hAssociatedClustersVersusEventNo->Fill(eventnumber,nClustersAssociated);
	hAssociatedClustersVersusTriggerNo->Fill(m_triggerNumber,nClustersAssociated);
	if(nValidTracks != 0) hFrameEfficiency->Fill(eventnumber,nClustersAssociated/nValidTracks);
	
	// Finally, update the event number
	eventnumber++;
	m_triggerNumber++;
}

void ClicpixAnalysis::end()
{

	// Compare number of valid tracks, and number of clusters associated in order to get global efficiency
	double nTracks = hTrackIntercepts->GetEntries();
	double nClusters = hGlobalAssociatedClusterPositions->GetEntries();
	double efficiency = nClusters/nTracks;
	double errorEfficiency = (1./nTracks) * sqrt(nClusters*(1-efficiency));

	std::cout<<"***** Clicpix efficiency calculation *****"<<std::endl;
	std::cout<<"***** ntracks: "<<(int)nTracks<<", nclusters "<<(int)nClusters<<std::endl;
	std::cout<<"***** Efficiency: "<<100.*efficiency<<" +/- "<<100.*errorEfficiency<<" %"<<std::endl;
	
	std::cout<<endl;
	std::cout<<"***** If including the "<<(int)m_lostHits<<" lost pixel hits, this becomes "<<100.*(m_lostHits+nClusters)/nTracks<<" %"<<std::endl;
	// Now fill the efficiency maps

	// Map over full matrix
	for(double col=0;col<64;col+=1){
		for(double row=0;row<64;row+=1){
			double tracks = hTrackInterceptsChip->GetBinContent(hTrackInterceptsChip->GetXaxis()->FindBin(col),
																													hTrackInterceptsChip->GetYaxis()->FindBin(row));
			
			double tracksAssociated = hTrackInterceptsChipAssociated->GetBinContent(hTrackInterceptsChipAssociated->GetXaxis()->FindBin(col),
																																							hTrackInterceptsChipAssociated->GetYaxis()->FindBin(row));
			if(tracks != 0) hChipEfficiencyMap->Fill(col,row,tracksAssociated/tracks);
		}
	}

	// Map over single pixel
	for(double pixX=0;pixX<=50;pixX+=1){
		for(double pixY=0;pixY<=25;pixY+=1){
			double tracks = hTrackInterceptsPixel->GetBinContent(hTrackInterceptsPixel->GetXaxis()->FindBin(pixX),
																													hTrackInterceptsPixel->GetYaxis()->FindBin(pixY));
			
			double tracksAssociated = hTrackInterceptsPixelAssociated->GetBinContent(hTrackInterceptsPixelAssociated->GetXaxis()->FindBin(pixX),
																																							hTrackInterceptsPixelAssociated->GetYaxis()->FindBin(pixY));
			if(tracks != 0) hPixelEfficiencyMap->Fill(pixX,pixY,tracksAssociated/tracks);
		}
	}
	
}


void ClicpixAnalysis::fillClusterHistos(TestBeamClusters* clusters){

	// Small sub-routine to fill histograms that only need clusters
	
	int nCols = parameters->nPixelsX["CLi-CPix"];
	int nClusters=0;
	TestBeamClusters::iterator itc;
	// To check if all pixel hits match
	bool newFrame = false;
	for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
		if (!(*itc)) continue;
		std::string chip = (*itc)->detectorId();
		// Only look at the clicpix
		if(chip != "CLi-CPix") continue;

		// Fill cluster histograms
		hClusterSizeAll->Fill((*itc)->size());
		hClusterTOTAll->Fill((*itc)->totalADC());
		nClusters++;
		hGlobalClusterPositions->Fill((*itc)->globalX(),(*itc)->globalY());
		hClusterWidthRow->Fill((*itc)->rowWidth());
		hClusterWidthCol->Fill((*itc)->colWidth());
		
		// Loop over pixels
		std::vector<RowColumnEntry*>::const_iterator ith;
		for (ith = (*itc)->hits()->begin(); ith != (*itc)->hits()->end(); ++ith) {
			hHitPixels->Fill((*ith)->column(),(*ith)->row());
			hColumnHits->Fill((*ith)->column());
			hRowHits->Fill((*ith)->row());
			// Check if this clicpix frame is still the current
			int pixelID = (*ith)->column() + nCols*(*ith)->row();
			if( m_hitPixels[pixelID] != (*ith)->value()){
				// Reset the current
				if(!newFrame){m_hitPixels.clear(); newFrame=true;}
				m_hitPixels[pixelID] = (*ith)->value();
				m_triggerNumber=0;
			}
		}
		
	}
	hClustersPerEvent->Fill(nClusters);
	hClustersVersusEventNo->Fill(eventnumber,nClusters);
}



PositionVector3D< Cartesian3D<double> > ClicpixAnalysis::getTrackIntercept(TestBeamTrack* track, string chip, bool global)
{

	// Sub-routine to get the track intercept with the clicpix, either in the local or global co-ordinate frame
	
	// Check if the chip has alignment parameters
	if (parameters->alignment.count(chip) <= 0)
	{
		std::cerr << m_name << std::endl;
		std::cerr << "    ERROR: no alignment for " << chip << ". Breaking..."<<std::endl;
	}
	
	// Make the transform to the plane of the detector
	TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[chip]->displacementX(),
																												 parameters->alignment[chip]->displacementY(),
																												 parameters->alignment[chip]->displacementZ(),
																												 parameters->alignment[chip]->rotationX(),
																												 parameters->alignment[chip]->rotationY(),
																												 parameters->alignment[chip]->rotationZ());
	
	PositionVector3D< Cartesian3D<double> > planePointLocalCoords(0,0,0);
	PositionVector3D< Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform())*planePointLocalCoords;
	PositionVector3D< Cartesian3D<double> > planePoint2LocalCoords(0,0,1);
	PositionVector3D< Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform())*planePoint2LocalCoords;
	
	const double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
	const double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
	const double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
	
	const double length=((planePointGlobalCoords.X()-track->firstState()->X()) * normal_x +
											 (planePointGlobalCoords.Y()-track->firstState()->Y()) * normal_y +
											 (planePointGlobalCoords.Z()-track->firstState()->Z()) * normal_z) /
	(track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
	const float x_inter = track->firstState()->X() + length * track->direction()->X();
	const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
	const float z_inter = track->firstState()->Z() + length * track->direction()->Z();
	
	// change to local coordinates of that plane
	PositionVector3D< Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
	PositionVector3D< Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;

	delete mytransform; mytransform = 0;
	if(!global) return intersect_local;
	return intersect_global;
}

bool ClicpixAnalysis::checkProximity(TestBeamTrack* track, TestBeamTracks* tracks){
	
	// Sub-routine to check if there is another track close to the selected track.
	// "Close" is defined as the intercept at the clicpix
	
	bool close = false;
	PositionVector3D< Cartesian3D<double> > trackIntercept = getTrackIntercept(track,"CLi-CPix",true);

	TestBeamTracks::iterator it2;
	for (it2 = tracks->begin(); it2 != tracks->end(); ++it2) {
		// Got the track
		TestBeamTrack* track2 = (*it2);
		if (!track2) continue;
		
		// Get the track intercept with the clicpix plane (global co-ordinates)
		PositionVector3D< Cartesian3D<double> > trackIntercept2 = getTrackIntercept(track2,"CLi-CPix",true);
		if(trackIntercept.X() == trackIntercept2.X() && trackIntercept.Y() == trackIntercept2.Y()) continue;
		if( fabs(trackIntercept.X() - trackIntercept2.X()) <= 0.125 ||
			 fabs(trackIntercept.Y() - trackIntercept2.Y()) <= 0.125 ) close = true;
		
	}
	return close;
}

bool ClicpixAnalysis::checkMasked(double chipInterceptRow, double chipInterceptCol){

	// Sub-routine to check if a track has hit near a masked pixel.
	
	// limit on how many pixels away from a masked pixel the track intercept must be (less than 1)
	double limit = 0.5;
	int nCols = parameters->nPixelsX["CLi-CPix"];
	int hitCol = floor(chipInterceptCol); int hitRow = floor(chipInterceptRow);
	
	// Get the hit pixel ID
	int hitID = hitCol + hitRow*nCols;
	
	// Check if it went through a masked pixel directly
	if( parameters->maskedPixelsDUT.count(hitID) > 0. ) return true;
	
	// Check if it is near to a masked pixel (one left, one right, one up...)
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol-limit) + floor(chipInterceptRow-limit)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol-limit) + floor(chipInterceptRow)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol-limit) + floor(chipInterceptRow+limit)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol) + floor(chipInterceptRow-limit)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol) + floor(chipInterceptRow+limit)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol+limit) + floor(chipInterceptRow-limit)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol+limit) + floor(chipInterceptRow)*nCols ) > 0. ) return true;
	if( parameters->maskedPixelsDUT.count( floor(chipInterceptCol+limit) + floor(chipInterceptRow+limit)*nCols ) > 0. ) return true;
	
	return false;
	
}

	
void ClicpixAnalysis::fillResponseHistos(double chipInterceptRow, double chipInterceptCol, TestBeamCluster* matchedCluster){
	
	// Sub-routine to look at pixel response, ie. how far from the pixel is the track intercept for the pixel to still see charge
	
	// Loop over pixels
	std::vector<RowColumnEntry*>::const_iterator ith;
	for (ith = matchedCluster->hits()->begin(); ith != matchedCluster->hits()->end(); ith++) {
	
		// Fill histogram of difference between track intercept and pixel centre
		hPixelResponseX->Fill( (0.5+(*ith)->column()-chipInterceptCol) * parameters->pixelPitchX["CLi-CPix"] );
		hPixelResponseY->Fill( (0.5+(*ith)->row()-chipInterceptRow) * parameters->pixelPitchY["CLi-CPix"] );
		
    double xDistance = (0.5+(*ith)->column()-chipInterceptCol) * parameters->pixelPitchX["CLi-CPix"];
    double yDistance = (0.5+(*ith)->row()-chipInterceptRow) * parameters->pixelPitchY["CLi-CPix"];
    
    if( xDistance < m_responseWidth && xDistance > (-1.*m_responseWidth)){
      int responseIDX = floor(m_responseBins * (xDistance + m_responseWidth)/(2.*m_responseWidth));
      totMapPixelResponseX[responseIDX]->Fill((*ith)->value());
    }

    if( yDistance < m_responseWidth && yDistance > (-1.*m_responseWidth)){
      int responseIDY = floor(m_responseBins * (yDistance + m_responseWidth)/(2.*m_responseWidth));
      totMapPixelResponseY[responseIDY]->Fill((*ith)->value());
    }
    
		int col = floor(chipInterceptCol);
		if( col%2 == 1){
			hPixelResponseXOddCol->Fill( ((*ith)->column()-chipInterceptCol) * parameters->pixelPitchX["CLi-CPix"] );
			hPixelResponseYOddCol->Fill( ((*ith)->row()-chipInterceptRow) * parameters->pixelPitchY["CLi-CPix"] );
		}
		if( col%2 == 0){
			hPixelResponseXEvenCol->Fill( ((*ith)->column()-chipInterceptCol) * parameters->pixelPitchX["CLi-CPix"] );
			hPixelResponseYEvenCol->Fill( ((*ith)->row()-chipInterceptRow) * parameters->pixelPitchY["CLi-CPix"] );
		}

		
	}
	
	return;
}

	
	
	
	
	
	
	
