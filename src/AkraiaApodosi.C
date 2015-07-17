// Include files 
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"

#include "Clipboard.h"
#include "AkraiaApodosi.h"
#include "TestBeamCluster.h"
#include "TestBeamTransform.h"
#include "TestBeamTrack.h"

//-----------------------------------------------------------------------------
// Implementation file for class : AkraiaApodosi
// 
//    This class calculates the efficiency in a small area defined by the user. 
// Here efficiency is defined as simply the ratio of hits on the DUT over the intercepted tracks.  
// The DUT should NOT be in Pattern Recognition!!! Else the efficiency is obviously biased..
// The name "Akraia Apodosi" stands for "Perfomance at the edge" in Greek. It is named like this
// because the purpose of this class is to calculate the eficiency at the edge of the sensor. 
//
//======This class has not been tested yet for edge measurements. It has only been tested for areas at the center of the DUT.====
//
//  IMPORTANT NOTES: 
//    The efficiency is strongly depended on the cuts you apply (here called x/yrescut).
//  The x/yrescut is basically the window which this class looks for hits around each intercepted track.
//  For >99.50% efficiency on the D07-W0160 chip, use cuts from 0.100-0.108 mm. For each sensor and each DUT this varies.
//  Also, the ADCcuts in Main.C should be 100 and 400 for the telescope planes but 0 and infinite for the DUT.
//  
// 20-08-2012 : Panagiotis Tsopelas
//-----------------------------------------------------------------------------

AkraiaApodosi::AkraiaApodosi(Parameters* p, bool d)
  : Algorithm("AkraiaApodosi") {

  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
  m_debug = false;

}
 
void AkraiaApodosi::initial() {
  
  double limit = 0.05;                      // default
  //double limit = 0.5;
  const int nbins = int(800 * limit / 0.2); // was 400 * limit / 0.2
  const int nbins2D = 400;                  // added by me
  const int nL = 11;                        // added by me to solve the binning problem / grid pattern 
                                            // in plots derived from trnasformations
   
  // Histograms
 hProtoInter = new TH2F("protointer", "prototracks intersecting DUT", 
                        nbins2D, -nL, nL, nbins2D, -nL, nL);    
 hAllCluster = new TH2F("all_clusterhits" , "all clusterhits on DUT", 
                        nbins2D, -nL + 0.055 / 5.0, nL + 0.055 / 5.0, 
                        nbins2D, -nL - 0.055 / 5.0, nL - 0.055 / 5.0);     
 hCutCluster = new TH2F("cut_clusterhits", "cuts on clusterhits on DUT", 
                        nbins2D, -nL + 0.055 / 5.0, nL + 0.055 / 5.0, 
                        nbins2D, -nL - 0.055 / 5.0, nL - 0.055 / 5.0); 
 hNonDetected= new TH2F("nondetected" , "Non detected Tracks in Test Area", 
                        nbins2D, -nL + 0.055 / 5.0, nL + 0.055 / 5.0, 
                        nbins2D, -nL - 0.055 / 5.0, nL - 0.055 / 5.0); 
 hProtoRatio = new TH1F("protoratio", "Prototracks intersecting + Clusters around them", 11, -0.5, 10.5); 
 hProtoPerFrame = new TH1F("protoperframe", "Prototracks per frame in Test Area", 50, -0.5, 49.5);                     
 hClustPerFrame = new TH1F("clustperframe", "Clusterhits per frame in Test Area", 50, -0.5, 49.5);

 hLocResTestAreaX = new TH1F("testAreaLocResX", "test area local residuals in X", nbins, -0.2, 0.2);                   
 hLocResTestAreaY = new TH1F("testAreaLocResY", "test area local residuals in Y", nbins, -0.2, 0.2);                   
 hGloResTestAreaX = new TH1F("testAreaGloResX", "test area global residuals in X", nbins, -0.2, 0.2);                  
 hGloResTestAreaY = new TH1F("testAreaGloResY", "test area global residuals in Y", nbins, -0.2, 0.2);                  
    
}

void AkraiaApodosi::run(TestBeamEvent* event, Clipboard* clipboard) {  

  int detectedTrackCounter;
  int prototestcount = 0;
  int clusttestcount = 0;
  bool checker = false;

  // Set the size of the area the efficiency will be calculated in mm 
  // This  area will be called in the code and histos as test area.
  float winXlow  = -6.5; 
  float winXhigh = -3.2;
  float winYlow  =  6.10;
  float winYhigh =  6.70;
  event->doesNothing();
  static int icall = 0;
  ++icall;

  if (m_debug) std::cout << m_name << ": call " << icall << std::endl;
 
  // Grab the clusters from the clipboard. 
  TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
  if (!clusters) {
    std::cerr << m_name << std::endl;
    std::cerr << "    no clusters on clipboard" << std::endl;
    // No point in going on.
    return;
  } 

  // Grab the tracks from the clipboard.
  std::vector<TestBeamTrack*>* tracks = (TestBeamTracks*)clipboard->get("Tracks");
  // Make sure there are tracks.
  if (!tracks) return; 
  if (m_debug) std::cout << m_name << ": looping through tracks" << std::endl;
  TestBeamTracks::iterator it;
  for (it = tracks->begin(); it < tracks->end(); ++it) { 
    TestBeamTrack* track = (*it);
    // Get intersection point of track with the DUT 
    TestBeamTransform* mytransformD = new TestBeamTransform(parameters, parameters->dut);
    PositionVector3D<Cartesian3D<double> > planePointLocalCoordsD(0, 0, 0);
    PositionVector3D<Cartesian3D<double> > planePointGlobalCoordsD = (mytransformD->localToGlobalTransform()) * planePointLocalCoordsD;
    PositionVector3D<Cartesian3D<double> > planePoint2LocalCoordsD(0, 0, 1);
    PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoordsD = (mytransformD->localToGlobalTransform()) * planePoint2LocalCoordsD;
    double normal_xD = planePoint2GlobalCoordsD.X() - planePointGlobalCoordsD.X();
    double normal_yD = planePoint2GlobalCoordsD.Y() - planePointGlobalCoordsD.Y();
    double normal_zD = planePoint2GlobalCoordsD.Z() - planePointGlobalCoordsD.Z();
    double lengthD = ((planePointGlobalCoordsD.X() - track->firstState()->X()) * normal_xD +
                      (planePointGlobalCoordsD.Y() - track->firstState()->Y()) * normal_yD +
                      (planePointGlobalCoordsD.Z() - track->firstState()->Z()) * normal_zD) /
      (track->direction()->X() * normal_xD + track->direction()->Y() * normal_yD + track->direction()->Z() * normal_zD);
    float x_interD = track->firstState()->X() + lengthD * track->direction()->X();
    float y_interD = track->firstState()->Y() + lengthD * track->direction()->Y();
    float z_interD = track->firstState()->Z() + lengthD * track->direction()->Z();

    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_globalD(x_interD, y_interD, z_interD);
    PositionVector3D<Cartesian3D<double> > intersect_localD = (mytransformD->globalToLocalTransform()) * intersect_globalD;
    float x_inter_localD = intersect_localD.X();
    float y_inter_localD = intersect_localD.Y();

    hProtoInter->Fill(x_inter_localD, y_inter_localD);

    // Check if the track intercept is inside the test area.
    if (x_inter_localD > winXlow && x_inter_localD < winXhigh && 
        y_inter_localD > winYlow && y_inter_localD < winYhigh) {
      if (m_debug) {
        std::cout << m_name << ": INTER: " << x_inter_localD << "  " 
                                           << y_inter_localD << std::endl;
      }
      hProtoRatio->Fill(1);
      ++prototestcount;
      detectedTrackCounter = 0;
    }
      
    // Iterator itc goes through the clusters/hits of the DUT
    TestBeamClusters::iterator itc;
    for (itc = clusters->begin(); itc != clusters->end(); ++itc) {
      if ((*itc)->detectorId() != parameters->dut) continue;
      float xresidualD = (*itc)->globalX() - x_interD;
      float yresidualD = (*itc)->globalY() - y_interD;
      float localxresidualD = ((*itc)->colPosition() - 128) * 0.055 - x_inter_localD;
      float localyresidualD = ((*itc)->rowPosition() - 128) * 0.055 - y_inter_localD;
	 
      // limits for test area 
      float xresCut = 0.097;
      float yresCut = 0.097;
       	 
      // I have to add an additional check to avoid clusters being counted twice - one cluster is assigned to one track, not more.
      // Since itc iterates over all clusters, I avoid filling the histogram more than once.
      if (!checker) {	
        hAllCluster->Fill(((*itc)->colPosition() - 128) * 0.055, 
                          ((*itc)->rowPosition() - 128) * 0.055);
      }

      // Fill the histos before you apply cuts on them !!!!
      hLocResTestAreaX->Fill(localxresidualD);
      hLocResTestAreaY->Fill(localyresidualD);	  
      hGloResTestAreaX->Fill(xresidualD);
      hGloResTestAreaY->Fill(yresidualD); 

      // IMPORTANT: Here I check if there is a hit in my detector corresponding to the intersecting track within my resCut limits.
      if (fabs(localxresidualD) < xresCut && 
          fabs(localyresidualD) < yresCut && 
          x_inter_localD > winXlow && x_inter_localD < winXhigh &&
          y_inter_localD > winYlow && y_inter_localD < winYhigh) {
        // std::cout << "Akr HIT : " << ((*itc)->colPosition() - 128) * 0.055 << "  " << ((*itc)->rowPosition() - 128) * 0.055 << std::endl;
        ++detectedTrackCounter;
        hCutCluster->Fill(((*itc)->colPosition() - 128) * 0.055, 
                          ((*itc)->rowPosition() - 128) * 0.055);
        hProtoRatio->Fill(5);	     
        ++clusttestcount;
      }
    }
    // Check whether a track was not detected and print its coordinates
    if (detectedTrackCounter == 0 && 
        x_inter_localD > winXlow && x_inter_localD < winXhigh && 
        y_inter_localD > winYlow && y_inter_localD < winYhigh) {
      if (m_debug) {
        std::cout << m_name << ": MISSED TRACK !!! "  << x_inter_localD << "  " << y_inter_localD << std::endl;
      }
      hNonDetected->Fill(x_inter_localD, y_inter_localD);
    }
    // Check where the mismatch between the tracks intercepted and the ones detected is.
    if (hProtoRatio->GetBinContent(2) != hProtoRatio->GetBinContent(6)) {
      // std::cout << "   PROBLEM: " << hProtoRatio->GetBinContent(2) << "   " << hProtoRatio->GetBinContent(6) << std::endl;   
    }
    checker = true;    
    // std::cout << " END of Track check: "<< hProtoRatio->GetBinContent(2) << "   " << hProtoRatio->GetBinContent(6) << std::endl; 
  } 
  hProtoPerFrame->Fill(prototestcount);
  hClustPerFrame->Fill(clusttestcount);
   
}

void AkraiaApodosi::end() {

  if (!display || !parameters->verbose) return;

}
