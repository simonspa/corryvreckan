// Include files 
#include <iostream>
#include <map>
#include <fstream>

#include "TFile.h"
#include "TGraph.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"

#include "Clipboard.h"
#include "Alignment.h"
#include "TestBeamEventElement.h"
#include "TestBeamTrack.h"
#include "TFitter.h"

using namespace std;

//-----------------------------------------------------------------------------
// Implementation file for class : Alignment
//
// 2009-06-20 : Malcolm John
// 2010-06-28 : Paula Collins
//-----------------------------------------------------------------------------

// The track store holds the tracks which we will minimize the alignment on. 
// It is, however, a member of the Alignment class, and the fcn function
// is not (see header file) so that the fcn function cannot see the track store.
// We get around this by defining a global pointer to the track store which is
// then visible to fcn. This is the power of pointers btw.  

// This alignment file does two different kinds of alignment.

// aligmenttechnique=1 just tries to globally place the detectors in a lined up way.
// it uses as a base the prototracks, so for this to work you have to apply a wide
// enough window in the MedipixTrackMaker program

// alignmenttechnique=2 aligns individual telescope planes.  It is perfect if for instance
// you have a good telescope alignment and you want to vary the angle of the device under test.
// It can also be used for refining the alignment.
// By default the pattern recognition will not use clusters for the tracking which are in the dut or in the 
// devicetoalign specified in the parameters file

// if you want to roughly align the dut using technique 1 you have to include the dut in the fit.

std::map<std::string, int> m_detNum;
int detectoridentifier(std::string dID) {
  return m_detNum[dID];
}
int nDetectorsInAlignmentFile = 0;
int nReferencePlane = 100;
std::vector<TestBeamTrack*>* globalAngledTrackStore;
std::vector<TestBeamTrack*>* globalTrackStore;
std::vector<TestBeamTrack*>* globalVetraTrackStore;
std::vector<TestBeamProtoTrack*>* globalProtoTrackStore;

std::string devicetoalign;
int devicenumbertoalign;
double chisquaredwindow;

TestBeamDataSummary* globalsummary;
Parameters* globalparameters;
bool angled = false;
TH1F* xresidualatstart_localcopy;
TH1F* yresidualatstart_localcopy;
TH1F* rresidualatstart_localcopy;
std::map<std::string, TH1F*> clusterdiffxbefore;
std::map<std::string, TH1F*> clusterdiffxafter;
std::map<std::string, TH1F*> clusterdiffybefore;
std::map<std::string, TH1F*> clusterdiffyafter;

TH1F* xresidualatend_localcopy;
TH1F* yresidualatend_localcopy;
TH1F* rresidualatend_localcopy;

double fei4weight;

Alignment::Alignment(Parameters* p, bool d)
  : Algorithm("Alignment") {
  parameters = p;
  display = d;

  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  m_outputfile = tbtree->GetCurrentFile()->GetName();
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);

  globalsummary = summary;
  globalparameters = parameters;

  trackStore = new std::vector<TestBeamTrack*>;
  vetratrackStore = new std::vector<TestBeamTrack*>;
  trackStoreAngled = new std::vector<TestBeamTrack*>;
  prototrackStore = new std::vector<TestBeamProtoTrack*>;

}

Alignment::~Alignment() {

  delete trackStore;       trackStore = 0;
  delete trackStoreAngled; trackStoreAngled = 0;
  delete prototrackStore;  prototrackStore = 0;
  delete vetratrackStore;  vetratrackStore = 0;
  globalTrackStore = 0;
  globalAngledTrackStore = 0;
  globalProtoTrackStore = 0;
  globalVetraTrackStore = 0;

} 

void Alignment::initial() {

  devicetoalign = parameters->devicetoalign;
  fei4weight = pow((parameters->residualmaxx / parameters->residualmaxy), 2);

  std::map<std::string, AlignmentParameters*> ap = parameters->alignment; 
  nDetectorsInAlignmentFile = 0;
  bool foundReferencePlane = false;
  bool foundDeviceToAlign = false;
  chisquaredwindow = parameters->trackwindow;
  std::map<std::string, AlignmentParameters*>::iterator it;
  for (it = ap.begin(); it != ap.end(); ++it) {
    m_detNum.insert(std::make_pair((*it).first, nDetectorsInAlignmentFile));
    if ((*it).first == parameters->referenceplane) {
      nReferencePlane = nDetectorsInAlignmentFile;
      foundReferencePlane = true;
    }
    if (parameters->verbose) {
      std::cout << "    Detector " << nDetectorsInAlignmentFile << ": " << (*it).first;
      if (nDetectorsInAlignmentFile == nReferencePlane) {
        std::cout << "          reference plane";
      }
      if ((*it).first == devicetoalign) {
        std::cout << "          device to align";
        foundDeviceToAlign = true;
      }
      std::cout << std::endl;
    }
    ++nDetectorsInAlignmentFile;
  }
  devicenumbertoalign = detectoridentifier(devicetoalign);
  if (!foundReferencePlane) {
    std::cerr << "    ERROR: reference plane " << parameters->referenceplane 
              << " is not in alignment file" << std::endl;
  }
  if (!foundDeviceToAlign) {
    std::cerr << "    WARNING: device to align " << devicetoalign
              << " is not in alignment file" << std::endl;
  }

  const double limitx = parameters->residualmaxx * 1.5;
  const double limity = parameters->residualmaxy * 1.5;
  const int nbinsres = 250;
  xresidualatstart_localcopy = new TH1F("xresidualatstart", "x residuals before alignment", nbinsres, -limitx, limitx);
  yresidualatstart_localcopy = new TH1F("yresidualatstart", "y residuals before alignment", nbinsres, -limity, limity);
  rresidualatstart_localcopy = new TH1F("rresidualatstart", "r residuals before alignment", nbinsres, -limitx, limitx);
  xresidualatend_localcopy = new TH1F("xresidualatend", "x residuals after alignment", nbinsres, -limitx, limitx);
  yresidualatend_localcopy = new TH1F("yresidualatend", "y residuals after alignment", nbinsres, -limity, limity);
  rresidualatend_localcopy = new TH1F("rresidualatend", "r residuals after alignment", nbinsres, -limitx, limitx);

  const double limit = parameters->trackwindow;
  const int nbins  = int(limit * 2 * 500);
  for (int i = 0; i < summary->nDetectors(); ++i) {
    std::string source = summary->source(i); 
    std::string chip = summary->detectorId(i);
    TH1F* h1 = 0;
    
    // Residuals
    std::string title = source + std::string("Residuals ") + chip;
    std::string name = std::string("Residuals_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, 0.07, 0.07);
    hResiduals.insert(make_pair(chip, h1));
    
    // Global x difference before alignment
    title = std::string("Difference in global x before alignment") + chip;
    name = std::string("DifferenceGlobalxbefore_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    clusterdiffxbefore.insert(make_pair(chip, h1));

    // Global y difference before alignment
    title = std::string("Difference in global y before alignment ") + chip;
    name = std::string("DifferenceGlobalybefore_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    clusterdiffybefore.insert(make_pair(chip, h1));

    // Global x difference after alignment
    title = std::string("Difference in global x after alignment") + chip;
    name = std::string("DifferenceGlobalxafter_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    clusterdiffxafter.insert(make_pair(chip, h1));

    // Global y difference after alignment
    title = std::string("Difference in global y after alignment ") + chip;
    name = std::string("DifferenceGlobalyafter_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit, limit);
    clusterdiffyafter.insert(make_pair(chip, h1));
  }
  m_debug = false;

}

void Alignment::run(TestBeamEvent* event, Clipboard* clipboard) {

  event->doesNothing();

	TestBeamTracks* tracks;
  // Grab the tracks.
	if(globalparameters->trackingPerArm && !globalparameters->joinTracksPerArm){
		if(globalparameters->alignDownstream) tracks = (TestBeamTracks*)clipboard->get("DownstreamTracks");
		if(!globalparameters->alignDownstream) tracks = (TestBeamTracks*)clipboard->get("UpstreamTracks");
	}else{
		tracks = (TestBeamTracks*)clipboard->get("Tracks");
	}
	
  if (!tracks) return;
  TestBeamTracks::iterator iter = tracks->begin();
  for (;iter != tracks->end(); ++iter) {
    TestBeamTrack* track = new TestBeamTrack(**iter, CLONE);
    if (abs(atan(track->slopeXZ())) > 0.001 || abs(atan(track->slopeYZ())) > 0.001) { //dhynds edited from 0.005
      trackStoreAngled->push_back(track);
    }
    trackStore->push_back(track);
  }

	TestBeamProtoTracks* prototracks;
	// Grab the prototracks.
	if(globalparameters->trackingPerArm && !globalparameters->joinTracksPerArm){
		if(globalparameters->alignDownstream) prototracks = (TestBeamProtoTracks*)clipboard->get("DownstreamProtoTracks");
		if(!globalparameters->alignDownstream) prototracks = (TestBeamProtoTracks*)clipboard->get("UpstreamProtoTracks");
	}else{
		prototracks = (TestBeamProtoTracks*)clipboard->get("ProtoTracks");
	}
	
	if (!prototracks) return;
  TestBeamProtoTracks::iterator protoiter = prototracks->begin();
  for (; protoiter != prototracks->end(); ++protoiter) {
    TestBeamProtoTrack* prototrack = new TestBeamProtoTrack(**protoiter, CLONE);
    prototrackStore->push_back(prototrack);
  }

  // Grab the Vetra alignment tracks.
  TestBeamTracks* vetratracks = (TestBeamTracks*)clipboard->get("VetraAlignmentTracks");
  if (!vetratracks) return;
  if (m_debug) {
    std::cout << "Alignment: found " << vetratracks->size() << " Vetra alignment tracks on the clipboard" << std::endl;
  }
  TestBeamTracks::iterator vetraiter = vetratracks->begin();
  for (;vetraiter != vetratracks->end(); ++vetraiter) {
    TestBeamTrack* track = new TestBeamTrack(**vetraiter, CLONE);  
    if (m_debug) {
      // std::cout << "fcn_technique2: track #" << vetraiter-vetratracks->begin() << " @ " << (*vetraiter) 
      //           << " has " << (*vetraiter)->vetraalignmentclusters()->size() << " vetraClusters" << std::endl;
      // std::cout << "fcn_technique2: Vetra alignment track number " << vetraiter-vetratracks->begin() << std::endl;
      // std::vector<VetraCluster*>* vetraclustersforalignment = track->vetraalignmentclusters();
      // for (std::vector< VetraCluster* >::iterator j=vetraclustersforalignment->begin(); j < vetraclustersforalignment->end(); ++j){
      //   std::cout << "fcn_technique2: associated cluster with cog " << (*j)->getVetraClusterPosition() << std::endl;
      // }
    }
    vetratrackStore->push_back(track);
  }

}

void fcn_technique2(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {
  // Alignment technique 2 for individual planes
 
  npar = npar;
  gin = gin;

  static int icount = 0;
  ++icount;
  const int idebug = 0;

  if (icount < idebug) {
    std::cout << "fcn_technique2: call " << icount << " to minimisation function (iflag " << iflag << ")" << std::endl;
  }

  double chi2 = 0.;

  // First pick up the coordinates of the device you want to align
  const double x = par[devicenumbertoalign * 6 + 0];
  const double y = par[devicenumbertoalign * 6 + 1];
  const double z = par[devicenumbertoalign * 6 + 2];
  const double xrot = par[devicenumbertoalign * 6 + 3];
  const double yrot = par[devicenumbertoalign * 6 + 4];
  const double zrot = par[devicenumbertoalign * 6 + 5];

  TestBeamTransform* mytransform = new TestBeamTransform(x, y, z, xrot, yrot, zrot);
  PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
  PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
  PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
  PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
  const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
  const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
  const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();

  std::vector<TestBeamTrack*>* alignmenttracks = globalTrackStore;
  if (icount < idebug) {
    std::cout << "fcn_technique2: will align " << devicetoalign << " with " << globalTrackStore->size() << " tracks " << std::endl;
  }
  TestBeamTracks::iterator iter = alignmenttracks->begin();
  int itrackcount = 0;
  for (;iter != alignmenttracks->end(); ++iter) {
    if (icount < idebug) std::cout << "fcn_technique2: track " << itrackcount << std::endl;
    TestBeamTrack* alignmenttrack = *iter;
    // Get the intercept of all the tracks with the device you want to align and form residuals with all clusters selected for alignment
    float length = ((planePointGlobalCoords.X() - alignmenttrack->firstState()->X()) * normal_x +
                    (planePointGlobalCoords.Y() - alignmenttrack->firstState()->Y()) * normal_y +
                    (planePointGlobalCoords.Z() - alignmenttrack->firstState()->Z()) * normal_z) /
      (alignmenttrack->direction()->X() * normal_x + alignmenttrack->direction()->Y() * normal_y + alignmenttrack->direction()->Z() * normal_z);
    const float x_inter = alignmenttrack->firstState()->X() + length * alignmenttrack->direction()->X();
    const float y_inter = alignmenttrack->firstState()->Y() + length * alignmenttrack->direction()->Y();
    const float z_inter = alignmenttrack->firstState()->Z() + length * alignmenttrack->direction()->Z();

    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    const float x_inter_local = intersect_local.X();
    const float y_inter_local = intersect_local.Y();
    if (icount < idebug) {
      std::cout << "    local x track intercept " << x_inter_local << std::endl;                
      std::cout << "    local y track intercept " << y_inter_local << std::endl; 
    }
    // float x_inter_global = intersect_global.X();
    // float y_inter_global = intersect_global.Y();
    // Grab the alignment clusters on this track.
    std::vector<TestBeamCluster* >* clustersforalignment = alignmenttrack->alignmentclusters();
    if (!clustersforalignment) {
      if (icount < idebug) std::cout << "    no alignment clusters on this track " << std::endl;
      continue; 
    }
    if (icount < idebug) {
      if (itrackcount == 1) {
        std::cout << "    " << clustersforalignment->size() << " alignment clusters on track " << itrackcount << std::endl;
        std::cout << "    memory location of alignment clusters " << clustersforalignment << endl;
      } else if (itrackcount == 36 || itrackcount == 37) {
        std::cout << "    " << clustersforalignment->size() << " alignment clusters on track " << itrackcount << std::endl;
        std::cout << "    first state of track " << alignmenttrack->firstState()->X() << std::endl;
      }
    }
    for (std::vector<TestBeamCluster*>::iterator j = clustersforalignment->begin(); j < clustersforalignment->end(); ++j) {
      if (icount < idebug && (itrackcount == 36 || itrackcount == 37)) {
        std::cout << "    global x of alignment cluster on track " <<  itrackcount << " is " << (*j)->globalX() << std::endl;
      }
      const std::string dID = (*j)->detectorId();
      int offsetx = 128;
      if (globalparameters->nPixelsX.count(dID) > 0) {
        offsetx = globalparameters->nPixelsX[dID] / 2;
      } else if (globalparameters->nPixelsX.count("default") > 0) {
        offsetx = globalparameters->nPixelsX["default"] / 2;
      }
      int offsety = 128;
      if (globalparameters->nPixelsY.count(dID) > 0) {
        offsety = globalparameters->nPixelsY[dID] / 2;
      } else if (globalparameters->nPixelsY.count("default") > 0) {
        offsety = globalparameters->nPixelsY["default"] / 2;
      }
      double pitchx = 0.055;
      if (globalparameters->pixelPitchX.count(dID) > 0) {
        pitchx = globalparameters->pixelPitchX[dID];
      } else if (globalparameters->pixelPitchX.count("default") > 0) {
        pitchx = globalparameters->pixelPitchX["default"];
      }
      double pitchy = 0.055;
      if (globalparameters->pixelPitchY.count(dID) > 0) {
        pitchy = globalparameters->pixelPitchY[dID];
      } else if (globalparameters->pixelPitchY.count("default") > 0) {
        pitchy = globalparameters->pixelPitchY["default"];
      }
      const float localxresidual = ((*j)->colPosition() - offsetx) * pitchx - x_inter_local;
      const float localyresidual = ((*j)->rowPosition() - offsety) * pitchy - y_inter_local;
      if (icount < idebug) {
        std::cout << "    x_inter local " << x_inter_local << " y_inter_local " << y_inter_local << std::endl; 
        std::cout << "    local xresidual " << localxresidual << " local yresidual " << localyresidual << std::endl;
      }
      chi2 += localxresidual * localxresidual + localyresidual * localyresidual;
      //chi2 += localyresidual * localyresidual;
      if (icount == 1) {
        xresidualatstart_localcopy->Fill(localxresidual);
        yresidualatstart_localcopy->Fill(localyresidual);
      }
      if (iflag == 3) {
        xresidualatend_localcopy->Fill(localxresidual);
        yresidualatend_localcopy->Fill(localyresidual);
      }
    }
    ++itrackcount;
  }
  delete mytransform;

  if (icount < idebug) {
    std::cout << "fcn_technique2: calculated chi2 is " << chi2<< std::endl;
  }
  f = chi2;

}

void fcn_technique4(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {
  // Alignment technique 4 for individual FEI4 planes

  npar = npar;
  gin = gin;

  static int icount = 0;
  ++icount;
  const int idebug = 0;

  if (icount < idebug) {
    std::cout << "fcn_technique4: call " << icount << " to minimisation function (iflag " << iflag << ")" << std::endl;
  }
  
  double chi2 = 0.;

  // First pick up the coordinates of the device you want to align
  const double x = par[devicenumbertoalign * 6 + 0];
  const double y = par[devicenumbertoalign * 6 + 1];
  const double z = par[devicenumbertoalign * 6 + 2];
  const double xrot = par[devicenumbertoalign * 6 + 3];
  const double yrot = par[devicenumbertoalign * 6 + 4];
  const double zrot = par[devicenumbertoalign * 6 + 5];

  TestBeamTransform* mytransform = new TestBeamTransform(x, y, z, xrot, yrot, zrot);
  PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
  PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
  PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
  PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
  const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
  const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
  const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();

  std::vector<TestBeamTrack*>* alignmenttracks = globalTrackStore;
  if (icount < idebug) {
    std::cout << "fcn_technique4: will align FEI4 with " << globalTrackStore->size() << " tracks" << std::endl;
  }
  TestBeamTracks::iterator iter = alignmenttracks->begin();
  int itrackcount = 0;
  for (;iter != alignmenttracks->end(); ++iter) {
    if (icount < idebug) std::cout << "fcn_technique4: track " << itrackcount << std::endl;
    TestBeamTrack* alignmenttrack = *iter;
    // Get the intercept of all the tracks with the device you want to align and form residuals with all clusters selected for alignment
    double length=((planePointGlobalCoords.X() - alignmenttrack->firstState()->X()) * normal_x +
                   (planePointGlobalCoords.Y() - alignmenttrack->firstState()->Y()) * normal_y +
                   (planePointGlobalCoords.Z() - alignmenttrack->firstState()->Z()) * normal_z) /
      (alignmenttrack->direction()->X() * normal_x + alignmenttrack->direction()->Y() * normal_y + alignmenttrack->direction()->Z() * normal_z);
    const double x_inter = alignmenttrack->firstState()->X() + length * alignmenttrack->direction()->X();
    const double y_inter = alignmenttrack->firstState()->Y() + length * alignmenttrack->direction()->Y();
    const double z_inter = alignmenttrack->firstState()->Z() + length * alignmenttrack->direction()->Z();
    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    const double x_inter_local = intersect_local.X();
    const double y_inter_local = intersect_local.Y();
    if (icount < idebug) {
      std::cout << "    local x track intercept " << x_inter_local << std::endl;                
      std::cout << "    local y track intercept " << y_inter_local << std::endl; 
    }
    // Grab the alignment clusters on this track.
    std::vector<FEI4Cluster*>* fei4clustersforalignment = alignmenttrack->fei4alignmentclusters();
    if (!fei4clustersforalignment) {
      if (icount < idebug) std::cout << "    no alignment clusters on this track " << std::endl;
      continue; 
    }
    if (icount < idebug) {
      std::cout << "    " << fei4clustersforalignment->size() << " FEI4 alignment clusters on track " << itrackcount << std::endl;
    }
    for (std::vector< FEI4Cluster* >::iterator j = fei4clustersforalignment->begin(); j < fei4clustersforalignment->end(); ++j) {
      const double localxresidual = x_inter_local - (*j)->colPosition() * 0.25;
      const double localyresidual = y_inter_local - (*j)->rowPosition() * 0.05;
      if (icount < idebug) std::cout << "    local x residual is " << localxresidual << ", local y residual is " << localyresidual << std::endl;
      chi2 += localxresidual * localxresidual / fei4weight + localyresidual * localyresidual;
      if (icount == 1) {
        xresidualatstart_localcopy->Fill(localxresidual);
        yresidualatstart_localcopy->Fill(localyresidual);
      }
      if (iflag == 3) {
        xresidualatend_localcopy->Fill(localxresidual);
        yresidualatend_localcopy->Fill(localyresidual);
      }
    }
    ++itrackcount;
  }
  delete mytransform;

  if (icount < idebug) {
    std::cout << "fcn_technique4: calculated chi2 is " << chi2<< std::endl;
  }
  f = chi2;

}


void fcn_technique5(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {
  // Alignment technique 5 for individual SciFi planes

  npar = npar;
  gin = gin;

  static int icount = 0;
  ++icount;
  const int idebug = 0;

  if (icount < idebug) {
    std::cout << "fcn_technique5: call " << icount << " to minimisation function (iflag " << iflag << ")" << std::endl;
  }
  
  double chi2 = 0.;

  // First pick up the coordinates of the device you want to align
  const double x = par[devicenumbertoalign * 6 + 0];
  const double y = par[devicenumbertoalign * 6 + 1];
  const double z = par[devicenumbertoalign * 6 + 2];
  const double xrot = par[devicenumbertoalign * 6 + 3];
  const double yrot = par[devicenumbertoalign * 6 + 4];
  const double zrot = par[devicenumbertoalign * 6 + 5];

  TestBeamTransform* mytransform = new TestBeamTransform(x, y, z, xrot, yrot, zrot);
  PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
  PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
  PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
  PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
  const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
  const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
  const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();

  std::vector<TestBeamTrack*>* alignmenttracks = globalTrackStore;
  if (icount < idebug) {
    std::cout << "fcn_technique5: will align SciFi with " << globalTrackStore->size() << " tracks" << std::endl;
  }
  TestBeamTracks::iterator iter = alignmenttracks->begin();
  int itrackcount = 0;
  for (;iter != alignmenttracks->end(); ++iter) {
    if (icount < idebug) std::cout << "fcn_technique5: track " << itrackcount << std::endl;
    TestBeamTrack* alignmenttrack = *iter;
    // Get the intercept of all the tracks with the device you want to align and form residuals with all clusters selected for alignment
    double length=((planePointGlobalCoords.X() - alignmenttrack->firstState()->X()) * normal_x +
                   (planePointGlobalCoords.Y() - alignmenttrack->firstState()->Y()) * normal_y +
                   (planePointGlobalCoords.Z() - alignmenttrack->firstState()->Z()) * normal_z) /
      (alignmenttrack->direction()->X() * normal_x + alignmenttrack->direction()->Y() * normal_y + alignmenttrack->direction()->Z() * normal_z);
    const double x_inter = alignmenttrack->firstState()->X() + length * alignmenttrack->direction()->X();
    const double y_inter = alignmenttrack->firstState()->Y() + length * alignmenttrack->direction()->Y();
    const double z_inter = alignmenttrack->firstState()->Z() + length * alignmenttrack->direction()->Z();
    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    const double x_inter_local = intersect_local.X();
    const double y_inter_local = intersect_local.Y();
    if (icount < idebug) {
      std::cout << "    local x track intercept " << x_inter_local << std::endl;                
      std::cout << "    local y track intercept " << y_inter_local << std::endl; 
    }
    // Grab the alignment clusters on this track.
    std::vector<SciFiCluster*>* scificlustersforalignment = alignmenttrack->scifialignmentclusters();
    if (!scificlustersforalignment) {
      if (icount < idebug) std::cout << "    no alignment clusters on this track " << std::endl;
      continue; 
    }
    if (icount < idebug) {
      std::cout << "    " << scificlustersforalignment->size() << " SciFi alignment clusters on track " << itrackcount << std::endl;
    }
    for (std::vector< SciFiCluster* >::iterator j = scificlustersforalignment->begin(); j < scificlustersforalignment->end(); ++j) {
      //std::cout << " position of cluster " << (*j)->getPosition() << " ADC " << (*j)->getADC() << " size " << (*j)->getClusterSize() << std::endl;
      const double localxresidual = x_inter_local - (*j)->getPosition();
      if (icount < idebug) std::cout << "    local x residual is " << localxresidual <<  std::endl;
      //chi2 += localxresidual * localxresidual / scifiweight;
      chi2 += localxresidual * localxresidual;
      if (icount == 1) {
        xresidualatstart_localcopy->Fill(localxresidual);
      }
      if (iflag == 3) {
        xresidualatend_localcopy->Fill(localxresidual);
      }
    }
    ++itrackcount;
  }
  delete mytransform;

  if (icount < idebug) {
    std::cout << "fcn_technique5: calculated chi2 is " << chi2<< std::endl;
  }
  f = chi2;

}


void fcn_technique3(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {
  // Alignment technique 3 for individual strip planes

  npar = npar;
  gin = gin;
  
  static int icount = 0;
  ++icount;
  const int idebug = 0;

  if (icount < idebug) {
    std::cout << "fcn_technique3: call " << icount << " to minimisation function (iflag " << iflag << ")" << std::endl;
  }

  double chi2 = 0.;

  // First pick up the coordinates of the device you want to align
  const double x = par[devicenumbertoalign * 6 + 0];
  const double y = par[devicenumbertoalign * 6 + 1];
  const double z = par[devicenumbertoalign * 6 + 2];
  const double xrot = par[devicenumbertoalign * 6 + 3];
  const double yrot = par[devicenumbertoalign * 6 + 4];
  const double zrot = par[devicenumbertoalign * 6 + 5];

  TestBeamTransform* mytransform = new TestBeamTransform(x, y, z, xrot, yrot, zrot);
  PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
  PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform())*planePointLocalCoords;
  PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
  PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform())*planePoint2LocalCoords;
  const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
  const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
  const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
  
  std::vector<TestBeamTrack*>* alignmenttracks = globalVetraTrackStore;
  if (icount < idebug) {
    std::cout << "fcn_technique3: will align vetra object with " << globalTrackStore->size() << " tracks" << std::endl;
    std::cout << "                initial positions of sensor are x = " << x << " y = " << y << " z = " << z 
              << " xrot = " << xrot << " yrot = " << yrot << " zrot = " << zrot << std::endl;
  }
  TestBeamTracks::iterator iter = alignmenttracks->begin();
  int itrackcount = 0;
  for (; iter != alignmenttracks->end(); ++iter) {
    if (icount < idebug) std::cout << "fcn_technique3: track " << itrackcount << std::endl;
    TestBeamTrack *alignmenttrack = *iter;
    // Get the intercept of all the tracks with the device you want to align and form residuals with all clusters selected for alignment
    double length=((planePointGlobalCoords.X() - alignmenttrack->firstState()->X()) * normal_x +
                   (planePointGlobalCoords.Y() - alignmenttrack->firstState()->Y()) * normal_y +
                   (planePointGlobalCoords.Z() - alignmenttrack->firstState()->Z()) * normal_z) /
      (alignmenttrack->direction()->X() * normal_x + alignmenttrack->direction()->Y() * normal_y + alignmenttrack->direction()->Z() * normal_z);
    const double x_inter = alignmenttrack->firstState()->X() + length * alignmenttrack->direction()->X();
    const double y_inter = alignmenttrack->firstState()->Y() + length * alignmenttrack->direction()->Y();
    const double z_inter = alignmenttrack->firstState()->Z() + length * alignmenttrack->direction()->Z();
    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    const double x_inter_local = intersect_local.X();
    const double y_inter_local = intersect_local.Y();
    if (icount < idebug && itrackcount == 1) {
      std::cout << "    local x track intercept " << x_inter_local << std::endl;                
      std::cout << "    local y track intercept " << y_inter_local << std::endl; 
    }
    // double r_inter_local = pow(x_inter_local * x_inter_local + y_inter_local * y_inter_local, 0.5);
    // Grab the alignment clusters on this track.
    std::vector< VetraCluster* >* clustersforalignment = alignmenttrack->vetraalignmentclusters();
    if (!clustersforalignment) continue; 
    if (icount < idebug && itrackcount == 1) {
      std::cout << "    " << clustersforalignment->size() << " alignment clusters on track " << itrackcount << std::endl;
      std::cout << "    memory location of align clusters: " << clustersforalignment << std::endl;
      // std::cout << "    track local intercept r " << r_inter_local << std::endl;
    }
    for (std::vector<VetraCluster* >::iterator j = clustersforalignment->begin(); j < clustersforalignment->end(); ++j){
      // double localrresidual = ((*j)->getVetraClusterPosition() - r_inter_local);
      const double localrresidual = ((*j)->getVetraClusterPosition() - x_inter_local);
      chi2 += localrresidual * localrresidual;
      if (icount == 1) rresidualatstart_localcopy->Fill(localrresidual);
      if (iflag == 3) rresidualatend_localcopy->Fill(localrresidual); 
    }
    ++itrackcount;
  }
  delete mytransform;

  if (icount < idebug) {
    std::cout << "fcn_technique2: calculated chi2 is " << chi2<< std::endl;
  }
  f = chi2;
  
}

void fcn_technique1(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {

  // Alignment technique 1 globally positions the detectors in a line

  npar = npar;
  gin = gin;

  static int icount = 0;
  ++icount;
  const int idebug = 2;
  
  double chi2 = 0.;

  if (icount < idebug){
    std::cout << "fcn_technique1: call " << icount << " to minimisation function (iflag " << iflag << ")" << std::endl;
    std::cout << "fcn_technique1: parameters are par0=" << par[0] << " par1=" << par[1] << " par2=" << par[2] << " par3=" << par[3] << std::endl;
  }

  double xreference = 0.;
  double yreference = 0.;

  // Loop through the proto tracks
  std::vector<TestBeamProtoTrack*>* alignmentprototracks = globalProtoTrackStore;
  TestBeamProtoTracks::iterator it = alignmentprototracks->begin();
  int tcount = 0;
  for (; it < alignmentprototracks->end(); ++it) {
    if (icount < idebug) std::cout << "fcn_technique1: prototrack " << tcount << std::endl;
    ++tcount;
    xreference = -1000.;
    yreference = -1000.;
    // Loop through clusters associated to the prototrack to find the global position of the reference cluster
    std::vector< TestBeamCluster* >* clusterloop = (*it)->clusters(); 
    int ccount = 0;
    if (icount < idebug) std::cout << "    Looping through clusters on track to find reference x, y" << std::endl;

    for (std::vector< TestBeamCluster* >::iterator kcluster = clusterloop->begin(); kcluster < clusterloop->end(); ++kcluster) {
      if (icount < idebug) std::cout << "      cluster " << ccount << std::endl;
      ++ccount;
      if ((*kcluster) == NULL) continue;
      // Find out which detector the cluster is on
      const std::string dID = (*kcluster)->detectorId();
      const int detectorid = detectoridentifier(dID);
      if (icount < idebug) std::cout << "        cluster is on detector " << dID << " (number " << detectorid << ")" << std::endl;
      if (detectorid == nReferencePlane) {
        // if (ccount < 2 && icount < 2) {
        //   std::cout << "fcn_technique1: using detector " << dID << " as the global reference " << std::endl;
        //   std::cout << "                If you are not happy with this choice hack Alignment.C " << std::endl;
        // }
        int offsetx = 128;
        if (globalparameters->nPixelsX.count(dID) > 0) {
          offsetx = globalparameters->nPixelsX[dID] / 2;
        } else if (globalparameters->nPixelsX.count("default") > 0) {
          offsetx = globalparameters->nPixelsX["default"] / 2;
        }
        int offsety = 128;
        if (globalparameters->nPixelsY.count(dID) > 0) {
          offsety = globalparameters->nPixelsY[dID] / 2;
        } else if (globalparameters->nPixelsY.count("default") > 0) {
          offsety = globalparameters->nPixelsY["default"] / 2;
        }
        double pitchx = 0.055;
        if (globalparameters->pixelPitchX.count(dID) > 0) {
          pitchx = globalparameters->pixelPitchX[dID];
        } else if (globalparameters->pixelPitchX.count("default") > 0) {
          pitchx = globalparameters->pixelPitchX["default"];
        }
        double pitchy = 0.055;
        if (globalparameters->pixelPitchY.count(dID) > 0) {
          pitchy = globalparameters->pixelPitchY[dID];
        } else if (globalparameters->pixelPitchY.count("default") > 0) {
          pitchy = globalparameters->pixelPitchY["default"];
        }
        TestBeamTransform* mytransform = new TestBeamTransform(par[6 * detectorid + 0], par[6 * detectorid + 1], par[6 * detectorid + 2],
                                                               par[6 * detectorid + 3], par[6 * detectorid + 4], par[6 * detectorid + 5]);
        // Define the local coordinate system with the pixel offset
        PositionVector3D<Cartesian3D<double> > localCoords(
            pitchx * ((*kcluster)->colPosition() - offsetx),
            pitchy * ((*kcluster)->rowPosition() - offsety),
            0.);
        if (icount < idebug) std::cout << "      local position is " << localCoords.X() << " " << localCoords.Y() << " " << localCoords.Z() << std::endl;
        // Move baby, move!
        PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords; 
        if (icount < idebug) std::cout << "      global position is " << globalCoords.X() << " " << globalCoords.Y() << " " << globalCoords.Z() << std::endl;
        xreference = globalCoords.X();
        yreference = globalCoords.Y();
        delete mytransform;
      }
    }

    if (icount < idebug) {
      std::cout << "    The reference global position is x= " << xreference << " y= " << yreference << std::endl;
      std::cout << "    Looping through clusters on track to find position wrt reference x,y" << std::endl;
    }
    // Loop through clusters and find the global position of each cluster relative to the reference cluster
    ccount = 0;
    for (std::vector< TestBeamCluster* >::iterator kcluster=clusterloop->begin(); kcluster < clusterloop->end(); ++kcluster) {
      if (icount < idebug) std::cout << "      cluster " << ccount << std::endl;
      ++ccount;
      if ((*kcluster) == NULL) continue; 
      // Find out which detector the cluster is on
      const std::string dID = (*kcluster)->detectorId();
      // Find the number of the detector so you can get the alignment parameters
      const int detectorid = detectoridentifier(dID);
      if (icount < idebug) std::cout << "        cluster is on detector (number " << detectorid << ")" << " with name " << dID << std::endl;
      if (icount < idebug) std::cout << " with alignment parameters x= " << par[6*detectorid +0] << " y= " << par[6*detectorid + 1] << std::endl;
      TestBeamTransform* mytransform = new TestBeamTransform(par[6 * detectorid + 0], par[6 * detectorid + 1], par[6 * detectorid + 2],
                                                             par[6 * detectorid + 3], par[6 * detectorid + 4], par[6 * detectorid + 5]);
      int offsetx = 128;
      if (globalparameters->nPixelsX.count(dID) > 0) {
        offsetx = globalparameters->nPixelsX[dID] / 2;
      } else if (globalparameters->nPixelsX.count("default") > 0) {
        offsetx = globalparameters->nPixelsX["default"] / 2;
      }
      int offsety = 128;
      if (globalparameters->nPixelsY.count(dID) > 0) {
        offsety = globalparameters->nPixelsY[dID] / 2;
      } else if (globalparameters->nPixelsY.count("default") > 0) {
        offsety = globalparameters->nPixelsY["default"] / 2;
      }
      double pitchx = 0.055;
      if (globalparameters->pixelPitchX.count(dID) > 0) {
        pitchx = globalparameters->pixelPitchX[dID];
      } else if (globalparameters->pixelPitchX.count("default") > 0) {
        pitchx = globalparameters->pixelPitchX["default"];
      }
      double pitchy = 0.055;
      if (globalparameters->pixelPitchY.count(dID) > 0) {
        pitchy = globalparameters->pixelPitchY[dID];
      } else if (globalparameters->pixelPitchY.count("default") > 0) {
        pitchy = globalparameters->pixelPitchY["default"];
      }
      // Define the local coordinate system with the pixel offset
      PositionVector3D<Cartesian3D<double> > localCoords(
          pitchx * ((*kcluster)->colPosition() - offsetx),
          pitchy * ((*kcluster)->rowPosition() - offsety),
          0);
      if (icount < idebug) {
        std::cout << "        local position is " << localCoords.X() << " " << localCoords.Y() << " " << localCoords.Z() << std::endl;
      }
      // Move baby, move!
      PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords; 
      if (icount < idebug) {
        std::cout << "        global position is " << globalCoords.X() << " " << globalCoords.Y() << " " << globalCoords.Z() << std::endl;
      }
      if (icount < idebug) {
        std::cout << "        The global residual in x and y is " 
                  << globalCoords.X() - xreference << "  " << globalCoords.Y() - yreference << std::endl;
      }
      
      double xresidual=min(chisquaredwindow,(globalCoords.X()-xreference));
      double yresidual=min(chisquaredwindow,(globalCoords.Y()-yreference));
      if (xresidual<(globalCoords.X()-xreference)) {
        if (icount < idebug) std::cout << " xresidual modifed from " << globalCoords.X()-xreference << " to " << xresidual << std::endl;
      }
      chi2 += pow(xresidual, 2) + pow(yresidual, 2);
      if (icount < idebug) std::cout << " chisquared running total " << chi2 << std::endl;
      if (icount == 1) {
        clusterdiffxbefore[dID]->Fill(globalCoords.X() - xreference);
        clusterdiffybefore[dID]->Fill(globalCoords.Y() - yreference);
      }
      if (iflag == 3) {
        clusterdiffxafter[dID]->Fill(globalCoords.X() - xreference);
        clusterdiffyafter[dID]->Fill(globalCoords.Y() - yreference);
      }
      delete mytransform;
    }
  }
  if (icount < idebug) {std::cout << " fcn_technique1: chi2 final total " << chi2 << std::endl;}
  f = chi2;

}

void fcn_technique0(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag) {

  npar = npar;
  gin = gin;
  iflag = iflag;

  static int icount = 0;
  ++icount;
  int idebug = 0;

  double xreference = 0.;
  double yreference = 0.;

  double chi2 = 0.;

  // We are about to align. Which is super trivial with the transform
  // classes which we have. First get the iterator to the beginning of the
  // track store from the global pointer defined above
  std::vector<TestBeamTrack*>* tracks = 0;
  if (angled) {
    tracks = globalAngledTrackStore;
  } else {
    tracks = globalTrackStore; 
  }

  // std::vector<TestBeamProtoTrack*>* tracks = globalProtoTrackStore;
  // TestBeamProtoTracks::iterator iter = tracks->begin();
  TestBeamTracks::iterator iter= tracks->begin();
  for (;iter < tracks->end(); ++iter) {
    // Paranoia check for null pointer
    if (*iter == NULL) continue; 
    // Now we iterate over each cluster on the track
    TestBeamClusters::iterator itclus = (*iter)->protoTrack()->clusters()->begin(); 
    // TestBeamClusters::iterator itclus = (*iter)->clusters()->begin(); 
    for(; itclus != (*iter)->protoTrack()->clusters()->end(); ++itclus) {
    // for(; itclus != (*iter)->clusters()->end(); ++itclus) {
      // Transform the cluster in question
      if (*itclus == NULL) continue;
      std::string dID = (*itclus)->detectorId();
			if(dID == "Cen-tral") continue;
      int i = detectoridentifier(dID);
      // Grab the new transform, and now we need to match it to the right MINUIT parameters...
      TestBeamTransform* mytransform = new TestBeamTransform(par[6 * i],     par[6 * i + 1], par[6 * i + 2], 
                                                             par[6 * i + 3], par[6 * i + 4], par[6 * i + 5]);
      int offsetx = 128;
      if (globalparameters->nPixelsX.count(dID) > 0) {
        offsetx = globalparameters->nPixelsX[dID] / 2;
      } else if (globalparameters->nPixelsX.count("default") > 0) {
        offsetx = globalparameters->nPixelsX["default"] / 2;
      }
      int offsety = 128;
      if (globalparameters->nPixelsY.count(dID) > 0) {
        offsety = globalparameters->nPixelsY[dID] / 2;
      } else if (globalparameters->nPixelsY.count("default") > 0) {
        offsety = globalparameters->nPixelsY["default"] / 2;
      }
      double pitchx = 0.055;
      if (globalparameters->pixelPitchX.count(dID) > 0) {
        pitchx = globalparameters->pixelPitchX[dID];
      } else if (globalparameters->pixelPitchX.count("default") > 0) {
        pitchx = globalparameters->pixelPitchX["default"];
      }
      double pitchy = 0.055;
      if (globalparameters->pixelPitchY.count(dID) > 0) {
        pitchy = globalparameters->pixelPitchY[dID];
      } else if (globalparameters->pixelPitchY.count("default") > 0) {
        pitchy = globalparameters->pixelPitchY["default"];
      }
      // Apply the pixel pitch by hand to the local coordinates
      PositionVector3D<Cartesian3D<double> > localCoords(
          pitchx * ((*itclus)->colPosition() - offsetx),
          pitchy * ((*itclus)->rowPosition() - offsety),
          0);
      // Do the transform and set the new cluster position
      PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords;
      (*itclus)->globalX(globalCoords.X());
      (*itclus)->globalY(globalCoords.Y());
      (*itclus)->globalZ(globalCoords.Z());
      // xreference = globalCoords.X();
      // yreference = globalCoords.Y();
      if (icount == 1) {
        clusterdiffxbefore[dID]->Fill(globalCoords.X() - xreference);
        clusterdiffybefore[dID]->Fill(globalCoords.Y() - yreference);
      }
      // if (iflag == 3) {
      //   clusterdiffxafter[dID]->Fill(globalCoords.X() - xreference);
      //   clusterdiffyafter[dID]->Fill(globalCoords.Y() - yreference);
      // }
      delete mytransform; 
    }  
    // Refit
    if (icount < idebug) std::cout << "    BEFORE: chi2 = " << (*iter)->chi2() << std::endl;
    (*iter)->fit();   
    if (icount < idebug) std::cout << "    AFTER:  chi2 = " << (*iter)->chi2() << std::endl;
    // Add the new track chi2 (recomputed with the new cluster positions) to the overall chi2 
    chi2 += (*iter)->chi2();
  }

  // The factor of 2 is because of how Minuit does errors. 
  // It could be wrong, read the manual and check I am doing the right thing here. 
  f = 2 * chi2;

}

void Alignment::end() {

  // Set the global track stores.
  globalProtoTrackStore = prototrackStore;
  globalTrackStore = trackStore;
  globalVetraTrackStore = vetratrackStore;
  globalAngledTrackStore = trackStoreAngled;

  // Some more monitoring
  if (m_debug) {
    std::cout << "Alignment: track store" << std::endl;
    TestBeamTracks::iterator iter = trackStore->begin();
    for(; iter != trackStore->end(); ++iter) {
      int number = iter-trackStore->begin();
      TestBeamTrack* track = (*iter);
      std::cout << number << "/" << trackStore->size() << "  " 
                << track->firstState()->X() << ", " 
                << track->firstState()->Y() << ", " 
                << track->firstState()->Z() << std::endl;
    }
  }

  // Set up the minuit fitter. You don't need to think about what arglist does
  // and don't change anything related to it.
  Double_t arglist[10];

  // Define the fitter. 
  // Initialize TMinuit with a maximum of 50 params.
  TFitter* myMinuit = new TFitter(50);
  // TVirtualFitter* myMinuit = TVirtualFitter::Fitter(0, 50); 
  // SetFCN sets the function it will use to minimize.
  std::cout << m_name << std::endl;
  if (parameters->alignmenttechnique == 0) {
    std::cout << "    Using alignment technique 0" << std::endl;
    myMinuit->SetFCN(fcn_technique0);
  } else if (parameters->alignmenttechnique == 1) {
    std::cout << "    Using alignment technique 1" << std::endl;
    myMinuit->SetFCN(fcn_technique1);
  } else if (parameters->alignmenttechnique == 2) {
    std::cout << "    Using alignment technique 2" << std::endl;
    myMinuit->SetFCN(fcn_technique2);
  } else if (parameters->alignmenttechnique == 3) {
    std::cout << "    Using alignment technique 3" << std::endl;
    myMinuit->SetFCN(fcn_technique3);
  } else if (parameters->alignmenttechnique == 4) {
    std::cout << "    Using alignment technique 4" << std::endl;
    myMinuit->SetFCN(fcn_technique4);
  } else if (parameters->alignmenttechnique == 5) {
    std::cout << "    Using alignment technique 5" << std::endl;
    myMinuit->SetFCN(fcn_technique5);
  } else {
    std::cerr << "    ERROR: pick 0, 1, or 2 for alignment technique (-l)" << std::endl;
    return;
  }
  // Check for serious problems.
  if (parameters->alignmenttechnique == 1) {
    if (!globalProtoTrackStore) {
      std::cerr << "    ERROR: there are no alignment prototracks" << std::endl;
      return;
    }
    if (globalProtoTrackStore->begin() == globalProtoTrackStore->end()) {
      std::cerr << "    ERROR: there are no alignment prototracks" << std::endl;
      return;
    }
    if (parameters->referenceplane == parameters->dut && parameters->dutinpatternrecognition) {
      std::cerr << "    ERROR: cannot use alignment technique 1 with DuT as reference plane" << std::endl;
      std::cerr << "           change the reference plane or include the DuT in the pattern recognition" << std::endl;
      return;
    }
  }
  if (parameters->alignmenttechnique == 2 || parameters->alignmenttechnique == 4 || parameters->alignmenttechnique == 5) {
    if (!globalTrackStore) {
      std::cerr << "    ERROR: there are no alignment tracks" << std::endl;
      return;
    }
    if (globalTrackStore->begin() == globalTrackStore->end()) {
      std::cerr << "    ERROR: there are no alignment tracks" << std::endl;
      return;
    }
  }
  if (parameters->alignmenttechnique == 3) {
    if (!globalVetraTrackStore) {
      std::cerr << "    ERROR: there are no Vetra alignment tracks" << std::endl;
      return;
    }
    if (globalVetraTrackStore->begin() == globalVetraTrackStore->end()) {
      std::cerr << "    ERROR: there are no Vetra alignment tracks" << std::endl;
      return;
    }
  }
  if (parameters->alignmenttechnique == 4 && devicetoalign != "FEI4") {
    std::cerr << "    ERROR: device to align (" << devicetoalign << ") is not a FEI4" << std::endl;
  }

  // The next four lines are not your concern to first order, you can worry
  // about them if you want to but you'll need to read the manual.
  // Set the verbosity.
  // -1  no output except from SHOW commands
  //  0  minimum output (no starting values or intermediate results)
  //  1  default value, normal output
  //  2  additional output giving intermediate results.
  //  3  maximum output, showing progress of minimizations.
  arglist[0] = 0;
  myMinuit->ExecuteCommand("SET PRINTOUT", arglist, 1);
  arglist[0] = 1.;
  myMinuit->ExecuteCommand("SET ERR", arglist, 1);

  // parameter number, parameter name, starting value, error, lowest allowed value, highest allowed value
  // TO TELL MINUIT NOT TO MINIMIZE A PARAMETER JUST SET THE "ERROR" TO 0 
  std::map<std::string, AlignmentParameters*> ap = parameters->alignment; 

  if (parameters->alignmenttechnique == 0) {
    std::map<std::string,AlignmentParameters*>::iterator it = ap.begin();
    int i = 0;
    for(; it != ap.end(); ++it) {
      myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0, -40.0, 40.0);
      myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0, -40.0, 40.0);
      myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0, -600.0, 600.0);
      myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0, -6.283, 6.283);
      myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0, -6.283, 6.283);
      myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0, -6.283, 6.283);
      ++i;
    }
    arglist[0] = 10000; arglist[1] = 1.e-2;

    const int nIterations0 = 1;
    for (int petlja = 0; petlja < nIterations0; ++petlja) {
      std::cout << "    Iteration " << petlja << std::endl;
      std::map<std::string, AlignmentParameters*>::iterator it1 = ap.begin();
      int i = 0;
      for (;it1 != ap.end(); ++it1) {
				if(parameters->alignDownstream && parameters->upstreamPlane[(*it1).first]){i++; continue;}
        if ((*it1).first != parameters->referenceplane &&
            (*it1).first != parameters->dut &&
            !parameters->masked[(*it1).first] &&
            !parameters->excludedFromPatternRecognition[(*it1).first] &&
						(*it1).first != "Cen-tral" ) {
          std::map<std::string, AlignmentParameters*>::iterator it2 = ap.begin();
          int j = 0;
          for (; it2 != ap.end(); ++it2) {
            double displacementX = myMinuit->GetParameter(6 * j);
            double displacementY = myMinuit->GetParameter(6 * j + 1);
            double displacementZ = myMinuit->GetParameter(6 * j + 2);
            double rotationX = myMinuit->GetParameter(6 * j + 3);
            double rotationY = myMinuit->GetParameter(6 * j + 4);
            double rotationZ = myMinuit->GetParameter(6 * j + 5);
            if (i == j) {
              std::cout << "      wobbling detector " << j << " (" << (*it2).first << ")" << std::endl;
              myMinuit->SetParameter(6 * j, ((*it2).first + "x").c_str(), displacementX, 0.01, -40.0, 40.0);
              myMinuit->SetParameter(6 * j + 1, ((*it2).first + "y").c_str(), displacementY, 0.01, -40.0, 40.0);
              myMinuit->SetParameter(6 * j + 2, ((*it2).first + "z").c_str(), displacementZ, 0., -600.0, 600.0); // dhynds edit here for z alignment
              myMinuit->SetParameter(6 * j + 3, ((*it2).first + "Rx").c_str(), rotationX, 0.1, -6.283, 6.283);
              myMinuit->SetParameter(6 * j + 4, ((*it2).first + "Ry").c_str(), rotationY, 0.1, -6.283, 6.283);
              myMinuit->SetParameter(6 * j + 5, ((*it2).first + "Rz").c_str(), rotationZ, 0.1, -6.283, 6.283);
            } else {  
              // std::cout << "      will keep fixed detector " << j << " (" << (*it2).first << ")" << std::endl;
              myMinuit->SetParameter(6 * j, ((*it2).first + "x").c_str(), displacementX, 0, -40.0, 40.0);
              myMinuit->SetParameter(6 * j + 1, ((*it2).first + "y").c_str(), displacementY, 0, -40.0, 40.0);
              myMinuit->SetParameter(6 * j + 2, ((*it2).first + "z").c_str(), displacementZ, 0, -600.0, 600.0);
              myMinuit->SetParameter(6 * j + 3, ((*it2).first + "Rx").c_str(), rotationX, 0, -6.283, 6.283);
              myMinuit->SetParameter(6 * j + 4, ((*it2).first + "Ry").c_str(), rotationY, 0, -6.283, 6.283);
              myMinuit->SetParameter(6 * j + 5, ((*it2).first + "Rz").c_str(), rotationZ, 0, -6.283, 6.283);
            }
            ++j;
          }
          // Execute the minimization
          myMinuit->ExecuteCommand("MIGRAD", arglist, 2);
          // Calculate the "proper" error matrix
          myMinuit->ExecuteCommand("HESSE", arglist ,0);
        }
        ++i;
      }
    }
  } else if (parameters->alignmenttechnique == 1) {
    std::map<std::string, AlignmentParameters*>::iterator it = ap.begin();
    int i = 0;
    std::cout << "      filling initial values" << std::endl;
    for (;it != ap.end(); ++it) {
      if ((*it).first != parameters->referenceplane && 
          (*it).first != parameters->dut &&
          !parameters->masked[(*it).first] &&
          !parameters->excludedFromPatternRecognition[(*it).first]) {
        std::cout << "        detector " << i << " (" << (*it).first  << ") with Z displacement " << (*it).second->displacementZ() << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first+"x").c_str(), (*it).second->displacementX(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0, -600.0, 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0.01, -6.283, 6.283);
      } else {
        std::cout << "        detector " << i << " (" << (*it).first << ") will be held fixed" << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0, -600., 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0, -6.283, 6.283);
      }
      ++i;
    }
    // Don't worry about this line
    arglist[0] = 10000; arglist[1] = 1.e-2;
    // Execute the minimization
    myMinuit->ExecuteCommand("MIGRAD", arglist, 2);
    // Calculate the "proper" error matrix
    myMinuit->ExecuteCommand("HESSE", arglist, 0);
    arglist[0] = 3.;
    myMinuit->ExecuteCommand("CALL", arglist, 1);
  } else if (parameters->alignmenttechnique == 2 || 
             parameters->alignmenttechnique == 4 || 
             parameters->alignmenttechnique == 5) {
  
    // For alignment technique 2 you can generally allow more free parameters; it is up to you!
    // These techniques allow a single plane to move around, for technique 4 it is the FEI4

    int i = 0;
    bool foundDeviceToAlign = false;
    std::map<std::string, AlignmentParameters*>::iterator it;
    for (it = ap.begin(); it != ap.end(); ++it) {
      if ((*it).first == devicetoalign) {
        foundDeviceToAlign = true;
        std::cout << "      detector " << i << " (" << (*it).first << ") will be wobbled" << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0.01 , -600.0, 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0.01, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0.01, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0.1, -6.283, 6.283);
      } else {
        std::cout << "      detector " << i << " (" << (*it).first  << ") will be kept fixed" << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0, -600., 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0, -6.283, 6.283);
      }
      ++i;
    }
    if (!foundDeviceToAlign) {
      std::cerr << "    ERROR: device to align " << devicetoalign << " not found\n";
      return;
    }
    // Don't worry about this line
    arglist[0] = 10000; arglist[1] = 1.e-2;
    // Execute the minimization
    myMinuit->ExecuteCommand("MIGRAD", arglist, 2);
    // Calculate the "proper" error matrix
    myMinuit->ExecuteCommand("HESSE", arglist, 0);
    arglist[0] = 3.;
    myMinuit->ExecuteCommand("CALL", arglist, 1);
  } else if (parameters->alignmenttechnique == 3) {
    // The PR01 probably can leave rz untouched, just slows down the code
    int i = 0;
    bool foundDeviceToAlign = false;
    std::map<std::string,AlignmentParameters*>::iterator it;
    for (it = ap.begin(); it != ap.end(); ++it) {
      if ((*it).first == devicetoalign) {
        foundDeviceToAlign = true;
        std::cout << "      detector " << i << " (" << (*it).first << ") will be wobbled" << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0.01, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0.0, -600.0, 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0.0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0.0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0.01, -6.283, 6.283);
      } else {
        std::cout << "      detector " << i << " (" << (*it).first  << ") will be kept fixed" << std::endl;
        myMinuit->SetParameter(6 * i, ((*it).first + "x").c_str(), (*it).second->displacementX(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 1, ((*it).first + "y").c_str(), (*it).second->displacementY(), 0, -40.0, 40.0);
        myMinuit->SetParameter(6 * i + 2, ((*it).first + "z").c_str(), (*it).second->displacementZ(), 0, -600., 600.0);
        myMinuit->SetParameter(6 * i + 3, ((*it).first + "Rx").c_str(), (*it).second->rotationX(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 4, ((*it).first + "Ry").c_str(), (*it).second->rotationY(), 0, -6.283, 6.283);
        myMinuit->SetParameter(6 * i + 5, ((*it).first + "Rz").c_str(), (*it).second->rotationZ(), 0, -6.283, 6.283);
      }
      ++i;
    }
    if (!foundDeviceToAlign) {
      std::cerr << "    ERROR: device to align " << devicetoalign << " not found\n";
      return;
    }
    // Don't worry about this line
    arglist[0] = 10000; arglist[1] = 1.e-2;
    // Execute the minimization
    myMinuit->ExecuteCommand("MIGRAD", arglist, 2);
    // Calculate the "proper" error matrix
    myMinuit->ExecuteCommand("HESSE", arglist, 0);
    arglist[0] = 3.;
    myMinuit->ExecuteCommand("CALL", arglist, 1);
  }
  std::cout << "    alignment finished" << std::endl;
  
  std::ofstream myfile;
  std::cout << m_outputfile << std::endl;
  std::string partstring = m_outputfile.substr(m_outputfile.size() - 12);
  std::string outfilename = "cond/AlignmentOutput_" + partstring + ".dat";
  std::cout << "    writing alignment output file " << outfilename << std::endl;
 
  myfile.open(outfilename.c_str());
  // Make a vector of planes and associated detector numbers sorted by z position.
  std::vector<std::pair<std::string, int> > planes;
  planes.clear();
  // Add the first plane.
  int detnum = 0;
  std::map<std::string, AlignmentParameters*>::iterator itm = ap.begin();
  planes.push_back(std::make_pair((*itm).first, detnum));
  ++itm;
  ++detnum;
  // Add the other planes.
  std::vector<std::pair<std::string, int> >::iterator itv;
  for (; itm != ap.end(); ++itm) {
    if (myMinuit->GetParameter(6 * detnum + 2) > myMinuit->GetParameter(6 * planes.back().second + 2)) {
      planes.push_back(std::make_pair((*itm).first, detnum));
      
    } else {
      for (itv = planes.begin(); itv != planes.end(); ++itv) {
        if (myMinuit->GetParameter(6 * detnum + 2) < myMinuit->GetParameter(6 * (*itv).second + 2)) {
          break;
        }
      }
      planes.insert(itv, std::make_pair((*itm).first, detnum));
    }
    ++detnum;
  }
  
  for (itv = planes.begin(); itv != planes.end(); ++itv) {
    detnum = (*itv).second;
    myfile << (*itv).first << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 0) << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 1) << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 2) << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 3) << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 4) << "  "
           << std::setw(10) << std::setprecision(6) << myMinuit->GetParameter(6 * detnum + 5) << "\n";
  }
  myfile.close();
  

  // Uncomment the following lines to perform scans of the alignment variables
  // TCanvas* debugCan = new TCanvas();
  // debugCan->Divide(5);
  // arglist[1] = 5000;
  // arglist[2] = -20.;
  // arglist[3] = 20.;
  // for (int petljica = 0; petljica < 5; ++petljica) {
  //   if (petljica < 2) arglist[0] = 6 * i + 1 + petljica;
  //   else arglist[0] = 6 * i + 2 + petljica;
  //   myMinuit->ExecuteCommand("SCAN", arglist, 2);
  //   debugCan->cd(petljica + 1);
  //   TGraph* gr = dynamic_cast<TGraph *>(myMinuit->GetMinuit()->GetPlot());
  //   gr->Draw("AP");
  // } 

  arglist[0] = 10000; arglist[1] = 1.e-2;

  if (parameters->alignmenttechnique == 1) { // || parameters->alignmenttechnique == 0){
    TCanvas* beforex = new TCanvas("beforex", "Before Alignment X", 1000, 1000);
    TPad* beforexpad = new TPad("beforexpad", "Before Alignment X", 0, 0, 1.0, 1.0);
    beforex->cd();
    beforexpad->Draw();
    beforexpad->Divide(3, 3); 

    TCanvas* beforey = new TCanvas("beforey", "Before Alignment Y", 1000, 1000);
    TPad* beforeypad = new TPad("beforeypad", "Before Alignment Y", 0, 0, 1.0, 1.0);
    beforey->cd();
    beforeypad->Draw();
    beforeypad->Divide(3, 3);

    TCanvas* afterx = new TCanvas("afterx", "After Alignment X", 1000, 1000);
    TPad* afterxpad = new TPad("afterxpad", "After Alignment X", 0, 0, 1.0, 1.0);
    afterx->cd();
    afterxpad->Draw();
    afterxpad->Divide(3, 3); 

    TCanvas* aftery = new TCanvas("aftery", "After Alignment Y", 1000, 1000);
    TPad* afterypad = new TPad("afterypad", "After Alignment Y", 0, 0, 1.0, 1.0);
    aftery->cd();
    afterypad->Draw();
    afterypad->Divide(3, 3);

    for (int i = 0; i < (summary->nDetectors()); ++i) {
      std::string dID = summary->detectorId(i);
      gStyle->SetOptStat(0);
      gStyle->SetOptFit(111111);

      beforexpad->cd(i + 1);           
      clusterdiffxbefore[dID]->Fit("gaus", "Q");
      clusterdiffxbefore[dID]->DrawCopy();
      beforeypad->cd(i + 1);
      clusterdiffybefore[dID]->Fit("gaus", "Q");
      clusterdiffybefore[dID]->DrawCopy();

      afterxpad->cd(i + 1);
      clusterdiffxafter[dID]->Fit("gaus", "Q");
      clusterdiffxafter[dID]->DrawCopy();
      afterypad->cd(i + 1);
      clusterdiffyafter[dID]->Fit("gaus", "Q");
      clusterdiffyafter[dID]->DrawCopy();

    }

  } else if (parameters->alignmenttechnique == 2 || 
             parameters->alignmenttechnique == 3 ||
             parameters->alignmenttechnique == 4) {

    TCanvas* cResiduals = new TCanvas("residuals", "Alignment residuals", 1000, 1000);
    cResiduals->Divide(2, 3);
    // gStyle->SetOptStat(111111);
    cResiduals->cd(1);
    xresidualatstart_localcopy->DrawCopy();
    cResiduals->cd(2);
    yresidualatstart_localcopy->DrawCopy();
    cResiduals->cd(3);
    xresidualatend_localcopy->DrawCopy();
    cResiduals->cd(4);
    yresidualatend_localcopy->DrawCopy();
    cResiduals->cd(5);
    rresidualatstart_localcopy->DrawCopy();
    cResiduals->cd(6);
    rresidualatend_localcopy->DrawCopy();
    cResiduals->Update();
  }

}


