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
#include "TrackFitter.h"
#include "TestBeamCluster.h"
#include "TestBeamTrack.h"
#include "TestBeamTransform.h"

//-----------------------------------------------------------------------------
// Implementation file for class : TrackFitter
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

TrackFitter::TrackFitter(Parameters* p, bool d) : 
    Algorithm("TrackFitter") {

  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);

}
 
void TrackFitter::initial() {

  hTracksClusterCount = new TH1F("TFtracksclustercount", "tracksclustercount", 11, -0.5, 10.5);
  hTrackSlopeX = new TH1F("TFtracksxslope", "tracksxslope", 100, -0.005, 0.005);
  hTrackSlopeY = new TH1F("TFtracksyslope", "tracksyslope", 100, -0.005, 0.005);
  hTrackChiSquared = new TH1F("TFtrackchisquared", "trackchisquared", 200, 0., 150.);
  hTrackChiSquaredNdof = new TH1F("TFtrackChiSquaredndof", "trackchi2ndof", 80, 0., 60.);
  hTrackProb = new TH1F("TFtrackprob", "trackprob", 50, 0., 1.);

  for (int i = 0; i < summary->nDetectors(); i++) {
    std::string chip = summary->detectorId(i);
		double limit = 0.05;
		if(chip == parameters->dut) limit = 0.1;
    const int nBins = int(200. * limit / 0.05);
    TH1F* h1 = 0;
    
    std::string title = std::string("global x residuals on ") + chip;
    std::string name = std::string("TFxresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResX.insert(make_pair(chip, h1));
    
    title = std::string("local x residuals on ") + chip;
    name = std::string("TFxresidual_local") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResLocalX.insert(make_pair(chip, h1));
    
    title = std::string("global y residuals on ") + chip;
    name = std::string("TFyresidual_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResY.insert(make_pair(chip, h1));
    
    title = std::string("local y residuals on ") + chip;
    name = std::string("TFyresidual_local_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResLocalY.insert(make_pair(chip, h1));
    
    title = std::string("global x residuals associated clusters on ") + chip;
    name = std::string("TFxresidualassociated_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResAssociatedX.insert(make_pair(chip, h1));
    
    title = std::string("global y residuals associated clusters on ") + chip;
    name = std::string("TFyresidualassociated_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit, limit);
    hResAssociatedY.insert(make_pair(chip, h1));
  }
  m_debug = false;

}

void TrackFitter::run(TestBeamEvent* event, Clipboard* clipboard) {

  event->doesNothing();

  // Grab the prototracks.
	TestBeamProtoTracks* prototracks = (TestBeamProtoTracks*)clipboard->get("ProtoTracks");
	TestBeamProtoTracks* upstreamPrototracks = (TestBeamProtoTracks*)clipboard->get("UpstreamProtoTracks");
	TestBeamProtoTracks* downstreamPrototracks = (TestBeamProtoTracks*)clipboard->get("DownstreamProtoTracks");
	
	// Check that you have the tracks
  if ( (!parameters->trackingPerArm || (parameters->trackingPerArm && parameters->joinTracksPerArm)) && !prototracks) {
    std::cerr << m_name << std::endl;
    std::cerr << "    no prototracks on clipboard" << std::endl;
    return;
	}else if (parameters->trackingPerArm && !parameters->joinTracksPerArm && (!upstreamPrototracks || !downstreamPrototracks)){
		std::cerr << m_name << std::endl;
		std::cerr << "    no upstream/downstream prototracks on clipboard" << std::endl;
		return;
	}
	
  // Grab the clusters.
  TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
  if (!clusters) {
    std::cerr << m_name << std::endl;
    std::cerr << "    no clusters on clipboard" << std::endl;
    return;
  }

	// Track containers
	std::vector<TestBeamTrack*>* tracks = new std::vector<TestBeamTrack*>;
	std::vector<TestBeamTrack*>* upstreamTracks = new std::vector<TestBeamTrack*>;
	std::vector<TestBeamTrack*>* downstreamTracks = new std::vector<TestBeamTrack*>;
	
	// Get the tracks for whichever configuration you use
	if (m_debug) std::cout << m_name << ": looping through prototracks " << std::endl;
	if (!parameters->trackingPerArm || ( parameters->trackingPerArm && parameters->joinTracksPerArm)){
		// Get the tracks
		getTracks(clusters, prototracks, tracks, 0);
		// Put the tracks on the clipboard.
		clipboard->put("Tracks", (TestBeamObjects*)tracks, CREATED);
	}
	else if ( parameters->trackingPerArm && !parameters->joinTracksPerArm){
		// Get the tracks
		getTracks(clusters, upstreamPrototracks, upstreamTracks, 1);
		getTracks(clusters, downstreamPrototracks, downstreamTracks, 2);
		// Put the tracks on the clipboard.
		clipboard->put("UpstreamTracks", (TestBeamObjects*)upstreamTracks, CREATED);
		clipboard->put("DownstreamTracks", (TestBeamObjects*)downstreamTracks, CREATED);
	}
	
 
}


void TrackFitter::getTracks(TestBeamClusters* clusters, TestBeamProtoTracks* prototracks, std::vector<TestBeamTrack*>*& tracks, int type){
	
	bool fullTracks(false), upstreamTracks(false), downstreamTracks(false);
	
	if(type == 0) fullTracks = true;
	if(type == 1) upstreamTracks = true;
	if(type == 2) downstreamTracks = true;
	
	TestBeamProtoTracks::iterator it;
	for (it = prototracks->begin(); it != prototracks->end(); ++it) {
		// Find slope and intercept from minimising least squares fit.
		TestBeamTrack* track = new TestBeamTrack(*it, parameters);
		if (m_debug) std::cout << m_name << ": about to fit track" << std::endl;
		track->fit();
		if (m_debug) std::cout << m_name << ": track chi2 is " << track->chi2()<< std::endl;
		
		// Track quality CUT!
		if( (track->chi2ndof() > parameters->trackchi2NDOF || track->probability() < parameters->trackprob) && !parameters->align ) continue;
		
		TestBeamClusters::iterator itc;
		for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
			if ((*itc) == NULL) continue;
			std::string chip = (*itc)->detectorId();
			// Make sure there is an alignment for this sensor.
			if (parameters->alignment.count(chip) <= 0) {
				std::cerr << m_name << std::endl;
				std::cerr << "    no alignment for " << chip << std::endl;
				return;
			}
			// Only make residual plots etc. of downstream tracks with downstream planes
			if( chip == parameters->dut && parameters->trackingPerArm && (!upstreamTracks || parameters->jointIntercept)) continue;
			if( (chip != parameters->dut) && upstreamTracks && !parameters->upstreamPlane[chip]) continue;
			if( (chip != parameters->dut) && downstreamTracks && !parameters->downstreamPlane[chip]) continue;
			
			// Get intersection point of track with that sensor.
			TestBeamTransform* mytransform = new TestBeamTransform(parameters, chip);
			PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
			PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
			PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
			PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
			
			const double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
			const double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
			const double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
			const double length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
														 (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
														 (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
			(track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
			const float x_inter = track->firstState()->X() + length * track->direction()->X();
			const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
			const float z_inter = track->firstState()->Z() + length * track->direction()->Z();
			
			// Change to local coordinates of that plane
			PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
			PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
			const float x_inter_local = intersect_local.X();
			const float y_inter_local = intersect_local.Y();
			
			const float xresidual = (*itc)->globalX() - x_inter;
			const float yresidual = (*itc)->globalY() - y_inter;
			const float localxresidual = ((*itc)->colPosition() - 128) * 0.055 - x_inter_local;
			const float localyresidual = ((*itc)->rowPosition() - 128) * 0.055 - y_inter_local;
			
			const float residualmaxx = parameters->residualmaxx;
			const float residualmaxy = parameters->residualmaxy;
			if ((fabs(yresidual) < residualmaxy) &&
					(fabs(xresidual) < residualmaxx) &&
					(chip == parameters->devicetoalign)) {
				track->addAlignmentClusterToTrack(*itc);
				if (m_debug) std::cout << m_name << ": added alignment cluster to track" << std::endl;
			}
			if (fabs(yresidual) < residualmaxy) {
				hResX[chip]->Fill(xresidual);
				hResLocalX[chip]->Fill(localxresidual);
			}
			if (fabs(xresidual) < residualmaxx) {
				hResY[chip]->Fill(yresidual);
				hResLocalY[chip]->Fill(localyresidual);
			}
			delete mytransform;
		}
		hTracksClusterCount->Fill((*it)->getNumClustersOnTrack());
		
		hTrackSlopeX->Fill(atan(track->slopeXZ()));
		hTrackSlopeY->Fill(atan(track->slopeYZ()));
		hTrackChiSquared->Fill(track->chi2());
		hTrackChiSquaredNdof->Fill(track->chi2()/track->ndof());
		hTrackProb->Fill(track->probability());
		tracks->push_back(track);
		
		
		// Plot the global residuals of all the associated clusters.
		TestBeamClusters::iterator ita;
		for (ita = (*it)->clusters()->begin(); ita != (*it)->clusters()->end(); ++ita) {
			if (*ita == NULL) continue;
			std::string chip = (*ita)->detectorId();
			if(chip == "Cen-tral") continue;
			// Get intersection point of track with that sensor
			TestBeamTransform* mytransform = new TestBeamTransform(parameters, chip);
			PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
			PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
			PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
			PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
			
			const double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
			const double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
			const double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
			
			const double length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
														 (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
														 (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
			(track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
			const float x_inter = track->firstState()->X() + length * track->direction()->X();
			const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
			hResAssociatedX[chip]->Fill(x_inter - (*ita)->globalX());
			hResAssociatedY[chip]->Fill(y_inter - (*ita)->globalY());
			delete mytransform;
		}
	}
	return;
}

void TrackFitter::end() {
	
}
