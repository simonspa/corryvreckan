// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <cmath>

#include <boost/utility.hpp>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TString.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TF1.h"
#include "TFitResult.h"

#include "Clipboard.h"
#include "TrackTimestamper.h"
#include "TestBeamEventElement.h"
#include "TestBeamCluster.h"
#include "TestBeamTransform.h"
#include "LanGau.h"

using namespace std;
//-----------------------------------------------------------------------------
// Implementation file for class : TrackTimestamper
//
// Note there are three (sets of) loops in the run method for monitoring TDC information, TDC information associated to clusters (and hits) on the DUT, 
// and TDC information associated to tracks. 
// Tracks are only timestamped if an unambiguous TDC match occurs.
// If writeTimewalkFile in Parameters.C is true, I assume you are interested in more details of the output so more histograms will be produced, 
//along with text files listing the timewalks.
// If you really want all the gory detail switch on "dofit" to true to get a histogram of timewalks for each frame.
//
// 2012-06-28 Hamish Gordon
//-----------------------------------------------------------------------------

TrackTimestamper::TrackTimestamper(Parameters* p, bool d)
  : Algorithm("TrackTimestamper")
{
  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(),"READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);

}

TrackTimestamper::~TrackTimestamper() {} 
 
void TrackTimestamper::initial()
{
  bool isthereatoa = false;
  for (int i = 0; i < summary->nDetectors(); ++i) {
    if (parameters->toa[summary->detectorId(i)]) isthereatoa = true;
  }
  totalveto=0;
  clockperiod = parameters->clock; 
	hTdcToa = new TH1F("tdc_tim_of_arrival", "tdc_tim_of_arrival", 5000, 0, 500000);
  hTpxToa = new TH1F("tpx_tim_of_arrival", "tpx_tim_of_arrival", 5000, 0, 500000);

  hTimeBetweenTriggers = new TH1F("timebetweentriggers", "Time between consecutive triggers (ns)", 5000, 0, 50000);
  hTdcTriggers = new TH1F("tdctriggers", "TDC time of all triggers", 2000, 0, 800000);
  hTdcTrigPerFrame = new TH1F("numtdctriggersperframe", "Number of TDC triggers per frame", 300, 0, 300);
  hSyncDelay = new TH1F("sync delay", "sync delay", 1000, 0, 1000);
  hTDCShutter = new TH1F("shutter tim", "shutter tim", 10000, 0, 10000000);

  m_debug = false;
  if (!isthereatoa) return;
  dofit = false;
  centrewindow = -1.5;// if > -1, can require that track passes through a 1-pixel cluster. Negative values turn this off.
  passcwin=0;failcwin=0;
 globalEfficiencyDenominator = 0;
  inOverlapEfficiencyDenominator = 0;
  globalEfficiencyNumerator = 0;
  inOverlapEfficiencyNumerator = 0;
  const float xtwmin = -500;
  const float xtwmax = 500;
  //int nbins = (int)(xtwmax-xtwmin)/clockperiod;
  const int nbins = 1000;  
  for (int i = 0; i < summary->nDetectors(); ++i) {
		if(!parameters->toa[summary->detectorId(i)])continue;
    const std::string chip = summary->detectorId(i);
    TH1F* h1 = 0;
    TH2F* h2 = 0;
    //we only want to get these histograms for the chips that are in ToA mode.
    if (parameters->toa[summary->detectorId(i)]){
      std::string name = std::string("Timewalks ") + chip;
      h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times",nbins, xtwmin, xtwmax);
      hTimewalk.insert(make_pair(chip, h1));
      if (parameters->writeTimewalkFile){
	name = std::string("TimewalksBlack ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times", nbins, xtwmin, xtwmax);
	hTimewalkBlack.insert(make_pair(chip, h1));
	
	name = std::string("TimewalksWhite ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times", nbins, xtwmin, xtwmax);
	hTimewalkWhite.insert(make_pair(chip, h1));
	
	name = std::string("TimewalksTop ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times:top", nbins, xtwmin, xtwmax);
	hTimewalkTop.insert(make_pair(chip, h1));
	
	name = std::string("TimewalksBottom ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times:bottom", nbins, xtwmin, xtwmax);
	hTimewalkBottom.insert(make_pair(chip, h1));
	
	name = std::string("TimewalksLeft ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times:left", nbins, xtwmin, xtwmax);
	hTimewalkLeft.insert(make_pair(chip, h1));
	
	name = std::string("TimewalksRight ") + chip;
	h1 = new TH1F(name.c_str(), "Timewalks for clusters that are the best match to TDC times:right", nbins, xtwmin, xtwmax);
	hTimewalkRight.insert(make_pair(chip, h1));
      }
      name = std::string("ClusterCorrelationTimewalks ") + chip;
      h1 = new TH1F(name.c_str(), "Correlation plot: Timewalks for clusters matched to tracks", nbins, xtwmin, xtwmax);
      hTimewalkTrack.insert(make_pair(chip, h1));

      name = std::string("rowtimewalk ") + chip;
      h2 = new TH2F(name.c_str(), "rowtimewalk", 256, 0, 256, 150, -50, 250);
      hRowTimewalk.insert(make_pair(chip, h2));

      name = std::string("coltimewalk ") + chip;
      h2 = new TH2F(name.c_str(), "coltimewalk", 256, 0, 256, 150, -50, 250);
      hColTimewalk.insert(make_pair(chip, h2));

      name = std::string("TDC-TPX correlation ") + chip;
      h2 = new TH2F(name.c_str(), "TDC-TPX correlation", 200, 0, 300000, 200, 0, 300000);
      hCorrTdcTpx.insert(make_pair(chip, h2));

      name = std::string("hassoctracks ") + chip;
      h2 = new TH2F(name.c_str(), "Position of associated tracks on TOA plane", 100, -10, 10, 100, -10, 10);
      hAssocTracks.insert(make_pair(chip, h2));
    
      name = std::string("hnonassoctracks ") + chip;
      h2 = new TH2F(name.c_str(), "Position of non-associated tracks on TOA plane", 100, -10, 10, 100, -10, 10);
      hNonAssocTracks.insert(make_pair(chip, h2));

      name = std::string("tdctoaOverlapAssoc ") + chip;
      h2 = new TH2F(name.c_str(), "Position of associated clusters on TOA plane", 256, -10, 10, 256, -10, 10);
      hTdcToaOverlapAssoc.insert(make_pair(chip, h2));

      name = std::string("tdctoaOverlapNonAssoc ") + chip;
      h2 = new TH2F(name.c_str(), "Position of non-associated clusters on TOA plane", 256, -10, 10, 256, -10, 10);
      hTdcToaOverlapNonAssoc.insert(make_pair(chip, h2));
 
      name = std::string("timebetweentriggerspostveto ") + chip;
      h1 = new TH1F(name.c_str(), "Time between consecutive triggers (ns) after veto", 5000, 0, 50000);
      hTimeBetweenTriggersPostVeto.insert(make_pair(chip, h1));

      name = std::string("tdctoas ") + chip;
      h1 = new TH1F(name.c_str(), "TDC Time of arrival of tracks", 200, 0, 300000);
      hTdcToas.insert(make_pair(chip, h1));
      if(parameters->writeTimewalkFile){
	  name = std::string("timepixassoctoasblack ") + chip;
	  h1 = new TH1F(name.c_str(), "ToA of associated tracks on timepix: black", 200, 0, 300000);
	  hTimepixAssocToasBlack.insert(make_pair(chip, h1));
	  
	  name = std::string("timepixassoctoaswhite ") + chip;
	  h1 = new TH1F(name.c_str(), "ToA of associated tracks on timepix: white", 200, 0, 300000);
	  hTimepixAssocToasWhite.insert(make_pair(chip, h1));
	}
    }
    // htimepixnonassoctoas = new TH1F("timepixnonassoctoas", "ToA of non-associated tracks on timepix", 200, 0, 300000);
    // hrawtpxclustertoas = new TH1F("rawtpxclustertoas", "ToA of clusters on TOA plane", 200, 0, 30000);

  }
  
  hGood = new TH1F("goodframe", "goodframe", 1000000, 0, 1000000);
  if(dofit){
    hZeropoint = new TH1F("hzeropoint", "Landau start point", 200, -100,100);
    hRiseTime = new TH1F("hrisetime", "Landau rise time (3sigma)", 400, 0, 400);
    hMean = new TH1F("hmean", "Landau mean", 200, -100, 100);
  
    nHists = 16;
    for (int i = 0; i < nHists; ++i) {
      TH1F* h1 = 0;
      TString title = "timewalks_20x20box_";
      title += i;
      TString name = "timewalks_20x20box_";
      name += i;
      h1 = new TH1F(name, title, nbins, xtwmin, xtwmax);
      tws.insert(make_pair(i, h1));

      TString titleb = "timewalksblack_20x20box_";
      titleb += i;
      TString nameb ="timewalksblack_20x20box_";
      nameb += i;
      h1 = new TH1F(nameb, titleb, nbins, xtwmin, xtwmax);
      twsblack.insert(make_pair(i, h1));

      TString titlew = "timewalkswhite_20x20box_";
      titlew += i;
      TString namew ="timewalkswhite_20x20box_";
      namew += i;
      h1 = new TH1F(namew, titlew, nbins, xtwmin, xtwmax);
      twswhite.insert(make_pair(i, h1));
    }
  }
  
  // Initialise counters.
  nTracksAssocTotal = 0;
  nTracksNonAssocTotal = 0;
  nTriggersNonAssocTotal = 0;
  nframes=0;
  nGoodClusterFrames = 0;
  nGoodFrames = 0;
  nTracksAssocGood = 0;
  nTracksNonAssocGood = 0;
  
  nlines = 0;
  nerror = 0;

  if (parameters->writeTimewalkFile) {
    std::string fname = parameters->timewalkFile;
    std::string oname = std::string("1px_") + fname;
    std::ofstream fout(oname.c_str());
    fout << "timewalks: 1pixel clusters" << std::endl;
    fout.close();
    oname = std::string("2px_") + fname;
    std::ofstream fout2(oname.c_str());
    fout2 << "timewalks : 2 pixel clusters" << std::endl;
    fout2.close();
    oname = std::string("manypx_") + fname;
    std::ofstream fout3(oname.c_str());
    fout3 << "timewalks: many-pixel clusters" << std::endl;
    fout3.close();
  }
  totalSaturated=0;
  totalSaturatedInOverlap=0;
  noTOAcluster=0;
  noTrackClusters=0;
  screwedUpFrame=0;
  totalTDCTriggers=0;
}

bool sortUnsyncTriggers( TDCTrigger* t1, TDCTrigger* t2)
{
  return t1->timeAfterShutterOpen()-t1->syncDelay() < t2->timeAfterShutterOpen()-t2->syncDelay();
}


void TrackTimestamper::run(TestBeamEvent* event, Clipboard *clipboard)
{
  
  // Need to set DUT and TOA parameters before any of this code gets run.
  // If it is run on ToT runs, get a bit of TDC monitoring suitable for all runs.
  int vetoed=0;
  std::map<std::string, int> nTriggersAssoc;
  std::map<std::string, int> nClustersAssoc;
  std::map<std::string, int> nClustersNonAssoc;
  std::map<std::string, int> nTracksWithoutCluster;
  std::map<std::string, int> nTracksWithSaturatedCluster;
  std::map<std::string, int> nTracksAssocPerPlane;
  std::map<std::string, TestBeamCluster*> assocCluster;
  std::map<std::string, TDCTrigger*> assocTdcTrig;
  std::map<std::string, double> assocTdcTimestamp;
  std::map<std::string, double> assocToaTimestamp;
  int nToaPlanes = 0;
  for (int i = 0; i < summary->nDetectors(); i++) {
    const std::string dID = summary->detectorId(i);
    if (parameters->toa[dID]) {
      ++nToaPlanes;
      nTriggersAssoc[dID] = 0;
      nClustersAssoc[dID] = 0;
      nClustersNonAssoc[dID] = 0;
      nTracksWithoutCluster[dID] = 0;
      nTracksWithSaturatedCluster[dID] = 0;
      nTracksAssocPerPlane[dID] = 0;
      assocCluster[dID] = NULL;
      assocTdcTrig[dID] = NULL;
      assocTdcTimestamp[dID] = 0.;
      assocToaTimestamp[dID] = 0.;
    }
  }

  if (m_debug) std::cout << m_name << ": first loop (always run)" << std::endl;
  // First loop 
  // Look only at TDC and fill some very basic monitoring histograms: tdc triggers, time between tdc triggers, synchronisation delays.
  int nTriggers = 0;
  for (unsigned int j = 0; j < event->nTDCElements(); ++j) {
    TestBeamEventElement* ele = event->getTDCElement(j);
    std::string chip = ele->detectorId();
    TDCFrame* frame = (TDCFrame*)ele;
    TDCTriggers* triggers = frame->triggers();
    nTriggers += triggers->size();
    if (m_debug) {
      std::cout << m_name << ": TDC frame " << frame->positionInSpill()
                << " at " << std::cout.precision(15) << std::cout.width(18) << frame->timeStamp()
                << " with " << frame->nTriggersInFrame()
                << " triggers in " << int(frame->timeShutterIsOpen() / 1000)
                << "us (" << frame->nTriggersUnmatchedInSpill()
                << " mismatch in frame)" << std::endl;
    }
    hTDCShutter->Fill(frame->timeShutterIsOpen());
    hTdcTrigPerFrame->Fill(triggers->size());
    float previousTdc = 0.;
    for (TDCTriggers::iterator itr = triggers->begin(); itr != triggers->end(); ++itr) {
      float tdctrig = (*itr)->timeAfterShutterOpen() - (*itr)->syncDelay();
      if (tdctrig > frame->timeShutterIsOpen() - 700) continue;
      hTdcTriggers->Fill(tdctrig);
      hSyncDelay->Fill((*itr)->syncDelay());
      if (itr != triggers->begin()) hTimeBetweenTriggers->Fill(tdctrig - previousTdc);
      previousTdc = tdctrig;
    }
  }
  if (nToaPlanes < 1) {
    // Algorithm stops here if there is no ToA plane.
    return;
  }
  
  // Runs after 4000 seem to have a constant offset of -75ns in timewalk, may well be due to cable lengths. 
  // The offset for run 3589 is 0.
  const double delta = parameters->tdcOffset;
  // Second loop
  // For each cluster on the DUT in ToA find the associated TDC time, then go through the hits in the cluster and
  // find the timewalk (TDC-ToA) for each hit. Fill appropriate correlation histograms for clusters.
  // This loop is not essential and could be removed if efficiency is key
  // This is simpler than the next loop because I don't refuse to associate clusters to TDC times if the TDC times are too close together.
  TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
  if (!clusters) {
    std::cerr << m_name << std::endl;
    std::cerr << "    no clusters on clipboard" << std::endl;
    return;
  }
  for (TestBeamClusters::iterator itc = clusters->begin(); itc < clusters->end(); ++itc) {
    TestBeamCluster* cl = *itc;
    const std::string dID = cl->detectorId();
    // Skip non-ToA planes.
    if (!parameters->toa[dID]) continue;
    // Get the ToA of this cluster.
    float toa = cl->totalADC();
    // Check for saturation.
    if (toa > 11800) continue;
    // hrawtpxclustertoas->Fill(toa);
    float toadiff = 100000000;
    bool isAssoc = false;
    float assocTdcTimestamp = 0;
    // Look for a matching TDC trigger.
    for (unsigned int j = 0; j < event->nTDCElements(); ++j) {
      TestBeamEventElement* ele = event->getTDCElement(j);
      TDCFrame* frame = (TDCFrame*)ele;
      float tpxtoa = frame->timeShutterIsOpen() - clockperiod * toa;
      TDCTriggers* triggers = frame->triggers();
      // The cluster should be associated with only one TDC trigger. 
      // Should choose the best one. Maybe leave this for a future refinement.
      for (TDCTriggers::iterator itr = triggers->begin(); itr != triggers->end(); ++itr) {
        float tdcTimestamp = (*itr)->timeAfterShutterOpen() - (*itr)->syncDelay();   
        if (tdcTimestamp > frame->timeShutterIsOpen() - 800) continue; // 563 plus some timewalk
        toadiff = tpxtoa - tdcTimestamp - delta;
        // Check if the TDC trigger is within the specified window.
        if (toadiff > -2. * clockperiod && toadiff < 8. * clockperiod) { 
          assocTdcTimestamp = tdcTimestamp;
          if (!isAssoc) {
            ++nTriggersAssoc[dID];
            isAssoc = true;
          } 
        }
      }
    }
    if (isAssoc) {
      hTdcToaOverlapAssoc[dID]->Fill(cl->globalX(), cl->globalY());
      ++nClustersAssoc[dID];
    } else {
      hTdcToaOverlapNonAssoc[dID]->Fill(cl->globalX(), cl->globalY());
      ++nClustersNonAssoc[dID];
    }
  }
  totalTDCTriggers+=nTriggers;
  if (nTriggers > 0 && m_debug) {
    std::cout << m_name << std::endl;
    for (int i = 0; i < summary->nDetectors(); ++i) {
      const std::string dID = summary->detectorId(i);
      if (!parameters->toa[dID]) continue;
      const double percentageTriggersAssoc = 100. * nTriggersAssoc[dID] / nTriggers;
      const double percentageClustersAssoc = 100. * nClustersAssoc[dID] / (nClustersAssoc[dID] + nClustersNonAssoc[dID]);
      std::cout << "    " << dID << ": " 
                << nTriggersAssoc[dID] << "/" << nTriggers 
                << " (" << percentageTriggersAssoc 
                << "\%) TDC triggers associated to a cluster" << std::endl;
      std::cout << "    " << dID << ": " 
                << nClustersAssoc[dID] << "/" << (nClustersAssoc[dID] + nClustersNonAssoc[dID])
                << " (" << percentageClustersAssoc
                << "\%) clusters associated to a TDC trigger" << std::endl;
    }
  }
  
  // Open output text files.
  std::ofstream output;
  std::ofstream output2;
  std::ofstream output3;
  if (parameters->writeTimewalkFile){
    std::string fname = parameters->timewalkFile;
    std::string oname = std::string("1px_") + fname;
    output.open(oname.c_str(), std::ios::app);
    if (!output.is_open()) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: could not open output file " << std::endl;
      ++nerror;
      return;
    }
    oname = std::string("2px_") + fname;
    output2.open(oname.c_str(), std::ios::app);
    if (!output2.is_open()) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: could not open output file " << std::endl;
      ++nerror;
      return;
    }
    oname = std::string("manypx_") + fname;
    output3.open(oname.c_str(), std::ios::app);
    if (!output3.is_open()) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: could not open output file " << std::endl;
      ++nerror;
      return;
    }
  }
  // Third loop
  // Do the same thing for tracks and add some monitoring to make plots of where matched and unmatched tracks intersect the DUT
  // This is where the timestamping happens.
  const float xmin = -6 * clockperiod;
  const float xmax = 16 * clockperiod;
  const int nBinsFrameTw = 4*int((xmax - xmin) / clockperiod);
  nframes++;
  TString ftwname= "frametws";
  ftwname+=nframes;
  TH1F* frametimewalks;
  if(dofit) frametimewalks = new TH1F(ftwname, "", nBinsFrameTw, xmin, xmax);//make bin width 1 clock cycle
  float maxTW = 16 * clockperiod; //dhynds for scifi data from 8 to 16
  
  TestBeamTracks* tracks = (TestBeamTracks*) clipboard->get("Tracks");
  if (!tracks) {
    std::cerr << m_name << std::endl;
    std::cerr << "    ERROR: no tracks on clipboard" << std::endl;
    return;
  }
  const int nTracks = tracks->size();
  int nTracksAssoc = 0; 
  const double window = parameters->trackwindow;
  bool donetbt = false;
  float shuttertime=0;
  for (TestBeamTracks::iterator it = tracks->begin(); it < tracks->end(); ++it) {
    //if (m_debug) std::cout << m_name << ": next track" << std::endl;
    TestBeamTrack* track = *it;
    TestBeamProtoTrack* proto = track->protoTrack();
    TestBeamClusters* trackclusters = proto->clusters();
    if (!trackclusters) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: no clusters on track" << std::endl;
      noTrackClusters++;
      return;
    }
    double supertoa =0;
    for (int i = 0; i < summary->nDetectors(); ++i) {
      std::string dID = summary->detectorId(i);
      // Skip non-ToA planes.
      if (!parameters->toa[dID]) continue;
      // Reset the associated ToA cluster.
      assocCluster[dID] = NULL;
      // Reset the associated TDC trigger.
      assocTdcTrig[dID] = NULL;
      assocTdcTimestamp[dID] = 0.;
      assocToaTimestamp[dID] = 0.;
      // Make sure there is an alignment.
      if (parameters->alignment.count(dID) <= 0) {
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: ToA plane " << dID << " has no alignment" << std::endl;
        continue;
      }
      // Find out where the track intersects the plane. 
      TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[dID]->displacementX(),
                                                             parameters->alignment[dID]->displacementY(),
                                                             parameters->alignment[dID]->displacementZ(),
                                                             parameters->alignment[dID]->rotationX(),
                                                             parameters->alignment[dID]->rotationY(),
                                                             parameters->alignment[dID]->rotationZ());
    
      PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
      PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
      PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
      PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
        
      const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
      const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
      const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
    
      const float length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
                            (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
                            (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
        (track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
      const float x_inter = track->firstState()->X() + length * track->direction()->X();
      const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
      const float z_inter = track->firstState()->Z() + length * track->direction()->Z();    
      // Change to local coordinates of that plane.
      PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
      PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
      //get local intersects which enables one to then select timewalks based on whether the track went through
      float x_inter_local = intersect_local.X()+128*0.055;
      float y_inter_local = intersect_local.Y()+128*0.055;
      delete mytransform;

      // When DuT is the ToA plane and it's not in pattern recognition or 
      // if the plane is excluded from the pattern recognition,
      // we have to associate clusters to tracks manually. What a pain.
      if ((!parameters->dutinpatternrecognition && dID == parameters->dut) ||
          parameters->excludedFromPatternRecognition[dID]) {
        TestBeamClusters* allclusters = (TestBeamClusters*)clipboard->get("Clusters");
        if (!allclusters) {
          std::cerr << m_name << std::endl;
          std::cerr << "    ERROR: no clusters on clipboard" << std::endl;
          return;
        }
        if (allclusters->size() == 0) {
          std::cerr << m_name << std::endl;
          std::cerr << "    ERROR: no clusters on clipboard" << std::endl;
          return;
        }
        // Filter out the clusters in this plane.
        std::vector<std::pair<TestBeamCluster*, int> > tmpclusters;
        for (TestBeamClusters::iterator clit = allclusters->begin(); clit < allclusters->end(); ++clit) {
          if ((*clit)->detectorId() == dID) {
            tmpclusters.push_back(std::make_pair((*clit), 0));
          }
        }
        // Loop over the clusters in this plane.
        for (std::vector<std::pair<TestBeamCluster*, int> >::iterator kk = tmpclusters.begin(); kk < tmpclusters.end(); ++kk) {
          if (fabs(((*kk).first)->globalX() - intersect_global.X()) < window && 
              fabs(((*kk).first)->globalY() - intersect_global.Y()) < window && (*kk).second == 0) {
            (*kk).second = 1;
            assocCluster[dID] = (*kk).first;
            if (m_debug) {
              std::cout << m_name << ": associated chessboard cluster to track" << std::endl;
              std::cout << "    Will not bother to look if there are other clusters which might be better!" << std::endl;
            }
            break;
          }
        }
      }else {
	//if the DUT is in the pattern recognition it's easier.
        for (TestBeamClusters::iterator itct = trackclusters->begin(); itct < trackclusters->end(); ++itct) {
          if (!parameters->toa[(*itct)->detectorId()]) continue;
          if ((*itct)->detectorId() == dID) {
            assocCluster[dID] = *itct;
          }
        }
      }
      if (!assocCluster[dID]) {
        ++nTracksWithoutCluster[dID];
        if (m_debug) std::cout << m_name << ": no ToA cluster on track" << std::endl; 
	noTOAcluster++;
        continue;
      }
      double toa = assocCluster[dID]->totalADC();
      if(supertoa==0) supertoa=toa;
      else std::cout << " have two ToA planes, weird..." << endl;
      // if (m_debug) std::cout << m_name << ": ToA is " << toa * clockperiod << std::endl;
      if (toa > 11800) {
        ++nTracksWithSaturatedCluster[dID];
	totalSaturated++;
	if(intersect_local.X() < 5.0 && intersect_local.X() > -5.0 && intersect_local.Y() < 3.5 && intersect_local.Y() > -6) totalSaturatedInOverlap++;
        if (m_debug) std::cout << m_name << ": ToA value in saturation" << std::endl;
        continue;
      } else if (toa == 0) {
        std::cout << m_name << ": raw ToA is zero, problem!!!" << std::endl;
      }

      bool supervetoflag = false;
      float tpxtoa = 0;
      float toadiff = 100000000;
      // float rawtpxtoa = toa;
      std::vector<RowColumnEntry*>* hitsincluster = assocCluster[dID]->hits();

      // Now we should have a cluster on the track with non-saturated ToA value.
      // Next, look for a matching TDC trigger.
      float shtime = 0.;
      for (unsigned int j = 0; j < event->nTDCElements(); ++j) {
        TestBeamEventElement* ele = event->getTDCElement(j);
        TDCFrame* frame = (TDCFrame*)ele;
        // During amalgamation, triggers are sorted by synchronised value.
        // Since I use unsync triggers I need to sort them by unsync value in 
        // order to ensure triggers are a minimum of 200ns apart.
        frame->sortUnsync();
        bool vetoflag = false; 
        tpxtoa = frame->timeShutterIsOpen() - clockperiod * toa;
	hTpxToa->Fill(tpxtoa);//dhynds
	//				std::cout<<"Timepix toa is "<<tpxtoa<<std::endl;//dhynds
				
				
				
          
        // Only one ToA should be associated to each TDC value. 
        // Choose the closest one. This does not mean every ToA will be correctly associated to its TDC hit; 
        // it causes a bias towards low timewalk.
        // This potential bias is minimised by vetoing triggers and associated tracks that are less than 150ns apart.
        float prevtdctoa = -1000000;
        TDCTriggers* triggers = frame->triggers();
        for (TDCTriggers::iterator itr = triggers->begin(); itr != triggers->end(); ++itr) {
          vetoflag = false;
          TDCTrigger* tdctrig = *itr;
          float tdctoaWithoutSyncDelay = tdctrig->timeAfterShutterOpen();
          float syncdelay = tdctrig->syncDelay();   
          float tdctoa = tdctrig->timeAfterShutterOpen() - syncdelay; 
					hTdcToa->Fill(tdctoa);//dhynds
//					std::cout<<"TDC toa is "<<tdctoa<<std::endl;//dhynds

          if (tdctoa < prevtdctoa) {
            std::cerr << "TrackTimestamper: mis-ordering of triggers in frame" << std::endl; 
	    screwedUpFrame+=tracks->size();
            return;
          }
          float nexttdctoa = 10000000;
          float nexttdctoaWithoutSyncDelay = 10000000;
          float nextsyncdelay = 10000000;
          if (boost::next(itr) != triggers->end()) {
            TDCTrigger* nexttrig = *(boost::next(itr));
            nexttdctoaWithoutSyncDelay = nexttrig->timeAfterShutterOpen();
            nextsyncdelay = nexttrig->syncDelay();
            nexttdctoa = nexttrig->timeAfterShutterOpen() - nexttrig->syncDelay();
            boost::prior(nexttrig);
          }                                                               
          if (parameters->dut != "SciFi" && tdctoa > nexttdctoa) {
            std::cerr << m_name << std::endl;
            std::cerr << "    mis-ordering of triggers in frame";
            std::cerr << "    TDC: " << tdctoa << ", next:  " << nexttdctoa << std::endl;
            std::cerr << "    TDC (without synchronisation delay): " << tdctoaWithoutSyncDelay << ", next: " << nexttdctoaWithoutSyncDelay << std::endl;
            std::cerr << "    Synchronisation delay: " << syncdelay << ", next: " << nextsyncdelay << std::endl;
            if (tdctoaWithoutSyncDelay < nexttdctoaWithoutSyncDelay) {
              std::cerr << "    Unlucky due to synchronisation delays. Hope this doesn't happen often." << std::endl;
            }
            float moretdctoa = 0.;
            if (boost::next(itr) != triggers->end()) {
              TDCTrigger* nexttrig2 = *(boost::next(itr));
              moretdctoa = nexttrig2->timeAfterShutterOpen() - nexttrig2->syncDelay();
              boost::prior(nexttrig2);
            }
            std::cerr << "    Next to next TDC: " << moretdctoa << std::endl;
	    screwedUpFrame+=tracks->size();
         //dhynds   return;
          }
          // if (it == tracks->begin() && m_debug) {
          //   std::cout << m_name << ": previous TDC = " << prevtdctoa
          //                       << ", current TDC = " << tdctoa
          //                       << ", next TDC = " << nexttdctoa << std::endl;
          // }
          if (tdctoa - prevtdctoa < 2.5 * clockperiod || 
              nexttdctoa - tdctoa < 2.5 * clockperiod) { //dhynds changed from 6 to 2 and Hamish increase back to 2.5.
            vetoflag = true;
          } else {
            if (!donetbt) {
              hTimeBetweenTriggersPostVeto[dID]->Fill(tdctoa - prevtdctoa);
            }
          }
          prevtdctoa = tdctoa;
          if (tdctoa > frame->timeShutterIsOpen() - 700) {
	    //if(m_debug) std::cout<<"Veto near end of frame: tdctoa = "<< tdctoa << " vs " << frame->timeShutterIsOpen() - 700<<std::endl;//dhynds
            vetoflag = true;
          }
          hTimewalkTrack[dID]->Fill(tpxtoa - tdctoa - delta);
          // TDC has to fire before timepix (give or take 2 clock cycles to account for chessboard effect)
          if (tpxtoa - tdctoa - delta > -2 * clockperiod && 
              tpxtoa - tdctoa - delta < toadiff) {
//						std::cout<<"New candidate"<<std::endl;//dhynds
            // Found a new candidate TDC trigger.
            // Annoying complicated way to veto tracks that are associated to triggers that are too close together.
            if (vetoflag) {
              supervetoflag = true;
	      vetoed++;
            } else {
              supervetoflag = false;
	      
            }
            toadiff = tpxtoa - tdctoa - delta;
            assocTdcTimestamp[dID] = tdctoa;
            assocToaTimestamp[dID] = tpxtoa;
            assocTdcTrig[dID] = tdctrig;
            shtime = frame->timeShutterIsOpen();
          } 
        }
        if (!donetbt) donetbt = true;
      } 
      //Completed loop over all TDC elements. Have a candidate timewalk, "toadiff" and an indication of whether or not it is good, "supervetoflag"
      shuttertime = shtime;
      //Now we know if we can timestamp the track.
      //if (m_debug) std::cout << m_name << ": minimum time difference " << toadiff << std::endl;
      // Check if we found a good TDC candidate and 
      // plot intersection points of tracks that have, and do not have,
      // associated TDC times (to test spatial position of TDC)
      if(nToaPlanes==1){ 
	globalEfficiencyDenominator++;
	if(intersect_local.X() < 5.0 && intersect_local.X() > -5.0 && intersect_local.Y() < 3.5 && intersect_local.Y() > -6) inOverlapEfficiencyDenominator++;
      }
      if (!assocTdcTrig[dID]) {
        if (m_debug) std::cout << m_name << ": no TDC candidate found" << std::endl;
          hNonAssocTracks[dID]->Fill(intersect_local.X(), intersect_local.Y());
      } else if (toadiff > maxTW) { 
//				std::cout<<"Time walk too large at "<<toadiff/1000<<" us"<<std::endl; //dhynds
        if (m_debug) {
          std::cout << m_name << ": best TDC candidate is " << toadiff 
                    << " ns before toa candidate, implausibly long timewalk, no good! " << std::endl;
        }
	
        hNonAssocTracks[dID]->Fill(intersect_local.X(), intersect_local.Y());
        assocTdcTrig[dID] = NULL;
      } else if (supervetoflag) {
	//either the TDC hits are too close together so we don't know which one to associate to a track, or the time of arrival
	// is too close to the end of the shutter
        if (m_debug) {
          std::cout << m_name << ": TDC candidate was vetoed" << std::endl;
        }
		//		std::cout<<"TDC vetoes for some reason"<<std::endl; //dhynds
        hNonAssocTracks[dID]->Fill(intersect_local.X(), intersect_local.Y());
        assocTdcTrig[dID] = NULL;
      } else {
	//Track is good. Timestamp it. Because there might be more than one plane in ToA, 
	//need to exit the loop over planes before associating timestamp to tracks on the clipbaord.
        ++nTracksAssocPerPlane[dID];
        hTdcToas[dID]->Fill(assocTdcTimestamp[dID]);
        hAssocTracks[dID]->Fill(intersect_local.X(), intersect_local.Y());
	if(nToaPlanes==1){
	  globalEfficiencyNumerator++;
	  if(intersect_local.X() < 5.0 && intersect_local.X() > -5.0 && intersect_local.Y() < 3.5 && intersect_local.Y() > -6) inOverlapEfficiencyNumerator++;
	}

	//Timestamping is done. Now loop over clusters in track for timewalk analysis.
        for (RowColumnEntries::iterator ith = hitsincluster->begin(); ith < hitsincluster->end(); ++ith) {
          float hittoa = (float)(*ith)->value();
          // Could apply chessboard correction to individual hits used in timewalk plot
          float hittoadiff = shtime - clockperiod * hittoa - assocTdcTimestamp[dID] - delta;
          if (hittoadiff > -6 * clockperiod && 
              hittoadiff < 16 * clockperiod && dofit) {
            frametimewalks->Fill(hittoadiff);
          }
	  if(hitsincluster->size() ==1 && centrewindow > -1){
	      //having found 1-pixel cluster associated to track calculate exactly where the track hit the DUT and optionally reject 1-pixel clusters where the track 
	      //doesn't fall precisely into a window around the cluster centre. Need to work out efficiency of this requirement. 
	    if(fabs((*ith)->column()*0.055-x_inter_local)> centrewindow || fabs((*ith)->row()*0.055 -y_inter_local) > centrewindow){
	      if(m_debug) cout << "Track is not inside pixel window: x hit, track = "<< (*ith)->column() <<" , "<< x_inter_local/0.055 <<" y hit, track, "<< (*ith)->row()<<", " <<y_inter_local/0.055 << endl;
	      failcwin++;
	      continue;
	    }else{
	      if(m_debug) cout << "Track is inside pixel window: displacement = "<< (*ith)->column()*0.055-x_inter_local <<","<< (*ith)->row()*0.055-y_inter_local << endl;
	      if(m_debug) cout << "Track is inside pixel window: x hit, track = "<< (*ith)->column() <<" , "<< x_inter_local/0.055 <<" y hit, track, "<< (*ith)->row()<<", " <<y_inter_local/0.055 << endl;
	      passcwin++;
	    }
	  }
          hTimewalk[dID]->Fill(hittoadiff);
          hRowTimewalk[dID]->Fill((*ith)->row(), hittoadiff);
          hColTimewalk[dID]->Fill((*ith)->column(), hittoadiff);
	  if(dofit){
	    std::cout <<"cluster hit coordinates "<< (*ith)->row() << "," << (*ith)->column() << std::endl;
	    for (int i = 0; i < nHists; ++i) {
	      if ((*ith)->row() >= 64 * (i / 4) && (*ith)->row() < 64 * (i / 4) + 32) {
		if (i % 4 == 0 && (*ith)->column() >= 32 && (*ith)->column() < 64) {
		  tws[i]->Fill(hittoadiff);
		  if ((((int)(*ith)->row()) % 2 == 0 && ((int)(*ith)->column()) % 2 == 0) ||
		      (((int)(*ith)->row()) % 2 == 1 && ((int)(*ith)->column()) % 2 == 1)) {
		    twsblack[i]->Fill(hittoadiff);
		  } else {
		    twswhite[i]->Fill(hittoadiff);
		  }
		} else if (i % 4 == 1 && (*ith)->column() >= 96 && (*ith)->column() < 128) {
		  tws[i]->Fill(hittoadiff);
		  if ((((int)(*ith)->row()) % 2 == 0 && ((int)(*ith)->column()) % 2 == 0) ||
		      (((int)(*ith)->row()) % 2 == 1 && ((int)(*ith)->column()) % 2 == 1)) {
		    twsblack[i]->Fill(hittoadiff);
		  } else {
		    twswhite[i]->Fill(hittoadiff);
		  }
		} else if (i % 4 == 2 && (*ith)->column() >= 160 && (*ith)->column() < 192) {
		  tws[i]->Fill(hittoadiff);
		  // cout << "found something in third group " << endl;
		  if ((((int)(*ith)->row()) % 2 == 0 && ((int)(*ith)->column()) % 2 == 0) ||
		      (((int)(*ith)->row()) % 2 == 1 && ((int)(*ith)->column()) % 2 == 1)) {
		    twsblack[i]->Fill(hittoadiff);
		  } else {
		    twswhite[i]->Fill(hittoadiff);
		  }
		} else if (i % 4 == 3 && (*ith)->column() >= 224 && (*ith)->column() < 256) {
		  tws[i]->Fill(hittoadiff);
		  if ((((int)(*ith)->row()) % 2 == 0 && ((int)(*ith)->column()) % 2 == 0) ||
		      (((int)(*ith)->row()) % 2 == 1 && ((int)(*ith)->column()) % 2 == 1)) {
		    twsblack[i]->Fill(hittoadiff);
		  } else {
		    twswhite[i]->Fill(hittoadiff);
		  }
		}
	      }
	    }
	  }
          // Also fill timewalk histograms for "black" and "white" squares
	  if(parameters->writeTimewalkFile){
	    if ((((int)(*ith)->row()) % 2 == 0 && ((int)(*ith)->column()) % 2 == 0) ||
		(((int)(*ith)->row()) % 2 == 1 && ((int)(*ith)->column()) % 2 == 1)) {
	      hTimewalkBlack[dID]->Fill(hittoadiff);
	      hTimepixAssocToasBlack[dID]->Fill(tpxtoa);
	    } else { 
	      hTimewalkWhite[dID]->Fill(hittoadiff);
	      hTimepixAssocToasWhite[dID]->Fill(tpxtoa);
	    }
	    if ((*ith)->row() > 128) {
	      hTimewalkBottom[dID]->Fill(hittoadiff);
	    } else {
	      hTimewalkTop[dID]->Fill(hittoadiff);
	    }
	    if ((*ith)->column() > 170) {
	      hTimewalkRight[dID]->Fill(hittoadiff);
	    } else {
	      hTimewalkLeft[dID]->Fill(hittoadiff);
	    }
	    if (hittoadiff > -4 * clockperiod && hittoadiff < 8 * clockperiod) {
	      if(hitsincluster->size() ==1){
		output << hittoadiff << std::endl;
		nlines++;
	      }else if(hitsincluster->size() ==2) output2 << hittoadiff << std::endl;
	      else if(hitsincluster->size() >2) output3 << hittoadiff << std::endl;
	    }
	  }
	}
      }
      if (parameters->writeTimewalkFile) {
        if(hitsincluster->size() ==1){
	  output << "-"<< std::endl;
	  nlines++;
	}else if(hitsincluster->size() ==2) output2 << "-"<< std::endl;
	else if(hitsincluster->size() >2) output3 << "-"<< std::endl;
      }
    }
    //Check consistency of timestamps between planes for the case where more than one plane is in ToA mode
    TDCTrigger* assocTdcTrigTrack = NULL;
    double assocTdcTimestampTrack = 0.;
    double assocToaTimestampTrack = 0.;
    for (int i = 0; i < summary->nDetectors(); ++i) {
      const std::string dID = summary->detectorId(i);
      if (!parameters->toa[dID]) continue;
      if (assocTdcTrig[dID]) {
        if (assocTdcTrigTrack) {
          if (assocTdcTrig[dID] != assocTdcTrigTrack) {
            if (parameters->verbose) {
               std::cout << "    ambigous associations of TDC triggers to track" << std::endl;
            }
						std::cout<<"Ambiguous??"<<std::endl;
          }
        } else {
          assocTdcTrigTrack = assocTdcTrig[dID];
          assocTdcTimestampTrack = assocTdcTimestamp[dID];
          assocToaTimestampTrack = assocToaTimestamp[dID];
        }
      }
    }

    //Finally apply the timestamps to the tracks on the clipboard.
    if (assocTdcTrigTrack) {
      track->setTDCTrigger(assocTdcTrigTrack);
      track->tdctimestamp(assocTdcTimestampTrack + delta);
      track->toatimestamp(assocToaTimestampTrack);
      ++nTracksAssoc;
      //if (m_debug) std::cout << m_name << ": successfully associated TDC trigger to track" << std::endl;
    } else {
      if (m_debug) {
        std::cout << m_name << ": failed to match track w/ ToA "<< supertoa <<" to TDC trigger, cf shtime"<< shuttertime << std::endl;
      }
    } 
  }  
  //Do some extra monitoring to get timestamping efficiency.
  const int nTracksNonAssoc = nTracks - nTracksAssoc;
  const double percentageTracksAssoc = 100. * nTracksAssoc / nTracks;
  if (parameters->verbose) {
    std::cout << m_name << std::endl;
    std::cout << "    time-tagged " << nTracksAssoc << "/" << nTracks 
              << " tracks (" << percentageTracksAssoc << "\%)" << std::endl;  
    for (int i = 0; i < summary->nDetectors(); ++i) {
      std::string dID = summary->detectorId(i);
      if (!parameters->toa[dID]) continue;
      const double percentageTracksAssocPerPlane = 100. * nTracksAssocPerPlane[dID] / nTracks;
      if (nToaPlanes > 1) {
        std::cout << "    " << nTracksAssocPerPlane[dID] << "/" << nTracks 
                  << " (" << percentageTracksAssocPerPlane 
                  << "\%) time-tagging efficiency on plane " << dID << std::endl;
      }
    }
  }
 
  if (parameters->writeTimewalkFile) {
    output << "----" << std::endl;
    output2 << "----" << std::endl;
    output3 << "----" << std::endl;
    ++nlines;
    output.close();
    output2.close();
    output3.close();
  }
  
  // Update the overall statistics.
  nTracksAssocTotal += nTracksAssoc;
  nTracksNonAssocTotal += nTracksNonAssoc;
  nTriggersNonAssocTotal += (nTriggers - nTracksNonAssoc);

  if (percentageTracksAssoc > 50.) {
    ++nGoodFrames;
    nTracksAssocGood += nTracksAssoc;
    nTracksNonAssocGood += nTracks - nTracksAssoc;
  }
          
  if (m_debug) std::cout << m_name << ": looping over TDC elements\n";
  for (unsigned int j = 0; j < event->nTDCElements(); ++j) {
    TestBeamEventElement* ele = event->getTDCElement(j);
    TDCFrame* frame = (TDCFrame*)ele;
    time += frame->timeShutterIsOpen() / 10000.;
    int bin = (int)time;
    if (percentageTracksAssoc > 50.) {
      hGood->SetBinContent(bin, nTracks);
    } else {
      hGood->SetBinContent(bin, -1 * nTracks);
    }
  }

  //Make histogram of timewalks for each frame and store it: careful not to run this over 5000 frames! 
  // Each histogram is fitted with a landau but often the statistics will be too low for this to work
  // The idea is to investigate the frame-to-frame jitter.
  if (nTracksAssoc > 0 && dofit) {
    TF1* fitfcn = new TF1("fitfcn", "landau(0)", -120, 300);
    fitfcn->SetParLimits(0, 0.0, 500.0);
    fitfcn->SetParLimits(1, -100.0, 100.0);
    fitfcn->SetParLimits(2, 0.0, 80.0);
    fitfcn->SetParameter(0, 80.0);
    fitfcn->SetParameter(1, 20.0);
    fitfcn->SetParameter(2, 20.0);
    hMapFrameTimewalks[nframes] = frametimewalks;
    std::cout << frametimewalks->GetEntries() << " " 
              << frametimewalks->GetMaximum() << " " 
              << frametimewalks->GetMaximumBin() << std::endl;
    TFitResultPtr fpr = frametimewalks->Fit("fitfcn", "NSRB", "", -120, 300); 
    float zp = fpr->Parameter(1) - 2 * fpr->Parameter(2);
    float rt = 3 * fpr->Parameter(2);
    if (fpr->Parameter(2) < 1.5 * fpr->ParError(2) || fpr->Parameter(0) > 450 || fpr->Parameter(1) < 1.5 * fpr->ParError(1)) {
      std::cout << "bad fit, will not add results to histogram" << std::endl;
    } else if (fpr->Parameter(2) == 80) { 
      std::cout << "silly width, will not add results to histograms" << std::endl;
    } else {
      hZeropoint->Fill(zp);
      hRiseTime->Fill(rt);
      hMean->Fill(fpr->Parameter(1));
    }
    delete fitfcn;
  }
  
  // TestBeamTracks* twtracks=(TestBeamTracks*)clipboard->get("Tracks");
  // if (twtracks == NULL) return;
  // for(TestBeamTracks::iterator it=twtracks->begin();it!=twtracks->end();it++){
  //   TestBeamTrack* twtrack= *it;
  //   if(it==twtracks->begin()) cout <<"Timestamp of track in TimewalkCalculator " << twtrack->tdctimestamp()<<endl;
  //   TDCTrigger *trig = twtrack->tdcTrigger();
  //   if(trig==NULL){
  //     cout << "NULL trigger, somehow an extra track has slipped through the net. Execute post-hoc repair. If this happens often need to sort it out!"<<endl;
     
  //   }
  //   cout << " Got trigger from track: time after shutter open = " << trig->timeAfterShutterOpen() << endl;
  // }
  if(m_debug) cout << " vetoed = " << vetoed << " triggers for being too close together" << endl;
  totalveto+=vetoed;
}

void TrackTimestamper::end()
{
  int ntoa=0;
  bool isthereatoa = false;
  for (int i = 0; i < summary->nDetectors(); i++) {
    if (parameters->toa[summary->detectorId(i)]){ 
      isthereatoa = true;
      ntoa++;
    }
  }
  if (!isthereatoa) return;
  
  // Print statistics for overall run.
  std::cout << m_name << std::endl;
  std::cout << "    " << nTracksAssocTotal << "/" << (nTracksAssocTotal + nTracksNonAssocTotal) << " tracks associated to TDC triggers" << std::endl;
  std::cout << "    " << nGoodFrames << " good frames with " 
            << nTracksAssocGood << " associated tracks and " 
            << nTracksNonAssocGood << " nonassociated tracks " << std::endl;
  // std::cout << "would have associated " << nsat << " saturated ToA times to TDC times, but actually these were filtered out" << std::endl;
  // std::cout << "Clusters on ToA plane: assoc " << assocCL << " nonassoc " << nonassocCL << " nonoverlap " << nonOverlapCL << std::endl;
  cout << "    vetoed " << totalveto << " out of " << totalTDCTriggers <<" triggers for being too close together" << endl;
  if(centrewindow > -1) std::cout << "Passed cut on pixel centre window for 1 pixel clusters: " << passcwin <<", failed: " << failcwin<< endl;
  // Check consistency of output text file. 
  if (parameters->writeTimewalkFile) {
    std::string fname = parameters->timewalkFile;
    std::string oname = std::string("1px_") + fname;
    std::ifstream ifs(oname.c_str(), std::ifstream::in);
    int nl = 0;
    char str[255];
    while (ifs) {
      ifs.getline(str, 255);  // delim defaults to '\n'
      if (ifs) nl++;
    }
    ifs.close();
    std::cout << m_name << std::endl;
    std::cout << "     expected number of lines: " << nlines << std::endl;
    std::cout << "     the file actually has " << nl << "lines." << std::endl;
    std::cout << "     there were " << nerror << " errors in the output" << std::endl;
  } 
  if(ntoa==1){
    std::cout << "    Hamish x-check: timestamped " << globalEfficiencyNumerator<<"/"<<globalEfficiencyDenominator+totalSaturated<<" = " << 100.0*globalEfficiencyNumerator/((double) globalEfficiencyDenominator+totalSaturated)<<"%"<<std::endl;
    std::cout << "    In fiducial region: timestamped " << inOverlapEfficiencyNumerator<<"/"<<inOverlapEfficiencyDenominator+totalSaturatedInOverlap<<" = " << 100.0*inOverlapEfficiencyNumerator/((double) inOverlapEfficiencyDenominator+totalSaturatedInOverlap)<<"%"<<std::endl;
    cout << "    No TOA cluster on " << noTOAcluster << " tracks " << endl;
    cout << "    Saturated TOA cluster on "<< totalSaturated << " tracks, of which " << totalSaturatedInOverlap<<" are in the scintillator overlap " << endl;
    if(m_debug) cout << "    Number of prototracks without any clusters = " << noTrackClusters << endl;
    cout << "    frames with misordering in triggers have " << screwedUpFrame << " tracks" << endl;
  }
}
