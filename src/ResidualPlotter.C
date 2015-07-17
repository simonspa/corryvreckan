// Include files 
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"

#include "Clipboard.h"
#include "ResidualPlotter.h"
#include "TestBeamTrack.h"
#include "TestBeamCluster.h"
#include "TestBeamTransform.h"

using namespace std;
//-----------------------------------------------------------------------------
// Implementation file for class : ResidualPlotter
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

ResidualPlotter::ResidualPlotter(Parameters* p, bool d)
  : Algorithm("ResidualPlotter") {

  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
  m_debug = false;

}
 
void ResidualPlotter::initial()
{
  
  hTracksClusterCount = new TH1F("tracksclustercount", "tracksclustercount", 10, -0.5, 9.5);
  hTrackSeparation = new TH1F("trackseparation", "trackseparation", 1000, 0., 14.);
  hTrackSlopeX = new TH1F("tracksxslope", "tracksxslope", 100, -0.005, 0.005);
  hTrackSlopeY = new TH1F("tracksyslope", "tracksyslope", 100, -0.005, 0.005);
  hTrackChiSquared = new TH1F("trackchisquared", "trackchisquared", 100, 0., 100.);
	icall = 0;
  for (int i = 0; i < summary->nDetectors(); i++) {
    const std::string chip = summary->detectorId(i);
    // Setup histograms.
    TH1F* h1 = 0;
    TH2F* h2 = 0;
    double limit = 0.05;
    const int nbins = int(200 * limit / 0.05);
    
    std::string title = std::string("global x residuals on ") + chip;
    std::string name = std::string("xresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResX.insert(make_pair(chip, h1));

		title = std::string("hTrackDiffX_") + chip;
		name = std::string("hTrackDiffX_") + chip;
		h1 = new TH1F(name.c_str(), title.c_str(), 2000, -10, 10);
		hTrackDiffX.insert(make_pair(chip, h1));

		title = std::string("hTrackDiffY_") + chip;
		name = std::string("hTrackDiffY_") + chip;
		h1 = new TH1F(name.c_str(), title.c_str(), 2000, -10, 10);
		hTrackDiffY.insert(make_pair(chip, h1));

		title = std::string("hTrackCorrelationX_") + chip;
		name = std::string("hTrackCorrelationX_") + chip;
		h2 = new TH2F(name.c_str(), title.c_str(), 400, -10, 10, 400, -10, 10);
		hTrackCorrelationX.insert(make_pair(chip, h2));

		title = std::string("hTrackCorrelationY_") + chip;
		name = std::string("hTrackCorrelationY_") + chip;
		h2 = new TH2F(name.c_str(), title.c_str(), 400, -10, 10, 400, -10, 10);
		hTrackCorrelationY.insert(make_pair(chip, h2));
		
    title = std::string("hResXversusTrackAngleX_") + chip;
    name = std::string("hResXversusTrackAngleX_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -0.005, 0.005, 200, -limit, limit);
    hResXversusTrackAngleX.insert(make_pair(chip, h2));

    title = std::string("hResYversusTrackAngleY_") + chip;
    name = std::string("hResYversusTrackAngleY_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -0.005, 0.005, 200, -limit, limit);
    hResYversusTrackAngleY.insert(make_pair(chip, h2));

    title = std::string("local x residuals on ") + chip;
    name = std::string("xresidual_local") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResXLocal.insert(make_pair(chip, h1));
    
    title = std::string("x resids as function of time") + chip;
    name = std::string("x resids time_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 50, 0, 2000, 200, -limit, limit);
    hResTimeX.insert(make_pair(chip, h2));
    
    title = std::string("global x residuals associated clusters on ") + chip;
    name = std::string("xresidualassociated_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResAssociatedX.insert(make_pair(chip, h1));

    title = std::string("global x residuals col1 on ") + chip;
    name = std::string("xresidualcol1_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResXCol1.insert(make_pair(chip, h1));
    
    title = std::string("global x residuals col2 on ") + chip;
    name = std::string("xresidualcol2_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResXCol2.insert(make_pair(chip, h1));
    
    title = std::string("global x residuals col3 on ") + chip;
    name = std::string("xresidualcol3_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResXCol3.insert(make_pair(chip, h1));
    
    title = std::string("global y residuals on ") + chip;
    name = std::string("yresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResY.insert(make_pair(chip, h1));
    
    title = std::string("local y residuals on ") + chip;
    name = std::string("yresidual_local_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResYLocal.insert(make_pair(chip, h1));

    title = std::string("y resids as function of time") + chip;
    name = std::string("y resids time_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 50, 0., 2000., 200, -limit, limit);
    hResTimeY.insert(make_pair(chip, h2));
        
    title = std::string("global y residuals associated clusters on ") + chip;
    name = std::string("yresidualassociated_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    hResAssociatedY.insert(make_pair(chip, h1));

    title = std::string("pseudoefficiency on ") + chip;
    name = std::string("pseudoefficiency_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 30, 0., 0.055, 30, 0., 0.055);
    hPseudoEfficiency.insert(make_pair(chip, h2));

    title = std::string("local track intercepts on ") + chip;
    name = std::string("localtrackintercepts_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -20., 20., 100, -20., 20.);
    hLocalTrackIntercepts.insert(make_pair(chip, h2));

    title = std::string("global track intercepts on ") + chip;
    name = std::string("globaltrackintercepts_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -20., 20., 100, -20., 20.);
    hGlobalTrackIntercepts.insert(make_pair(chip, h2));

    title = std::string("local cluster positions on ") + chip;
    name = std::string("localclusterpositions_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -20., 20., 100, -20., 20.);
    hLocalClusterPositions.insert(make_pair(chip, h2));

    title = std::string("x residuals vs global y on ") + chip;
    name = std::string("xresidualvsGlobaly_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResXGlobalY.insert(make_pair(chip, h2));

    title = std::string("x residuals vs global x on ") + chip;
    name = std::string("xresidualvsGlobalx_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResXGlobalX.insert(make_pair(chip, h2));

    title = std::string("y residuals vs global x on ") + chip;
    name = std::string("yresidualvsGlobalx_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResYGlobalX.insert(make_pair(chip, h2));

    title = std::string("y residuals vs global y on ") + chip;
    name = std::string("yresidualvsGlobaly_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResYGlobalY.insert(make_pair(chip, h2));

    title = std::string("localy residuals on ") + chip;
    name = std::string("localyresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, -limit, limit);
    hResYLocal.insert(make_pair(chip, h1));
    
    title = std::string("localx residuals on ") + chip;
    name = std::string("localxresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, -limit, limit);
    hResXLocal.insert(make_pair(chip, h1)); 

    title = std::string("y residuals vs local y on ") + chip;
    name = std::string("yresidualvsLocaly_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResYLocalY.insert(make_pair(chip, h2));
    
    title = std::string("x residuals vs local x on ") + chip;
    name = std::string("xrevsLocalxsidual_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -10, 10, 100, -limit, limit);
    hResXLocalX.insert(make_pair(chip, h2)); 

    double conversionfactor = 1.;
    if (chip == parameters->dut && parameters->pixelcalibration > 0) {
      conversionfactor = 22000. / 125.;
    }

    // TOT values
    title = std::string("Single Pixel TOT values ") + chip;
    name = std::string("SinglePixelTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hSinglePixelTOTValues.insert(make_pair(chip, h1));

    // Cluster ToT values
    title = std::string("Cluster TOT values ") + chip;
    name = std::string("ClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hClusterTOTValues.insert(make_pair(chip, h1));

    title = std::string("One Pixel Cluster TOT values ") + chip;
    name = std::string("OnePixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hOnePixelClusterTOTValues.insert(make_pair(chip, h1));

    title = std::string("One Pixel Central TOT values ") + chip;
    name = std::string("OnePixelCentralTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hOnePixelCentralTOTValues.insert(make_pair(chip, h1));

    title = std::string("Two Pixel Cluster TOT values ") + chip;
    name = std::string("TwoPixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hTwoPixelClusterTOTValues.insert(make_pair(chip, h1));

    title = std::string("Three Pixel Cluster TOT values ") + chip;
    name = std::string("ThreePixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hThreePixelClusterTOTValues.insert(make_pair(chip, h1));

    title = std::string("Four Pixel Cluster TOT values ") + chip;
    name = std::string("FourPixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hFourPixelClusterTOTValues.insert(make_pair(chip, h1));
  
    title = std::string("Four Pixel Non Square Cluster TOT values ") + chip;
    name = std::string("FourPixelClusterNonSquareTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hFourPixelClusterNonSquareTOTValues.insert(make_pair(chip, h1));

    title = std::string("Four Pixel Non Corner Cluster TOT values ") + chip;
    name = std::string("FourPixelClusterNonCornerTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,400.*conversionfactor);
    hFourPixelClusterNonCornerTOTValues.insert(make_pair(chip, h1));
  
    title = std::string("Cluster Sizes of Associated Clusters ") + chip;
    name = std::string("AssociatedClusterSizes_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 20, 0.5, 20.5);
    hAssociatedClusterSizes.insert(make_pair(chip, h1));

    // Global x correlation
    title = std::string("Correlation in global x ") + chip;
    name = std::string("CorrelationGlobalx_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 200, -20. ,20., 200, -9., 9.);
    hClusterCorrelationX.insert(make_pair(chip, h2));
    
    // Global y correlation
    title = std::string("Correlation in global y ") + chip;
    name = std::string("CorrelationGlobaly_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 200, -20., 20., 200, -9., 9.);
    hClusterCorrelationY.insert(make_pair(chip, h2));
    
    // Global x difference
    double factor = 60.;
    const int nbinsdiff = int(200. * factor / 6.);
    title = std::string("Difference in global x ") + chip;
    name = std::string("DifferenceGlobalx_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbinsdiff, -limit * factor, limit * factor);
    hClusterDiffX.insert(make_pair(chip, h1));

    // Global y difference
    title = std::string("Difference in global y ") + chip;
    name = std::string("DifferenceGlobaly_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbinsdiff, -limit * factor, limit * factor);
    hClusterDiffY.insert(make_pair(chip, h1));

    // Global x difference vs y
    title = std::string("Difference in global x vs y ") + chip;
    name = std::string("DifferenceGlobalxvsy_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -9., 9., 200, -limit, limit);
    hClusterDiffXvsY.insert(make_pair(chip, h2));

    // Global y difference vs x
    title = std::string("Difference in global y vs x ") + chip;
    name = std::string("DifferenceGlobalyvsx_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 100, -9., 9., 200, -limit, limit);
    hClusterDiffYvsX.insert(make_pair(chip, h2));
  }

}

void ResidualPlotter::run(TestBeamEvent* event, Clipboard* clipboard)
{
	icall++;

  if (m_debug) std::cout << m_name << ": call " << icall << std::endl;
  
  TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
  if (!clusters) {
    std::cerr << m_name << std::endl;
    std::cerr << "    no clusters on clipboard" << std::endl;
    // No point in going on.
    return;
  } 
  if (m_debug) std::cout << m_name << ": picking up " << clusters->size() << " clusters" << std::endl;
  // Fill cluster correlations histograms.
  for (TestBeamClusters::iterator it1 = clusters->begin(); 
       it1 != clusters->end(); ++it1) {
    if ((*it1)->detectorId() == parameters->referenceplane) {
      TestBeamClusters::iterator it2;
      for (it2 = clusters->begin(); it2 != clusters->end(); ++it2) {
        const std::string chip = (*it2)->detectorId();
        hClusterCorrelationX[chip]->Fill((*it1)->globalX(), (*it2)->globalX());
        if (fabs((*it1)->globalY() - (*it2)->globalY()) < 1.) {
          hClusterDiffX[chip]->Fill((*it1)->globalX() - (*it2)->globalX());
          hClusterDiffXvsY[chip]->Fill((*it1)->globalY(), (*it1)->globalX() - (*it2)->globalX());
        }
        hClusterCorrelationY[chip]->Fill((*it1)->globalY(), (*it2)->globalY());
        if (fabs((*it1)->globalX() - (*it2)->globalX()) < 1.) {
          hClusterDiffY[chip]->Fill((*it1)->globalY() - (*it2)->globalY());
          hClusterDiffYvsX[chip]->Fill((*it1)->globalX(), (*it1)->globalY() - (*it2)->globalY());
        }
      }
    }
  }
	
  // Grab the tracks from the clipboard.
	std::vector<TestBeamTrack*>* tracks = (TestBeamTracks*)clipboard->get("Tracks");
	std::vector<TestBeamTrack*>* upstreamTracks = (TestBeamTracks*)clipboard->get("UpstreamTracks");
	std::vector<TestBeamTrack*>* downstreamTracks = (TestBeamTracks*)clipboard->get("DownstreamTracks");

	// Make sure there are tracks.
  if (parameters->trackingPerArm && !parameters->joinTracksPerArm && (!upstreamTracks || !downstreamTracks) ) return;
  if ( (!parameters->trackingPerArm || (parameters->trackingPerArm && parameters->joinTracksPerArm)) && !tracks) return;
 
	if (parameters->trackingPerArm && !parameters->joinTracksPerArm){
		fillTrackHistos(upstreamTracks,clusters,1);
		fillTrackHistos(downstreamTracks,clusters,2);
	}else{
		fillTrackHistos(tracks,clusters,0);
	}
	
	
	/*
  if (parameters->alignmenttechnique < 2) return;
  // Pick up the coordinates of the device you want to align
  std::string dID = parameters->devicetoalign;
  if (parameters->alignment.count(dID) <= 0) {
    std::cerr << m_name << std::endl;
    std::cerr << "    ERROR: alignment for device to align " << dID << " not available" << std::endl;
    return;
  }
  TestBeamTransform* mytransform = new TestBeamTransform(parameters, dID);
  // Get intersection point of track with sensor
  PositionVector3D<Cartesian3D<double> > p1Local(0, 0, 0);
  PositionVector3D<Cartesian3D<double> > p1Global = (mytransform->localToGlobalTransform()) * p1Local;
  PositionVector3D<Cartesian3D<double> > p2Local(0, 0, 1);
  PositionVector3D<Cartesian3D<double> > p2Global = (mytransform->localToGlobalTransform()) * p2Local;
  float nx = p2Global.X() - p1Global.X();
  float ny = p2Global.Y() - p1Global.Y();
  float nz = p2Global.Z() - p1Global.Z();

  // Look at the distance between tracks at the device to align
  if (m_debug) std::cout << m_name << ": looping through tracks at position of " << dID << std::endl;
  TestBeamTracks::iterator it1 = tracks->begin();
  for (; it1 != tracks->end(); ++it1) {
    TestBeamTrack* track1 = (*it1);
    double l1 = ((p1Global.X() - track1->firstState()->X()) * nx +
                 (p1Global.Y() - track1->firstState()->Y()) * ny +
                 (p1Global.Z() - track1->firstState()->Z()) * nz) /
      (track1->direction()->X() * nx + track1->direction()->Y() * ny + track1->direction()->Z() * nz);
    double x1 = track1->firstState()->X() + l1 * track1->direction()->X();
    double y1 = track1->firstState()->Y() + l1 * track1->direction()->Y();
    TestBeamTracks::iterator it2 = it1;
    ++it2;
    for (;it2 != tracks->end(); ++it2) {
      TestBeamTrack* track2 = (*it2);
      double l2 = ((p1Global.X() - track2->firstState()->X()) * nx +
                   (p1Global.Y() - track2->firstState()->Y()) * ny +
                   (p1Global.Z() - track2->firstState()->Z()) * nz) /
        (track2->direction()->X() * nx + track2->direction()->Y() * ny + track2->direction()->Z() * nz);
      double x2 = track2->firstState()->X() + l2 * track2->direction()->X();
      double y2 = track2->firstState()->Y() + l2 * track2->direction()->Y();
      double dist = sqrt((pow((x2 - x1), 2) + pow((y2 - y1), 2)));
      hTrackSeparation->Fill(dist);
    }
  }
	 */
	return;
}

void ResidualPlotter::fillTrackHistos(std::vector<TestBeamTrack*>* tracks, TestBeamClusters* clusters, int type){
	
	bool fullTracks(false), upstreamTracks(false), downstreamTracks(false);
	
	if(type == 0) fullTracks = true;
	if(type == 1) upstreamTracks = true;
	if(type == 2) downstreamTracks = true;

	double pixelsquareintx;
	double pixelsquareinty;

	TestBeamTracks::iterator it;
	for (it = tracks->begin(); it != tracks->end(); ++it) {
		TestBeamTrack* track = (*it);
		if (!track) continue;
		TestBeamClusters::iterator itc;
		for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
			if (!(*itc)) continue;
			std::string chip = (*itc)->detectorId();
			
			// Only make residual plots etc. of downstream tracks with downstream planes
			if( (chip != parameters->dut) && upstreamTracks && !parameters->upstreamPlane[chip]) continue;
			if( (chip != parameters->dut) && downstreamTracks && !parameters->downstreamPlane[chip]) continue;

			TestBeamTransform* mytransform = new TestBeamTransform(parameters, chip);
			PositionVector3D<Cartesian3D<double> > p1Local(0, 0, 0);
			PositionVector3D<Cartesian3D<double> > p1Global = (mytransform->localToGlobalTransform()) * p1Local;
			PositionVector3D<Cartesian3D<double> > p2Local(0, 0, 1);
			PositionVector3D<Cartesian3D<double> > p2Global = (mytransform->localToGlobalTransform()) * p2Local;
			const double nx = p2Global.X() - p1Global.X();
			const double ny = p2Global.Y() - p1Global.Y();
			const double nz = p2Global.Z() - p1Global.Z();
			const double tx = track->direction()->X();
			const double ty = track->direction()->Y();
			const double tz = track->direction()->Z();
			const double length = ((p1Global.X() - track->firstState()->X()) * nx +
														 (p1Global.Y() - track->firstState()->Y()) * ny +
														 (p1Global.Z() - track->firstState()->Z()) * nz) /
			(tx * nx + ty * ny + tz * nz);
			float x_inter = track->firstState()->X() + length * tx;
			float y_inter = track->firstState()->Y() + length * ty;
			float z_inter = track->firstState()->Z() + length * tz;
			
			// Get local and global intersection points.
			PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
			PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
			float x_inter_local = intersect_local.X();
			float y_inter_local = intersect_local.Y();
			float x_inter_global = intersect_global.X();
			float y_inter_global = intersect_global.Y();
			
			delete mytransform;
			mytransform = 0;
			
			if (m_debug) {
				std::cout << m_name << ": forming global residual in chip " << chip
				<< " with cluster " << itc - clusters->begin() << std::endl;
			}
			
			const double xresidual = (*itc)->globalX() - x_inter;
			const double yresidual = (*itc)->globalY() - y_inter;
			int offsetx = 128;
			if (parameters->nPixelsX.count(chip) > 0) {
				offsetx = parameters->nPixelsX[chip] / 2;
			} else if (parameters->nPixelsX.count("default") > 0) {
				offsetx = parameters->nPixelsX["default"] / 2;
			}
			int offsety = 128;
			if (parameters->nPixelsY.count(chip) > 0) {
				offsety = parameters->nPixelsY[chip] / 2;
			} else if (parameters->nPixelsY.count("default") > 0) {
				offsety = parameters->nPixelsY["default"] / 2;
			}
			double pitchx = 0.055;
			if (parameters->pixelPitchX.count(chip) > 0) {
				pitchx = parameters->pixelPitchX[chip];
			} else if (parameters->pixelPitchX.count("default") > 0) {
				pitchx = parameters->pixelPitchX["default"];
			}
			double pitchy = 0.055;
			if (parameters->pixelPitchY.count(chip) > 0) {
				pitchy = parameters->pixelPitchY[chip];
			} else if (parameters->pixelPitchY.count("default") > 0) {
				pitchy = parameters->pixelPitchY["default"];
			}
			float localxresidual = ((*itc)->colPosition() - offsetx) * pitchx - x_inter_local;
			float localyresidual = ((*itc)->rowPosition() - offsety) * pitchy - y_inter_local;
			
			const float residualmaxx = parameters->residualmaxx;
			const float residualmaxy = parameters->residualmaxy;
			
			hTrackDiffX[chip]->Fill(xresidual);
			hTrackDiffY[chip]->Fill(yresidual);
			hTrackCorrelationX[chip]->Fill(x_inter,(*itc)->globalX());
			hTrackCorrelationY[chip]->Fill(y_inter,(*itc)->globalY());
			// Fill the residual histograms.
			if (fabs(yresidual) < residualmaxy) {
				hResX[chip]->Fill(xresidual);
				hResXLocal[chip]->Fill(localxresidual);
				hResTimeX[chip]->Fill(icall, xresidual);
				hResXGlobalY[chip]->Fill((*itc)->globalY(), xresidual);
				hResXGlobalX[chip]->Fill((*itc)->globalX(), xresidual);
				if ((*itc)->colWidth() == 1) hResXCol1[chip]->Fill(xresidual);
				if ((*itc)->colWidth() == 2) hResXCol2[chip]->Fill(xresidual);
				if ((*itc)->colWidth() == 3) hResXCol3[chip]->Fill(xresidual);
			}
			if (fabs(xresidual) < residualmaxx) {
				hResY[chip]->Fill(yresidual);
				hResYLocal[chip]->Fill(localyresidual);
				hResTimeY[chip]->Fill(icall, yresidual);
				hResYGlobalX[chip]->Fill((*itc)->globalX(), yresidual);
				hResYGlobalY[chip]->Fill((*itc)->globalY(), yresidual);
			}
			
			x_inter_local = x_inter_local + 0.5 * pitchx;
			y_inter_local = y_inter_local + 0.5 * pitchy;
			
			double interpixfitprobex = x_inter_local - pitchx * floor(x_inter_local / pitchx);
			double interpixfitprobey = y_inter_local - pitchy * floor(y_inter_local / pitchy);
			
			if (chip == parameters->dut) {
				pixelsquareintx = interpixfitprobex;
				pixelsquareinty = interpixfitprobey;
			}
			
			if (fabs(xresidual) < 0.1 && fabs(yresidual) < 0.1) {
				hPseudoEfficiency[chip]->Fill(interpixfitprobex, interpixfitprobey);
			}
			hLocalTrackIntercepts[chip]->Fill(x_inter_local, y_inter_local);
			hGlobalTrackIntercepts[chip]->Fill(x_inter_global, y_inter_global);
			hLocalClusterPositions[chip]->Fill(((*itc)->colPosition() - offsetx) * pitchx,
																				 ((*itc)->rowPosition() - offsety) * pitchy);
			
			hResXversusTrackAngleX[chip]->Fill(atan(track->slopeXZ()),xresidual);
			hResYversusTrackAngleY[chip]->Fill(atan(track->slopeYZ()),yresidual);
			
		}
		
		hTracksClusterCount->Fill(track->protoTrack()->getNumClustersOnTrack());
		hTrackSlopeX->Fill(atan(track->slopeXZ()));
		hTrackSlopeY->Fill(atan(track->slopeYZ()));
		hTrackChiSquared->Fill(track->chi2());
		
		// Now get the characteristics of clusters associated to tracks.
		if (m_debug) std::cout << m_name << ": looking at associated clusters " << std::endl;
		std::vector<TestBeamCluster*>* assclusters = track->protoTrack()->clusters();
		for (itc = assclusters->begin(); itc != assclusters->end(); ++itc) {
			if (!(*itc)) continue;
			const std::string chip = (*itc)->detectorId();
			if(chip == "Cen-tral") continue;
			double conversionfactor = 1.;
			if (chip == parameters->dut && parameters->pixelcalibration > 0) {
				conversionfactor = 22000. / 125.;
			}
			double adc = (*itc)->totalADC() * conversionfactor;
			hClusterTOTValues[chip]->Fill(adc);
			std::vector<RowColumnEntry*>::const_iterator ith;
			for (ith = (*itc)->hits()->begin(); ith != (*itc)->hits()->end(); ++ith) {
				if (!(*ith)) {
					// Shouldn't happen
					std::cerr << m_name << std::endl;
					std::cerr << "    ERROR: cannot access cluster hit" << std::endl;
					continue;
				}
				hSinglePixelTOTValues[chip]->Fill((*ith)->value() * conversionfactor);
			}
			if ((*itc)->size() == 1) {
				hOnePixelClusterTOTValues[chip]->Fill(adc);
				if ((pixelsquareintx > 0.015 && pixelsquareintx < 0.040) &&
						(pixelsquareinty > 0.015 && pixelsquareinty < 0.040)){
					hOnePixelCentralTOTValues[(*itc)->detectorId()]->Fill(adc);
				}
			} else if ((*itc)->size() == 2) {
				hTwoPixelClusterTOTValues[chip]->Fill(adc);
			} else if ((*itc)->size() == 3) {
				hThreePixelClusterTOTValues[chip]->Fill(adc);
			} else if ((*itc)->size() == 4) {
				hFourPixelClusterTOTValues[chip]->Fill(adc);
				if ((*itc)->colWidth() > 2 || ((*itc)->rowWidth()>2)) {
					hFourPixelClusterNonSquareTOTValues[chip]->Fill(adc);
				}
				if ((pixelsquareintx > 0.015 && pixelsquareintx < 0.040) ||
						(pixelsquareinty > 0.015 && pixelsquareinty < 0.040)) {
					hFourPixelClusterNonCornerTOTValues[chip]->Fill(adc);
				}
			}
			hAssociatedClusterSizes[chip]->Fill((*itc)->size());
			
			// Get intersection point of track with that sensor
			TestBeamTransform* mytransform = new TestBeamTransform(parameters, chip);
			PositionVector3D<Cartesian3D<double> > p1Local(0, 0, 0);
			PositionVector3D<Cartesian3D<double> > p1Global = (mytransform->localToGlobalTransform()) * p1Local;
			PositionVector3D<Cartesian3D<double> > p2Local(0, 0, 1);
			PositionVector3D<Cartesian3D<double> > p2Global = (mytransform->localToGlobalTransform()) * p2Local;
			double nx = p2Global.X() - p1Global.X();
			double ny = p2Global.Y() - p1Global.Y();
			double nz = p2Global.Z() - p1Global.Z();
			double tx = track->direction()->X();
			double ty = track->direction()->Y();
			double tz = track->direction()->Z();
			double length = ((p1Global.X() - track->firstState()->X()) * nx +
											 (p1Global.Y() - track->firstState()->Y()) * ny +
											 (p1Global.Z() - track->firstState()->Z()) * nz) /
			(tx * nx + ty * ny + tz * nz);
			float x_inter = track->firstState()->X() + length * tx;
			float y_inter = track->firstState()->Y() + length * ty;
			hResAssociatedX[chip]->Fill(x_inter - (*itc)->globalX());
			hResAssociatedY[chip]->Fill(y_inter - (*itc)->globalY());
			delete mytransform;
			mytransform = 0;
		}
	}

	
}













