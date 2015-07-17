// $Id: EtaDistribution.C,v 1.4 2010/09/02 15:04:23 prodrig Exp $
// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TH3.h"
#include "TF1.h"
#include "TProfile.h"
#include "TCutG.h"
#include "TLegend.h"
#include "THistPainter.h"

#include "Clipboard.h"
#include "RowColumnEntry.h"
#include "ClusterMaker.h"
#include "TestBeamEventElement.h"
#include "TestBeamCluster.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamTransform.h"
#include "TestBeamTrack.h"
#include "EtaDistribution.h"

using namespace std;

#define ROW 57
#define COL 57

//-----------------------------------------------------------------------------
// Implementation file for class : EtaDistribution
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

EtaDistribution::EtaDistribution(Parameters* p,bool d)
  : Algorithm("EtaDistribution")
{
  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(),  "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
}

EtaDistribution::~EtaDistribution() {} 
 
void EtaDistribution::initial()
{

  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    TH1F* h1 = 0;
    TH2F* h2 = 0;
    
    const int nbinsy = 220;
    const double ylo = -0.025;
    const double yhi = 0.080;
    const int nbinsx = 60;
    const double xlo = 0.00;
    const double xhi = 0.055;

    string title = std::string("etax") + chip;
    string name = std::string("etax_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    etax.insert(make_pair(chip, h2));
  
    title = std::string("etaxmulti") + chip;
    name = std::string("etaxmulti_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    etaxmulti.insert(make_pair(chip, h2));
  
    title = std::string("inverseetax") + chip;
    name = std::string("inverseetax_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetax.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaxmulti") + chip;
    name = std::string("inverseetaxmulti_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaxmulti.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaxcol1") + chip;
    name = std::string("inverseetaxcol1_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaxcol1.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaxcol2") + chip;
    name = std::string("inverseetaxcol2_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaxcol2.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaxcol3") + chip;
    name = std::string("inverseetaxcol3_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaxcol3.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaxcol4") + chip;
    name = std::string("inverseetaxcol4_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaxcol4.insert(make_pair(chip, h2));
    
    title = std::string("inverseetaxmultiprofile") + chip;
    name = std::string("inverseetaxmultiprofile_") + chip;
    TProfile *iwmp = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaxmultiprofile.insert(make_pair(chip,iwmp));
  
    title = std::string("inverseetaxcol1profile") + chip;
    name = std::string("inverseetaxcol1profile_") + chip;
    TProfile *iwmpc1 = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaxcol1profile.insert(make_pair(chip,iwmpc1));

    title = std::string("inverseetaxcol2profile") + chip;
    name = std::string("inverseetaxcol2profile_") + chip;
    TProfile *iwmpc2 = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaxcol2profile.insert(make_pair(chip,iwmpc2));

    title = std::string("inverseetaxcol3profile") + chip;
    name = std::string("inverseetaxcol3profile_") + chip;
    TProfile *iwmpc3 = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaxcol3profile.insert(make_pair(chip,iwmpc3));

    title = std::string("inverseetaxcol4profile") + chip;
    name = std::string("inverseetaxcol4profile_") + chip;
    TProfile *iwmpc4 = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaxcol4profile.insert(make_pair(chip,iwmpc4));
  
    title = std::string("inverseetaymulti") + chip;
    name = std::string("inverseetaymulti_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaymulti.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaycol1") + chip;
    name = std::string("inverseetaycol1_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaycol1.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaycol2") + chip;
    name = std::string("inverseetaycol2_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaycol2.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaycol3") + chip;
    name = std::string("inverseetaycol3_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaycol3.insert(make_pair(chip, h2));
  
    title = std::string("inverseetaycol4") + chip;
    name = std::string("inverseetaycol4_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo, xhi, nbinsy, ylo, yhi);
    inverseetaycol4.insert(make_pair(chip, h2));
    
    title = std::string("inverseetaymultiprofile") + chip;
    name = std::string("inverseetaymultiprofile_") + chip;
    TProfile *iwmpy = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaymultiprofile.insert(make_pair(chip,iwmpy));
  
    title = std::string("inverseetaycol1profile") + chip;
    name = std::string("inverseetaycol1profile_") + chip;
    TProfile *iwmpc1y = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaycol1profile.insert(make_pair(chip,iwmpc1y));

    title = std::string("inverseetaycol2profile") + chip;
    name = std::string("inverseetaycol2profile_") + chip;
    TProfile *iwmpc2y = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaycol2profile.insert(make_pair(chip,iwmpc2y));

    title = std::string("inverseetaycol3profile") + chip;
    name = std::string("inverseetaycol3profile_") + chip;
    TProfile *iwmpc3y = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaycol3profile.insert(make_pair(chip,iwmpc3y));

    title = std::string("inverseetaycol4profile") + chip;
    name = std::string("inverseetaycol4profile_") + chip;
    TProfile *iwmpc4y = new TProfile(name.c_str(), title.c_str(), nbinsx, xlo, xhi, ylo, yhi);
    inverseetaycol4profile.insert(make_pair(chip,iwmpc4y));
  
    title = std::string("1interpixdist") + chip;
    name = std::string("1interpixdist_") + chip;
    //h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.11);
    h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.055);
    interpixdist1.insert(make_pair(chip, h1));

    title = std::string("2interpixdist") + chip;
    name = std::string("2interpixdist_") + chip;
    //h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.11);
    h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.055);
    interpixdist2.insert(make_pair(chip, h1));

    title = std::string("3interpixdist") + chip;
    name = std::string("3interpixdist_") + chip;
    //h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.11);
    h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.055);
    interpixdist3.insert(make_pair(chip, h1));

    title = std::string("4interpixdist") + chip;
    name = std::string("4interpixdist_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 110,0.0,0.055);
    interpixdist4.insert(make_pair(chip, h1));

    title = std::string("etay") + chip;
    name = std::string("etay_") + chip;
    //h2 = new TH2F(name.c_str(), title.c_str(), 110,0.0,0.11,110,0.0,0.11);
    h2 = new TH2F(name.c_str(), title.c_str(), nbinsx, xlo,xhi,55,0.0,0.055);
    etay.insert(make_pair(chip, h2));
 
    title = std::string(" ADC vs x-y position 1hit, ") + chip;
    name = std::string("1hit_pulseshape_position_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 500,0.0,0.056,500,0.0,0.056 );
    pulse_position1hit.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x-y position 2hit, ") + chip;
    name = std::string("2hit_pulseshape_position_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 500,0.0,0.056,500,0.0,0.056 );
    pulse_position2hit.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x-y position 3hit, ") + chip;
    name = std::string("3hit_pulseshape_position_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 500,0.0,0.056,500,0.0,0.056);
    pulse_position3hit.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x-y position 4hit, ") + chip;
    name = std::string("4hit_pulseshape_position_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 500,0.0,0.056,500,0.0,0.056);
    pulse_position4hit.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x position 1hit, ") + chip;
    name = std::string("1hit_pulseshape_xposition_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 55,0.0,0.055,400,-0.5, 399.5 );
    pulse_position1hit_x.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x position 2hit, ") + chip;
    name = std::string("2hit_pulseshape_xposition_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 55,0.0,0.055,400,-0.5, 399.5 );
    pulse_position2hit_x.insert(make_pair(chip, h2));

    title = std::string(" ADC vs x position 3hit, ") + chip;
    name = std::string("3hit_pulseshape_xposition_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 55,0.0,0.055,400,-0.5, 399.5 );
    pulse_position3hit_x.insert(make_pair(chip, h2));

    title = std::string("etaxvh") + chip;
    name = std::string("etax_vh_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, -0.05, 1.05);
    etaxvh.insert(make_pair(chip, h1));

    title = std::string("etayvh") + chip;
    name = std::string("etay_vh_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, -0.05, 1.05);
    etayvh.insert(make_pair(chip, h1));

    title = std::string("etaxyvh") + chip;
    name = std::string("etaxy_vh_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 55, 0.0, 0.055, 55, 0.0, 0.055);
    etaxyvh.insert(make_pair(chip, h2));

    // title = std::string("arroweta") + chip;
    // name = std::string("arrow_eta") + chip;
    // h2 = new TH2F(name.c_str(), title.c_str(), 20, -4, 4, 20, -20, 20);
    // arroweta.insert(make_pair(chip, h2));
  }
  myfirstevent = 0;

}

void EtaDistribution::run(TestBeamEvent* event, Clipboard* clipboard)
{

  for (int i = 0; i < 56; i += 5) {
    for (int j = 0; j < 56; j += 5) {
      if (myfirstevent == 0) {
        totalx[i][j] = 0.;
        totaly[i][j]= 0.;
        meanx[i][j]= 0.;
        meany[i][j]= 0.;
        totalfitx[i][j] = 0.;
        totalfity[i][j]= 0.;
        meanfitx[i][j]= 0.;
        meanfity[i][j]= 0.;
        count[i][j]= 0;
      }
    }
  } 
  ++myfirstevent;
  event->doesNothing();
  TestBeamTracks* tracks=(TestBeamTracks*)clipboard->get("Tracks");
  if (tracks == NULL) return;
  for (TestBeamTracks::iterator it = tracks->begin(); it < tracks->end(); ++it) {
    TestBeamTrack* track = *it;  
    for (TestBeamClusters::iterator itc = (*it)->protoTrack()->clusters()->begin(); itc<(*it)->protoTrack()->clusters()->end(); ++itc) {
      const std::string dID = (*itc)->detectorId();       
      if (parameters->alignment.count(dID) <= 0) {
        std::cerr << m_name << std::endl;
        std::cerr << "    no alignment for " << dID << std::endl;
      }
      //get intersection point of track with sensor
      TestBeamTransform* mytransform = new TestBeamTransform( 
                    parameters->alignment[dID]->displacementX(),
                    parameters->alignment[dID]->displacementY(),
                    parameters->alignment[dID]->displacementZ(),
                    parameters->alignment[dID]->rotationX(),
                    parameters->alignment[dID]->rotationY(),
                    parameters->alignment[dID]->rotationZ());

      PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
      PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
      PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
      PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
  
      //std::cout <<"plane_local  "<<planePointLocalCoords<< std::endl;
      //std::cout <<"plane_global  "<<planePointGlobalCoords<< std::endl;
      //PositionVector3D< Cartesian3D<double> > planePointLocalCoords2 = (mytransform->globalToLocalTransform())*planePointGlobalCoords;
      //std::cout <<"plane_local2  "<<planePointLocalCoords2<< std::endl;
  
      const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
      const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
      const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
  
      const float length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
                            (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
                            (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
        (track->direction()->X() * normal_x+track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
      const float x_inter = track->firstState()->X() + length * track->direction()->X();
      const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
      const float z_inter = track->firstState()->Z() + length * track->direction()->Z();
    
      //change to local coordinates of that plane 
      PositionVector3D< Cartesian3D<double> > intersect_global(x_inter,y_inter,z_inter);
      PositionVector3D< Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform())*intersect_global;
      int offsetx = 128;
      if (parameters->nPixelsX.count(dID) > 0) {
        offsetx = parameters->nPixelsX[dID] / 2;
      } else if (parameters->nPixelsX.count("default") > 0) {
        offsetx = parameters->nPixelsX["default"] / 2;
      }
      int offsety = 128;
      if (parameters->nPixelsY.count(dID) > 0) {
        offsety = parameters->nPixelsY[dID] / 2;
      } else if (parameters->nPixelsY.count("default") > 0) {
        offsety = parameters->nPixelsY["default"] / 2;
      }
      double pitchx = 0.055;
      if (parameters->pixelPitchX.count(dID) > 0) {
        pitchx = parameters->pixelPitchX[dID];
      } else if (parameters->pixelPitchX.count("default") > 0) {
        pitchx = parameters->pixelPitchX["default"];
      }
      double pitchy = 0.055;
      if (parameters->pixelPitchY.count(dID) > 0) {
        pitchy = parameters->pixelPitchY[dID];
      } else if (parameters->pixelPitchY.count("default") > 0) {
        pitchy = parameters->pixelPitchY["default"];
      }
      float x_inter_local = intersect_local.X() + offsetx * pitchx;
      float y_inter_local = intersect_local.Y() + offsety * pitchy;
      //  cout<<"x intersect"<<x_inter_local<<endl;
      //  cout<<"y intersect"<<y_inter_local<<endl;
      //  cout<<(*itc)->colPosition()<<endl;
      //  cout<<(*itc)->rowPosition()<<endl;

    
      const double interpix = (*itc)->colPosition()*pitchx-pitchx*floor((*itc)->colPosition());
      const double interpixfit = x_inter_local-pitchx*floor((*itc)->colPosition());  
      const double interpixfitprobe = x_inter_local-pitchx*floor(x_inter_local/pitchx);
      const double interpixy = (*itc)->rowPosition()*pitchy-pitchy*floor((*itc)->rowPosition());
      const double interpixfity = y_inter_local-pitchy*floor((*itc)->rowPosition());  

      //  std::cout<<"inter"<<interpix<<std::endl;
      //  std::cout<<"xfit"<<interpixfit<<std::endl;
    
      //  cout<<interpix<<endl;
      //  cout<<interpixfit<<endl;
      etax[dID]->Fill(interpixfit, interpix);
      inverseetax[dID]->Fill(interpix, interpixfit);

      if ((*itc)->colWidth()>1){
        etaxmulti[dID]->Fill(interpixfit,interpix);
        inverseetaxmulti[dID]->Fill(interpix,interpixfit);
        inverseetaxmultiprofile[dID]->Fill(interpix,interpixfit);
      }
      if ((*itc)->colWidth()==1){
        inverseetaxcol1[dID]->Fill(interpix,interpixfit);
        inverseetaxcol1profile[dID]->Fill(interpix,interpixfit);
      }
      if ((*itc)->colWidth()==2){
        inverseetaxcol2[dID]->Fill(interpix,interpixfit);
        inverseetaxcol2profile[dID]->Fill(interpix,interpixfit);
      }
      if ((*itc)->colWidth()==3){
        inverseetaxcol3[dID]->Fill(interpix,interpixfit);
        inverseetaxcol3profile[dID]->Fill(interpix,interpixfit);
      }
      if ((*itc)->colWidth()==4){
        inverseetaxcol4[dID]->Fill(interpix,interpixfit);
        inverseetaxcol4profile[dID]->Fill(interpix,interpixfit);
      }
      //etay[dID]->Fill(interpixfitY,interpixY);

      if ((*itc)->rowWidth()>1){
        inverseetaymulti[dID]->Fill(interpixy,interpixfity);
        inverseetaymultiprofile[dID]->Fill(interpixy,interpixfity);
      }
      if ((*itc)->rowWidth()==1){
        inverseetaycol1[dID]->Fill(interpixy,interpixfity);
        inverseetaycol1profile[dID]->Fill(interpixy,interpixfity);
      }
      if ((*itc)->rowWidth()==2){
        inverseetaycol2[dID]->Fill(interpixy,interpixfity);
        inverseetaycol2profile[dID]->Fill(interpixy,interpixfity);
      }
      if ((*itc)->rowWidth()==3){
        inverseetaycol3[dID]->Fill(interpixy,interpixfity);
        inverseetaycol3profile[dID]->Fill(interpixy,interpixfity);
      }
      if ((*itc)->rowWidth()==4){
        inverseetaycol4[dID]->Fill(interpixy,interpixfity);
        inverseetaycol4profile[dID]->Fill(interpixy,interpixfity);
      }
      // if((*itc)->rowWidth()==1 && (*itc)->colWidth()==1) interpixdist1[dID]->Fill(interpix);
      // if((*itc)->colWidth()>1) interpixdist2[dID]->Fill(interpix);  
      //etaxyvh[dID]->Fill(interpixfit,interpixfitY);
      if((*itc)->colWidth()==1) interpixdist1[dID]->Fill(interpixfitprobe);
      if((*itc)->colWidth()==2) interpixdist2[dID]->Fill(interpixfitprobe);
      if((*itc)->colWidth()==3) interpixdist3[dID]->Fill(interpixfitprobe);
      if((*itc)->colWidth()==4) interpixdist4[dID]->Fill(interpixfitprobe);//this is the 'true' track fit position
      // if((*itc)->colWidth()==1) interpixdist1[dID]->Fill(interpix);//
      // if((*itc)->colWidth()==2) interpixdist2[dID]->Fill(interpix);//
      // if((*itc)->colWidth()==3) interpixdist3[dID]->Fill(interpix);//this is the 'reco' cluster position

      double myinterpixfit = x_inter_local+pitchx/2.-pitchx*floor((x_inter_local+pitchx/2.)/pitchy); // per pixel
      double myinterpixfitY = y_inter_local+pitchy/2.-pitchy*floor((y_inter_local+pitchy/2.)/pitchy); // per pixel

      double myinterpix = 0.;
      double myinterpixY = 0.;

      if ((*itc)->size() == 1) {
        pulse_position1hit_x[dID]->Fill(myinterpixfit,(*itc)->totalADC());
        pulse_position1hit[dID]->Fill(myinterpixfit,myinterpixfitY,(*itc)->totalADC());
      }
      if ((*itc)->size() == 2) {
        pulse_position2hit_x[dID]->Fill(myinterpixfit,(*itc)->totalADC());
        pulse_position2hit[dID]->Fill(myinterpixfit,myinterpixfitY,(*itc)->totalADC());  
      } 
      if ((*itc)->size() == 3) {
        pulse_position3hit_x[dID]->Fill(myinterpixfit,(*itc)->totalADC());
        pulse_position3hit[dID]->Fill(myinterpixfit,myinterpixfitY,(*itc)->totalADC());
      }
      if ((*itc)->size() == 4) {
        //pulse_position4hit_x[dID]->Fill(myinterpixfit,(*itc)->totalADC());
        pulse_position4hit[dID]->Fill(myinterpixfit,myinterpixfitY,(*itc)->totalADC());
      }

      //Arrow eta plot: 
      // Divide each pixel into 11*11 cells of 5mu each. Get the middle position of each cell (2.5 mu), and the mean reconstructed position of each cell (in x and y)
      if (dID == "D04-W0015") {
        int cellx = 0;
        int celly = 0;
        // cellx = (int)((1000*myinterpixfit)/(55./11.))*5;
        // celly = (int)((1000*myinterpixfitY)/(55./11.))*5;//to select cell based on track position
        cellx = (int)((1000*myinterpix)/(55./11.))*5;
        celly = (int)((1000*myinterpixY)/(55./11.))*5;//to select cell based on cluster position

        // std::cout<<"cell ("<<cellx<<","<<celly<<") cluster xy ("<<1000*myinterpix<<","<<1000*myinterpixY<<"), track xy ("<<1000*myinterpixfit<<","<<1000*myinterpixfitY<<")"<<std::endl;
        // array to store values per cell and calculate mean:
        totalx[cellx][celly] = totalx[cellx][celly] + 1000 * myinterpix;
        totaly[cellx][celly] = totaly[cellx][celly] + 1000 * myinterpixY;
        totalfitx[cellx][celly] = totalfitx[cellx][celly] + 1000 * myinterpixfit;
        totalfity[cellx][celly] = totalfity[cellx][celly] + 1000 * myinterpixfitY;
        count[cellx][celly] = count[cellx][celly] + 1;
        meanx[cellx][celly] = totalx[cellx][celly] / count[cellx][celly];
        meany[cellx][celly] = totaly[cellx][celly] / count[cellx][celly];
        meanfitx[cellx][celly] = totalfitx[cellx][celly] / count[cellx][celly];
        meanfity[cellx][celly] = totalfity[cellx][celly] / count[cellx][celly];
        // std::cout<<"chip "<<dID<<" cell ("<<cellx<<","<<celly<<") totalclus xy ("<<totalx[cellx][celly]<<","<<totaly[cellx][celly]<<"), count: "<<count[cellx][celly]<<" mean xy ("<<meanx[cellx][celly]<<","<<meany[cellx][celly]<<")"<<std::endl;
      }
    }//end loop clusters
  }//end loop tracks
}

void EtaDistribution::end()
{

  if (!display || !parameters->verbose) return;

  /*
  TCanvas *TBTFetaxprof = new TCanvas("TBTFetaxprof","VELO Timepix testbeam",768,768);
  TBTFetaxprof->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBTFetaxprof->cd(i+1);
    TProfile *prof = etax[summary->detectorId(i)]->ProfileY();
    prof->Fit("pol1");
    prof->DrawCopy();
    //etax[summary->detectorId(i)]->DrawCopy();
  }
  */

  TCanvas *TBTFetax = new TCanvas("TBTFetax","VELO Timepix testbeam - etaDistr",768,768);
  TBTFetax->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBTFetax->cd(i+1);
    etax[summary->detectorId(i)]->DrawCopy();
  }

  /*
  TCanvas *TBTFetayprof = new TCanvas("TBTFetayprof","VELO Timepix testbeam",768,768);
  TBTFetayprof->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBTFetayprof->cd(i+1);
    TProfile *prof = etay[summary->detectorId(i)]->ProfileY();
    prof->Fit("pol1");
    prof->DrawCopy();
  }
  //*/
  TCanvas *TBTFetay = new TCanvas("TBTFetay","VELO Timepix testbeam - etaDistr",768,768);
  TBTFetay->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBTFetay->cd(i+1);
    etay[summary->detectorId(i)]->DrawCopy();
  }

  TCanvas *TBTinterpixdist = new TCanvas("TBTInterpixdist","VELO Timepix testbeam - etaDistr",768,768);
  TBTinterpixdist->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBTinterpixdist->cd(i+1);
    interpixdist1[summary->detectorId(i)]->DrawCopy();
  }
  TCanvas *TBT2interpixdist = new TCanvas("TBT2Interpixdist","VELO Timepix testbeam - etaDistr",768,768);
  TBT2interpixdist->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBT2interpixdist->cd(i+1);
    interpixdist2[summary->detectorId(i)]->DrawCopy();
  }
  TCanvas *TBT3interpixdist = new TCanvas("TBT3Interpixdist","VELO Timepix testbeam - etaDistr",768,768);
  TBT3interpixdist->Divide(3,3);
  for(int i=0;i<summary->nDetectors();i++){
    TBT3interpixdist->cd(i+1);
    interpixdist3[summary->detectorId(i)]->DrawCopy();
  }

  // TCanvas *cVH = new TCanvas("cVH","VELO Timepix testbeam - pulseshapeVH1hit",1500,1000);
  // cVH->Divide(3,3);
  // TCanvas *cVH2 = new TCanvas("cVH2","VELO Timepix testbeam - pulseshapeVH2hit",1500,1000);
  // cVH2->Divide(3,3);
  // TCanvas *cVH3 = new TCanvas("cVH3","VELO Timepix testbeam - pulseshapeVH3hit",1500,1000);
  // cVH3->Divide(3,3);
  // TCanvas *cVH11 = new TCanvas("cVH11","VELO Timepix testbeam - x pulseshapeVH1hit",1500,1000);
  // cVH11->Divide(3,3);
  // TCanvas *cVH12 = new TCanvas("cVH12","VELO Timepix testbeam - x pulseshapeVH2hit",1500,1000);
  // cVH12->Divide(3,3);
  // TCanvas *cVH13 = new TCanvas("cVH13","VELO Timepix testbeam - x pulseshapeVH3hit",1500,1000);
  // cVH13->Divide(3,3);
  // TCanvas *cVH21 = new TCanvas("cVH21","VELO Timepix testbeam - vh eta x",1500,1000);
  // cVH21->Divide(3,3);
  // TCanvas *cVH22 = new TCanvas("cVH22","VELO Timepix testbeam - vh eta y",1500,1000);
  // cVH22->Divide(3,3);
  // TCanvas *cVH23 = new TCanvas("cVH23","VELO Timepix testbeam -vh eta xy",1500,1000);
  // cVH23->Divide(3,3);

  /*
  for(int i=0;i<summary->nDetectors();i++){
    cVH->cd(i+1);
    pulse_position1hit[summary->detectorId(i)]->GetYaxis()->SetTitle("Total ADC 1hit");
    pulse_position1hit[summary->detectorId(i)]->GetYaxis()->SetTitle("track y in pixel");
    pulse_position1hit[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position1hit[summary->detectorId(i)]->DrawCopy("colz");
    cVH2->cd(i+1);
    pulse_position2hit[summary->detectorId(i)]->GetYaxis()->SetTitle("track y in pixel");
    pulse_position2hit[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position2hit[summary->detectorId(i)]->DrawCopy("colz");
    cVH3->cd(i+1);
    pulse_position3hit[summary->detectorId(i)]->GetYaxis()->SetTitle("track y in pixel");
    pulse_position3hit[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position3hit[summary->detectorId(i)]->DrawCopy("colz");
    cVH11->cd(i+1);
    pulse_position1hit_x[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position1hit_x[summary->detectorId(i)]->GetYaxis()->SetTitle("ADC count");
    pulse_position1hit_x[summary->detectorId(i)]->DrawCopy("");
    cVH12->cd(i+1);
    pulse_position2hit_x[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position2hit_x[summary->detectorId(i)]->GetYaxis()->SetTitle("ADC count");
    pulse_position2hit_x[summary->detectorId(i)]->DrawCopy("");
    cVH13->cd(i+1);
    pulse_position3hit_x[summary->detectorId(i)]->GetXaxis()->SetTitle("track x in pixel");
    pulse_position3hit_x[summary->detectorId(i)]->GetYaxis()->SetTitle("ADC count");
    pulse_position3hit_x[summary->detectorId(i)]->DrawCopy("");
    cVH21->cd(i+1);
    etaxvh[summary->detectorId(i)]->GetXaxis()->SetTitle("eta x");
    etaxvh[summary->detectorId(i)]->GetYaxis()->SetTitle("entries");
    etaxvh[summary->detectorId(i)]->DrawCopy("");

    cVH22->cd(i+1);
    etayvh[summary->detectorId(i)]->GetXaxis()->SetTitle("eta y");
    etayvh[summary->detectorId(i)]->GetYaxis()->SetTitle("entries");
    etayvh[summary->detectorId(i)]->DrawCopy("");
    cVH23->cd(i+1);
    etaxyvh[summary->detectorId(i)]->Fill(etaxvh[summary->detectorId(i)],etayvh[summary->detectorId(i)]);
    etaxyvh[summary->detectorId(i)]->GetXaxis()->SetTitle("x");
    etaxyvh[summary->detectorId(i)]->GetYaxis()->SetTitle("y");
    etaxyvh[summary->detectorId(i)]->DrawCopy("lego2z");
  }
  //*/
  
}

