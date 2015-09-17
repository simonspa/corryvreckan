#include <TStopwatch.h>

#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <algorithm>

#include <unistd.h>

#include "TFile.h"
#include "TROOT.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TRandom.h"
#include "TH2.h"
#include "TStyle.h"

#include "LanGau.h"

// local
#include "NewVetraClusterMaker.h"
#include "VetraMapping.h"
#include "VetraNoise.h"
#include "TestBeamProtoTrack.h"
#include "Clipboard.h"

using namespace std;

// static variables
int NewVetraClusterMaker::nTotalFrames;
int NewVetraClusterMaker::nFramesWithTDCinfo;
int NewVetraClusterMaker::nTDCElements;
int NewVetraClusterMaker::nMatchedTDCElements;
int NewVetraClusterMaker::nTotalTriggers;
int NewVetraClusterMaker::nValidTriggers;
int NewVetraClusterMaker::removeChannelsAffectedXTalk;

float NewVetraClusterMaker::globalAccumulatedCharge[200];
float NewVetraClusterMaker::globalAccumulatedEntries[200];
float NewVetraClusterMaker::accumulatedCharge[200];
float NewVetraClusterMaker::accumulatedEntries[200];
float NewVetraClusterMaker::globalAccumulatedChargeMod25[25];
float NewVetraClusterMaker::globalAccumulatedEntriesMod25[25];
float NewVetraClusterMaker::accumulatedChargeMod25[25];
float NewVetraClusterMaker::accumulatedEntriesMod25[25];
float NewVetraClusterMaker::globalChargePerSyncDelay[200];
float NewVetraClusterMaker::globalEntriesPerSyncDelay[200];
VetraMapping NewVetraClusterMaker::vetraReorder;
VetraNoise NewVetraClusterMaker::vetraNoise;

std::vector <int> NewVetraClusterMaker::stripsStartRange;
std::vector <int> NewVetraClusterMaker::stripsEndRange;



//-----------------------------------------------------------------------------
// Implementation file for class : NewVetraClusterMaker
//
// 2010-06-02 : Pablo Rodriguez
//-----------------------------------------------------------------------------

NewVetraClusterMaker::~NewVetraClusterMaker(){
}

void NewVetraClusterMaker::initClusterCuts(){
  if(getenv("SEEDTHRESHOLD")!=NULL)
    clusterSeedCut=atoi(getenv("SEEDTHRESHOLD"));// seedCut = noise*clusterSeedCut
  else
    clusterSeedCut=6;
  if(getenv("INCLUSIONTHRESHOLD")!=NULL)
    clusterInclusionCut=atoi(getenv("INCLUSIONTHRESHOLD")); // inclusionCut = noise*clusterInclusionCut
  else
    clusterInclusionCut=3;

  clusterTotalChargeCut=9;


  //   clusterInclusionCut = clusterInclusionCut/2;

  if(getenv("SYNCDELAYLOWCUT")!=NULL)
    syncDelayLowCutRegion=atoi(getenv("SYNCDELAYLOWCUT"));
  else
    syncDelayLowCutRegion=75;
  if(getenv("SYNCDELAYHIGHCUT")!=NULL)
    syncDelayHighCutRegion=atoi(getenv("SYNCDELAYHIGHCUT"));
  else
    syncDelayHighCutRegion=87;

  cout<<" NewVetraClusterMaker:  selected SyncDelayCut from "<<syncDelayLowCutRegion<<" to "<<syncDelayHighCutRegion<<endl;

  timeWindowVetraTrack=2; //nanosec
  spatialWindowVetraTrack=0.120; //milimeters

  stripWindowLowCut=0;
  stripWindowHighCut=2000;

  rejectedStrips.clear();

  removeChannelsAffectedXTalk=1;

  efficiency_trackNr=0;
  efficiency_clusterNr_1000um=0;
  efficiency_clusterNr_500um=0;
  efficiency_clusterNr_200um=0;
  ratioCounter_m2=0;
  ratioCounter_m1=0;
  ratioCounter_p1=0;
  ratioCounter_p2=0;
  ratioCounter_m2_15=0;
  ratioCounter_m1_15=0;
  ratioCounter_p1_15=0;
  ratioCounter_p2_15=0;
  ratioCounter_m2_30=0;
  ratioCounter_m1_30=0;
  ratioCounter_p1_30=0;
  ratioCounter_p2_30=0;

}


NewVetraClusterMaker::NewVetraClusterMaker(Parameters* p, bool d) : Algorithm("NewVetraClusterMaker") {
  parameters=p;
  display=d;

  //   if(p->dut=="D0")
  //     etaDistributionFile="aux/D0_etaDistribution.root";


  initClusterCuts();
}

NewVetraClusterMaker::NewVetraClusterMaker() {
  initClusterCuts();
  nValidTriggers = 0;
  nTotalFrames = 0;
  nFramesWithTDCinfo = 0;
  nTotalTriggers = 0;
  nTDCElements = 0;
  nMatchedTDCElements = 0;

  for(int i=0;i<200;i++){
    globalAccumulatedCharge[i]=0;
    globalAccumulatedEntries[i]=0;
    accumulatedCharge[i]=0;
    accumulatedEntries[i]=0;
    globalChargePerSyncDelay[i]=0;
    globalEntriesPerSyncDelay[i]=0;
  }
  for(int i=0;i<25;i++){
    globalAccumulatedChargeMod25[i]=0;
    globalAccumulatedEntriesMod25[i]=0;
    accumulatedChargeMod25[i]=0;
    accumulatedEntriesMod25[i]=0;
  }
}

 

void NewVetraClusterMaker::initial()
{
  std::cout << "Call to NewVetraClusterMaker::initial"<<std::endl;

  m_transform = new TestBeamTransform( parameters->alignment[parameters->dut]->displacementX(),
				       parameters->alignment[parameters->dut]->displacementY(),
				       parameters->alignment[parameters->dut]->displacementZ(),
				       parameters->alignment[parameters->dut]->rotationX(),
				       parameters->alignment[parameters->dut]->rotationY(),
				       parameters->alignment[parameters->dut]->rotationZ());
    
  PositionVector3D< Cartesian3D<double> > tempPlanePoint0(0,0,0);
  PositionVector3D< Cartesian3D<double> > tempPlanePoint1(0,0,1);

  m_planePointLocalCoords=tempPlanePoint0;
  m_planePointGlobalCoords = (m_transform->localToGlobalTransform())*m_planePointLocalCoords;
  m_planePoint2LocalCoords=tempPlanePoint1;
  m_planePoint2GlobalCoords = (m_transform->localToGlobalTransform())*m_planePoint2LocalCoords;
						
  m_dut_normal_x=m_planePoint2GlobalCoords.X()-m_planePointGlobalCoords.X();
  m_dut_normal_y=m_planePoint2GlobalCoords.Y()-m_planePointGlobalCoords.Y();
  m_dut_normal_z=m_planePoint2GlobalCoords.Z()-m_planePointGlobalCoords.Z();


  vetraReorder.createMapping(parameters);

  defineHistograms();

  vetraReorder.createMapping(parameters);

  vetraNoise.readNoise(parameters->dut);

  fillRejectedStripsArray();

  eventIndex=parameters->firstEvent;


  //-----------  Fill noise histograms  ------------------------
  int strip;
  string dut=parameters->dut;

  for(int iTell1Ch=0;iTell1Ch<2048;iTell1Ch++){
    strip = vetraReorder.getStripFromTell1ch(iTell1Ch);
    float noise = vetraNoise.getNoiseFromStrip[strip];
    if (noise<=0.5) continue;
    sensorNoiseResiduals->Fill(noise);
    sensorNoiseDistributionH->Fill(strip, noise);
    tell1ChNoiseDistributionH->Fill(iTell1Ch, vetraNoise.getNoiseFromTell1Ch[iTell1Ch]);
  }
  TF1* noiseFit=new TF1("n1", "gaus", 0.2, 3.0);
  sensorNoiseResiduals->Fit(noiseFit, "QR");
  float mean = noiseFit->GetParameter(1);
  float sigma = noiseFit->GetParameter(2);
  float noiseCut = mean + 2*sigma;
  if(noiseCut<1.2) noiseCut=1.2;
  std::cout<<"  noiseCut = "<<noiseCut<<std::endl;
  for(int iTell1Ch=0;iTell1Ch<2048;iTell1Ch++){
    if( vetraNoise.getNoiseFromStrip[iTell1Ch] > noiseCut){
      rejectedStrips.push_back(vetraReorder.getStripFromTell1ch(iTell1Ch));
    }
  }
  //-------------------------------------------------------------


  std::cout << "  clusterSeedCut = "<< clusterSeedCut<< " times the noise of the strip"<<std::endl;
  std::cout << "  clusterInclusionCut = "<<clusterInclusionCut<<" times the noise of the strip"<<std::endl;

  std::cout << "NewVetraClusterMaker::initial finished"<<std::endl;
  std::cout << " "<<std::endl;

  m_debug=false;
}


void NewVetraClusterMaker::fillRejectedStripsArray(){
  if(m_debug) cout<<"NewVetraClusterMaker::fillRejectedStripsArray running"<<endl;
  int strip;
  string dut=parameters->dut;

  if(dut=="PR01"){
    //border routing lines
    std::vector <int> borderRL;
    borderRL.push_back(559);
    borderRL.push_back(568);
    borderRL.push_back(879);
    borderRL.push_back(900);
    borderRL.push_back(495);
    borderRL.push_back(558);
    borderRL.push_back(109);
    borderRL.push_back(128);
    borderRL.push_back(129);
    borderRL.push_back(236);
    borderRL.push_back(773);
    borderRL.push_back(878);
    borderRL.push_back(441);
    borderRL.push_back(494);

    for(int i=0;i<(int)borderRL.size();i++){
      strip = vetraReorder.getStripFromRoutingLine(borderRL.at(i));
      rejectedStrips.push_back(strip);
    }

    //   //masked strips
    //   rejectedStrips.push_back(9);
    //   rejectedStrips.push_back(84);
    //   rejectedStrips.push_back(414);
    //   rejectedStrips.push_back(491);

    //non bonded routing lines
    std::vector <int> noBondedRL;
    noBondedRL.push_back(798);
    noBondedRL.push_back(799);
    noBondedRL.push_back(802);
    noBondedRL.push_back(803);
    noBondedRL.push_back(779);
    noBondedRL.push_back(783);
    noBondedRL.push_back(796);
    noBondedRL.push_back(809);
    noBondedRL.push_back(808);
    noBondedRL.push_back(827);
    noBondedRL.push_back(830);
    noBondedRL.push_back(857);
    noBondedRL.push_back(854);
    noBondedRL.push_back(872);
    noBondedRL.push_back(896);
    noBondedRL.push_back(897);
    noBondedRL.push_back(898);

    for(int i=879; i<887;i++)
      noBondedRL.push_back(i);

    noBondedRL.push_back(443);
    noBondedRL.push_back(456);
    noBondedRL.push_back(461);
    noBondedRL.push_back(471);
    noBondedRL.push_back(495);
    noBondedRL.push_back(548);
    noBondedRL.push_back(550);
    noBondedRL.push_back(552);
    noBondedRL.push_back(557);

    for(int i=487;i<495;i++)
      noBondedRL.push_back(i);

    noBondedRL.push_back(123);
    noBondedRL.push_back(148);
    noBondedRL.push_back(189);
    noBondedRL.push_back(224);

    for(int i=0;i<(int)noBondedRL.size();i++){
      strip = vetraReorder.getStripFromRoutingLine(noBondedRL.at(i));
      rejectedStrips.push_back(strip);
    }
    rejectedStrips.push_back(367); //tbOct2011
    rejectedStrips.push_back(368); //tbOct2011
  }
  else if(dut=="BCB"){
    //strips selected after visual inspection of the sensor seed distribution plot
    for(int i=0;i<14;i++)
      rejectedStrips.push_back(i);
    rejectedStrips.push_back(29);
    rejectedStrips.push_back(47);
    rejectedStrips.push_back(48);
    rejectedStrips.push_back(49);
    rejectedStrips.push_back(58);
    rejectedStrips.push_back(61);
    for(int i=117;i<2048;i++)
      rejectedStrips.push_back(i);
  }
  else if(dut=="D0"){
    rejectedStrips.push_back(vetraReorder.getStripFromTell1ch(1883));
    rejectedStrips.push_back(vetraReorder.getStripFromTell1ch(1884));
    rejectedStrips.push_back(vetraReorder.getStripFromTell1ch(1885));
    rejectedStrips.push_back(vetraReorder.getStripFromTell1ch(1886));
  }
  else cout<<"Not rejected strips found for this DUT="<<dut<<endl;

  //Here we remove the channels affected by cross talk
  if(removeChannelsAffectedXTalk){
    std::sort(rejectedStrips.begin(), rejectedStrips.end());
    int xtalkCh;
    for(int iTell1Ch=0; iTell1Ch<2048;iTell1Ch++){
      xtalkCh=iTell1Ch%32;
      if(xtalkCh < 3){
	strip=vetraReorder.getStripFromTell1ch(iTell1Ch);
	if(strip>0) rejectedStrips.push_back(strip);
      }
    }
    std::sort(rejectedStrips.begin(), rejectedStrips.end());
    vector<int>::iterator itr=std::unique(rejectedStrips.begin(), rejectedStrips.end()); //to eliminate duplicities 
    rejectedStrips.resize(itr - rejectedStrips.begin());

  }

  std::sort(rejectedStrips.begin(), rejectedStrips.end());
  if(m_debug) cout<<"NewVetraClusterMaker::fillRejectedStripsArray finished"<<endl;
}



void NewVetraClusterMaker::run(TestBeamEvent *event, Clipboard *clipboard)
{
  cout<<endl<<"     ------ NewVetraClusterAnalysis -------"<<endl;
  eventIndex++;

  int seedsInEvent=0;
  int hitsInEvent=0;
  int hitsInMatchedTriggerEvent=0;

  if(event->nTDCElements() == 0) return;
  nFramesWithTDCinfo ++;
  int nTriggers=0;
  int nVetraHits=0;

  if(m_debug) std::cout << "We are running in NewVetraClusterMaker now"<<std::endl;
  TStopwatch watch; 
  string dut=parameters->dut;
  nTotalFrames ++;


  TestBeamTracks *tracks=(TestBeamTracks*)clipboard->get("Tracks");
  if(tracks == NULL) {
    std::cout<<"  ::  No tracks in this frame"<<std::endl;
    return;
  }
  int validTracksInFrame=0;
  int onlyToaTracksInFrame=0;
  for(TestBeamTracks::iterator iTr=tracks->begin();iTr!=tracks->end();iTr++){
    if(((*iTr)->toatimestamp()!=0)&&((*iTr)->tdctimestamp()!=0)){
      validTracksInFrame++;
    }
    else if(((*iTr)->toatimestamp()!=0)&&((*iTr)->tdctimestamp()==0)){
      onlyToaTracksInFrame++;
    }
  }
  onlyToaTracksInFrameH->Fill(onlyToaTracksInFrame);
  emptyTracksInFrameH->Fill(tracks->size()-validTracksInFrame-onlyToaTracksInFrame);
  totalTracksInFrameH->Fill(tracks->size());
  tracksInEvent->Fill(eventIndex, tracks->size());
  validTracksInEvent->Fill(eventIndex, validTracksInFrame);
  if(validTracksInFrame!=0)
    validTracksInFrameH->Fill(validTracksInFrame);

  if(m_debug){
    cout<<"=============  event elements  ==============="<<endl;
    cout<<" nTDCElements = "<<event->nTDCElements()<<endl;
    cout<<" nElements="<<event->nElements()<<endl;
    cout<<" nElementsWithHits="<<event->nElementsWithHits()<<endl;
    cout<<" nMediPixElements="<<event->nMediPixElements()<<endl;
    cout<<" nTimePixElements="<<event->nTimePixElements()<<endl;
    cout<<"=============================================="<<endl;
  }

  std::vector<VetraCluster*>* vetraClustersInFrame = NULL;
  vetraClustersInFrame = new std::vector<VetraCluster*>;
  vetraClustersInFrame->clear();

  std::vector <std::pair <int, int > > candidates;
  TDCFrame* frame;
  TDCTriggers* triggers;
  int strip, tell1Ch, thisTrigger=0;
  int seed = -1;

  nTDCElementsH->Fill(event->nTDCElements());

  std::vector<TestBeamTrack*>* alignedTracks;
  alignedTracks = new std::vector<TestBeamTrack*>;

  float temp_efficiency_matchedTrigger=0;

  for(int iTDCel=0; iTDCel<(int)event->nTDCElements();iTDCel++){
    frame= event->getTDCElement(iTDCel);
    triggersInFrameH->Fill(frame->nTriggersInFrame());
    triggers=frame->triggers();
    nTriggers = triggers->size();

    if(m_debug) cout <<" "<<nTriggers<<" triggers in this TDCElement"<<endl;
    thisTrigger=0;
    seed=-1;
    nTDCElements ++;
    //We reject those frames which number of triggers is not the same in the Telescope and in the TDC
    //     if(frame->nTriggersUnmatchedInSpill()) 
    //       continue; 

    nMatchedTDCElements ++;


    for(TDCTriggers::iterator iTrigger=triggers->begin();iTrigger!=triggers->end();iTrigger++) {
//       vetraHitsInEvent->Fill(eventIndex, (*iTrigger)->nVetraHits());
      hitsInEvent += (*iTrigger)->nVetraHits();
    }

    for(TestBeamTracks::iterator iTrack=tracks->begin();iTrack!=tracks->end();iTrack++){
      //       // Cut in chi2
      //       if((*iTrack)->chi2()>0.08) {
      //       cout<<" track->chi2() = "<<(*iTrack)->chi2()<<endl;
      //       continue;
      //       }

      TestBeamTrack *track = new TestBeamTrack(**iTrack, CLONE);
      //get intersection point of track with Sensor
      float length=((m_planePointGlobalCoords.X()-track->firstState()->X())*m_dut_normal_x+
		    (m_planePointGlobalCoords.Y()-track->firstState()->Y())*m_dut_normal_y+
		    (m_planePointGlobalCoords.Z()-track->firstState()->Z())*m_dut_normal_z)/
	(track->direction()->X()*m_dut_normal_x+track->direction()->Y()*m_dut_normal_y+track->direction()->Z()*m_dut_normal_z);
      float xintercept=track->firstState()->X()+length*track->direction()->X();
      float yintercept=track->firstState()->Y()+length*track->direction()->Y();
      float zintercept=track->firstState()->Z()+length*track->direction()->Z();
      telescopesHitmap->Fill(xintercept, yintercept);
      //change to local coordinates of that plane
      PositionVector3D< Cartesian3D<double> > intersect_global(xintercept,yintercept,zintercept);
      PositionVector3D< Cartesian3D<double> > intersect_local = (m_transform->globalToLocalTransform())*intersect_global;
      float x_inter_local=intersect_local.X();
      float y_inter_local=intersect_local.Y();

       for(TDCTriggers::iterator iTrigger=triggers->begin();iTrigger!=triggers->end();iTrigger++){
	for(int ih=0;ih<(*iTrigger)->nVetraHits();ih++){
	  int adcValue=(*iTrigger)->vetraADC(ih);
	  tell1Ch = (*iTrigger)->vetraLink(ih)*32+(*iTrigger)->vetraChannel(ih);
	  
	  //Cut:: we only want channels between 1536 to 2048. This is valid for PR01, BCB, D0 and 3D
	  if(tell1Ch < 1536) continue;
	  strip = vetraReorder.getStripFromTell1ch(tell1Ch);
	  double position = vetraReorder.StripToPosition[strip];
	  if(fabs(position - x_inter_local)<0.12)
	    adcAdjacentStrips->Fill(adcValue);
	}
      }


      if(track->tdctimestamp()==0.0) 
	continue;
      diffToaTrackTdcTrackH->Fill(track->toatimestamp()-track->tdctimestamp());
      for(TDCTriggers::iterator iTrigger=triggers->begin();iTrigger!=triggers->end();iTrigger++){
	nTotalTriggers ++;
	diffTdcTrackTdcVetraH->Fill(track->tdctimestamp()-(*iTrigger)->timeAfterShutterOpen()+(*iTrigger)->syncDelay());

	// Temporal Matching
	if(fabs(track->tdctimestamp()-(*iTrigger)->timeAfterShutterOpen()+(*iTrigger)->syncDelay()) > timeWindowVetraTrack) // around 2 nanosecs
	  continue;


	hitsInMatchedTriggerEvent += (*iTrigger)->nVetraHits();
// 	vetraHitsInMatchedTriggerInEvent->Fill(eventIndex, (*iTrigger)->nVetraHits());


	// track counter for efficiency
	if(((int)(*iTrigger)->syncDelay() < syncDelayLowCutRegion )||((int)(*iTrigger)->syncDelay() > syncDelayHighCutRegion)) {
	  continue;  // we are going to use only triggers with a "good" delay
	}
	if(((xintercept > -4.6) && (xintercept < -3.8)) ||
	   ((xintercept > -2.6) && (xintercept < -1.8)) ||
	   ((xintercept > -0.8) && (xintercept <  0.2)) ||
	   ((xintercept >  1.2) && (xintercept <  2.0)) ||
	   ((xintercept >  3.0) && (xintercept <  3.8)))
	  temp_efficiency_matchedTrigger++;

	if(m_debug){
	  cout<<""<<endl;
	  cout<<" ------------- New Trigger --------------"<<endl;
	}

	telescopesHitmapTimeMatchedWithTrigger->Fill(xintercept, yintercept);

	vetraHitsPerTriggerH->Fill((*iTrigger)->nVetraHits());
	syncDelayTriggerGlobal->Fill((*iTrigger)->syncDelay());

	nVetraHits += (*iTrigger)->nVetraHits();

	if(m_debug) cout<<"nVetraHits = "<<(*iTrigger)->nVetraHits()<<endl;
    
	bool * stripAlreadyInCluster;
	seed=0;
	candidates.clear();
	std::vector <std::pair <int, int > > strip_adc;
	for(int ih=0;ih<(*iTrigger)->nVetraHits();ih++){
	  int adcValue=(*iTrigger)->vetraADC(ih);
	  //We change the polarity of the ADCvalue only for D0 device
	  if(dut!="D0" && adcValue<0) continue;
	  else if(dut=="D0"){
	    if (adcValue>0) continue;
	    adcValue = -1*adcValue; // because D0 is PonN while PR01 and BCB are NonP
	  }

	  tell1Ch = (*iTrigger)->vetraLink(ih)*32+(*iTrigger)->vetraChannel(ih);

	  if(m_debug){
	    cout<<"tell1Ch = "<<tell1Ch<<"  strip = "<<vetraReorder.getStripFromTell1ch(tell1Ch)<<"  adc = "<<adcValue;
	    cout.precision(2);
	    cout<<"  noise = "<<vetraNoise.getNoiseFromStrip[vetraReorder.getStripFromTell1ch(tell1Ch)];
	    if(adcValue > (vetraNoise.getNoiseFromStrip[vetraReorder.getStripFromTell1ch(tell1Ch)]*clusterInclusionCut))
	      cout<<"  (inclusion)";
	    if(adcValue > (vetraNoise.getNoiseFromStrip[vetraReorder.getStripFromTell1ch(tell1Ch)]*clusterSeedCut))
	      cout<<"  (seed)";
	    cout<<""<<endl;
	  }

	  //Cut:: we only want channels between 1536 to 2048. This is valid for PR01, BCB, D0 and 3D
	  if(tell1Ch < 1536) {
	    if(m_debug) cout<<"**Rejected hit in tell1Ch = "<<tell1Ch<<endl;
	    continue;
	  }

	  strip = vetraReorder.getStripFromTell1ch(tell1Ch);
	  correlationDUTTrack->Fill(vetraReorder.StripToPosition[strip]-x_inter_local);

	  if(find(rejectedStrips.begin(), rejectedStrips.end(), strip) != rejectedStrips.end()) {
	    if(m_debug) cout<<"**Strip "<<strip<<" belongs to rejectedStrips group"<<endl;
	    continue;
	  }

	  if(strip < 0) {
	    if(m_debug) cout<<"**Rejected strip:"<<strip<<"  which correspond to Tell1 channel:"<<tell1Ch<<endl;
	    continue; //not bonded channels are assigned to strip=-1
	  }
	  strip_adc.push_back(make_pair(strip,adcValue));
	  // -----  Filling some histograms -----
	  initialAdcVsStrip->Fill(strip, adcValue);
	  if(((int)(*iTrigger)->syncDelay() > syncDelayLowCutRegion)&&((int)(*iTrigger)->syncDelay() < syncDelayHighCutRegion))
	    landauBeforeClustering->Fill(adcValue);
	  landauBeforeClusteringWithoutCut->Fill(adcValue);
	  syncDelayBeforeClustering->Fill((*iTrigger)->syncDelay());
	  //Channel occupancy before clustering
	  vetraChannelH->Fill(tell1Ch);
	  vetraStripH->Fill(strip);

	  //---------------------------------------

	  globalAccumulatedChargeMod25[(int)(*iTrigger)->syncDelay()%25] += adcValue;
	  globalAccumulatedEntriesMod25[(int)(*iTrigger)->syncDelay()%25]++;
	  globalAccumulatedCharge[(int)(*iTrigger)->syncDelay()] += adcValue;
	  globalAccumulatedEntries[(int)(*iTrigger)->syncDelay()]++;
	  globalChargePerSyncDelay[(int)(*iTrigger)->syncDelay()] += adcValue;
	  globalEntriesPerSyncDelay[(int)(*iTrigger)->syncDelay()]++;

	  // check if the adc value is high enough to belong to a cluster
	  if((adcValue) < (vetraNoise.getNoiseFromStrip[strip]*clusterInclusionCut)) continue; 
	  if(strip>stripWindowLowCut && strip<stripWindowHighCut){
	    candidates.push_back(make_pair(strip, adcValue));
	    if(m_debug) cout<<"  candidate-> <"<<strip<<","<<adcValue<<">"<<"  candidates.size()="<<candidates.size()<<endl;
	    if(adcValue > vetraNoise.getNoiseFromStrip[strip]*clusterSeedCut){
	      vetraClusterSeedH->Fill(strip);
	      seed ++; //at least we got one seed
	      seedsInEvent ++;
	    }
	  }
	} //loop over hits in trigger

	// Fill Impulse histogram
	std::sort(strip_adc.begin(), strip_adc.end());
	for(vector<pair<int, int> >::iterator iHit=strip_adc.begin(); iHit!=strip_adc.end(); iHit++){
	  float stripCenter=(*iHit).first*0.06;
	  if(fabs(x_inter_local-stripCenter)<0.006){
	    checkImpulseADC_central->Fill((*iHit).second);
	    checkImpulseCenter->Fill(0.,(*iHit).second);
	    for(vector<pair<int, int> >::iterator iHit2=strip_adc.begin(); iHit2!=strip_adc.end(); iHit2++){
	      if(((*iHit2).first-(*iHit).first)==-2) {
		ratioAdcNeighbor_m2->Fill((float)(*iHit2).second/(*iHit).second);
		checkImpulseCenter->Fill(-2.,(*iHit2).second);
		checkImpulseADC_m2->Fill((*iHit2).second);
	      }
	      else if(((*iHit2).first-(*iHit).first)==-1) {
		ratioAdcNeighbor_m1->Fill((float)(*iHit2).second/(*iHit).second);
		checkImpulseCenter->Fill(-1.,(*iHit2).second);
		checkImpulseADC_m1->Fill((*iHit2).second);
	      }
	      else if(((*iHit2).first-(*iHit).first)==1) {
		ratioAdcNeighbor_p1->Fill((float)(*iHit2).second/(*iHit).second);
		checkImpulseCenter->Fill(1.,(*iHit2).second);
		checkImpulseADC_p1->Fill((*iHit2).second);
	      }
	      else if(((*iHit2).first-(*iHit).first)==2) {
		ratioAdcNeighbor_p2->Fill((float)(*iHit2).second/(*iHit).second);
		checkImpulseCenter->Fill(2.,(*iHit2).second);
		checkImpulseADC_p2->Fill((*iHit2).second);
	      }
	    }
	  }
	  if(fabs(x_inter_local-stripCenter+0.015)<0.006){
	    for(vector<pair<int, int> >::iterator iHit2=strip_adc.begin(); iHit2!=strip_adc.end(); iHit2++){
	      if(((*iHit2).first-(*iHit).first)==-2) ratioAdcNeighbor_m2_15->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==-1) ratioAdcNeighbor_m1_15->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==1) ratioAdcNeighbor_p1_15->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==2) ratioAdcNeighbor_p2_15->Fill((float)(*iHit2).second/(*iHit).second);
	    }
	  }
	  if(fabs(x_inter_local-stripCenter+0.030)<0.006){
	    for(vector<pair<int, int> >::iterator iHit2=strip_adc.begin(); iHit2!=strip_adc.end(); iHit2++){
	      if(((*iHit2).first-(*iHit).first)==-2) ratioAdcNeighbor_m2_30->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==-1) ratioAdcNeighbor_m1_30->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==1) ratioAdcNeighbor_p1_30->Fill((float)(*iHit2).second/(*iHit).second);
	      else if(((*iHit2).first-(*iHit).first)==2) ratioAdcNeighbor_p2_30->Fill((float)(*iHit2).second/(*iHit).second);
	    }
	  }

	}


	
	if(candidates.size()==0) continue; //empty trigger.
	if(seed == 0) continue; //if we don't have any seed in this trigger
	//for the PR01, around 25% of the triggers exceed this condition

	//Sort the strips in order to find consecutive strips
	if(m_debug){
	  for(vector<pair<int, int> >::iterator iCand=candidates.begin(); iCand!=candidates.end(); iCand++)
	    cout<<"  1-> <"<<(*iCand).first<<","<<(*iCand).second<<">"<<"  candidates.size()="<<candidates.size()<<endl;
	}
	std::sort(candidates.begin(), candidates.end());
	if(m_debug){
	  for(vector<pair<int, int> >::iterator iCand=candidates.begin(); iCand!=candidates.end(); iCand++)
	    cout<<"  2-> <"<<(*iCand).first<<","<<(*iCand).second<<">"<<"  candidates.size()="<<candidates.size()<<endl;
	}

	if(candidates.size()==0) {
	  cout<<"FATAL: Strip candidates disapeared during the sorting algorithm!!!"<<endl;
	  continue;  //paranoia check
	}

	if(((int)(*iTrigger)->syncDelay() < syncDelayLowCutRegion )||((int)(*iTrigger)->syncDelay() > syncDelayHighCutRegion)) {
	  continue;  // we are going to use only triggers with a "good" delay
	}

	stripAlreadyInCluster = new bool[candidates.size()];
	for(int i=0;i<(int)candidates.size();i++){
	  stripAlreadyInCluster[i]=false;
	}
      
	// //Exceptions after sorting (only for PR01) 
	if(dut=="PR01"){
	  int found191=0, found383=0, inverseReordering=0;
	  for(int iCandS=0; iCandS<(int)candidates.size();iCandS++){
	    if(strip == 191) found191=candidates.at(iCandS).second;
	    if(strip == 383) found383=candidates.at(iCandS).second;
	  }
	  for(int iCandS=0; iCandS<(int)candidates.size();iCandS++){
	    strip = candidates.at(iCandS).first;
	    //Case1: if we got a seed in the strip 384, we must select which is his neighbour strip: 383 or 191
	    if((strip == 384)&&(candidates.at(iCandS).second>clusterSeedCut*vetraNoise.getNoiseFromStrip[384])){
	      if(found191>found383) inverseReordering=1;
	    }
	    //Case2: if there is a seed in strips 189, 190 or 191
	    if(strip>188 && strip<192){
	      if((candidates.at(iCandS).second>clusterSeedCut*vetraNoise.getNoiseFromStrip[candidates.at(iCandS).first])&&
		 found191>found383){
		inverseReordering=1;
	      }
	    }
	  }
	  // 	  if(inverseReordering){
	  // 	    std::cout << "Swap region1-region2 condition achieved for trigger: " << nValidTriggers<<std::endl;
	  // 	    for(int iCandS=(int)candidates.size()-1;iCandS>=0; iCandS--){
	  // 	      if((candidates.at(iCandS).first>191)&&(candidates.at(iCandS).first<384)){
	  // 		candidates.insert(stripCandidates.begin(), stripCandidates.at(iCandS));
	  // 		stripCandidates.erase(stripCandidates.begin()+iCandS+1);
	  // 		adcCandidates.insert(adcCandidates.begin(), adcCandidates.at(iCandS));
	  // 		adcCandidates.erase(adcCandidates.begin()+iCandS+1);
	  // 		t1ChCandidates.insert(t1ChCandidates.begin(), t1ChCandidates.at(iCandS));
	  // 		t1ChCandidates.erase(t1ChCandidates.begin()+iCandS+1);
	  // 	      }
	  // 	    }
	  // 	  }

	  if(m_debug){
	    if(seed > 1){
	      std::cout<<std::endl;
	      std::cout<<"Candidatos: "<<std::endl;
	      for(int stripIdx=0; stripIdx<(int)candidates.size(); stripIdx++){
		for(int rl=0;rl<2048;rl++){
		  if(vetraReorder.getStripFromRoutingLine(rl) == candidates.at(stripIdx).first)
		    std::cout<<"    routingLine = "<<rl<<"  strip="<<candidates.at(stripIdx).first<<"  ADC="<<candidates.at(stripIdx).second<<std::endl;
		}
	      } 
	    }
	  }
	}  //end of PR01's exception

	if(m_debug) {
	  cout<<" -------- Clustering Algorithm  ------------"<<endl;
	  cout<<" Candidates:"<<endl;
	  std::vector <std::pair <int, int > >::iterator iCand=candidates.begin();
	  for(; iCand != candidates.end();iCand++) {
	    cout<<"    strip = "<<(*iCand).first<<"  adc = "<<(*iCand).second;
	    cout.precision(3);
	    cout<<"  position = "<<vetraReorder.StripToPosition[(*iCand).first];
	    if((*iCand).second>vetraNoise.getNoiseFromStrip[(*iCand).first]*clusterSeedCut)
	      cout<<" <- seed";
	    cout<<""<<endl;
	  }
	  cout<<""<<endl;
	}

	for(int stripIdx=0; stripIdx<(int)candidates.size(); stripIdx++){ //Note that all candidates have passed the inclusion cut
	  if(stripAlreadyInCluster[stripIdx]) continue;
	  int stripMinus1 = 0, stripPlus1 = 0, stripPlus2 = 0;
	  strip = candidates.at(stripIdx).first;

	  int adcValue = candidates.at(stripIdx).second;
	  if(stripIdx>0){
	    if((!stripAlreadyInCluster[stripIdx-1])&&
	       (candidates.at(stripIdx).first - candidates.at(stripIdx-1).first == 1)) 
	      stripMinus1=1;
	  }
	  if(stripIdx < ((int)candidates.size()-1)){	
	    if((!stripAlreadyInCluster[stripIdx+1])&&
	       (candidates.at(stripIdx+1).first - candidates.at(stripIdx).first == 1)) 
	      stripPlus1=1;
	  }
	  if(stripIdx < ((int)candidates.size()-2)){
	    if((!stripAlreadyInCluster[stripIdx+2])&&
	       (candidates.at(stripIdx+2).first - candidates.at(stripIdx).first == 2)) 
	      stripPlus2=1;
	  }
	  // 	std::cout <<" --------------   Strip: "<<strip<<"/"<<adcCandidates.at(stripIdx)<<"  -------------" << std::endl;
	  if(adcValue > vetraNoise.getNoiseFromStrip[strip]*clusterSeedCut){
	    VetraCluster *newVetraCluster = new VetraCluster();
	    newVetraCluster->setEtaDistributionFile(etaDistributionFile);
	    newVetraCluster->addStripToVetraCluster(strip, adcValue, adcValue/vetraNoise.getNoiseFromStrip[strip], vetraReorder.StripToPosition[strip]);
	    newVetraCluster->setTriggerVetraCluster(thisTrigger);
	    newVetraCluster->setTimestamp(frame->timeStamp(), (*iTrigger)->timeAfterShutterOpen());
	    newVetraCluster->setVetraTimeBeforeShutterClose(frame->timeShutterIsOpen()-(*iTrigger)->timeAfterShutterOpen());
	    newVetraCluster->setSyncDelay((int)((*iTrigger)->syncDelay()));
	    stripAlreadyInCluster[stripIdx] = true;
	    //  	    cout<<"timestamp = "<<frame->timeStamp()<<"  timeAfterShutterOpen = "<<(*iTrigger)->timeAfterShutterOpen()<<"  clock="<<(*iTrigger)->clock()<<endl;
//  	    cout<<"    SeedFound: strip = "<<strip<<"  adc = "<<adcValue<<endl;
	   
	    //case0: previous strip belongs to cluster
	    if(stripMinus1){    // if are consecutive
	      newVetraCluster->addStripToVetraCluster(candidates.at(stripIdx-1).first, candidates.at(stripIdx-1).second, 
						      candidates.at(stripIdx-1).second/vetraNoise.getNoiseFromStrip[candidates.at(stripIdx-1).first], 
						      vetraReorder.StripToPosition[candidates.at(stripIdx-1).first]);
	      stripAlreadyInCluster[stripIdx-1]=true;
	      // 		std::cout<<"Case0: includind previous strip to cluster. Previous strip = "<<candidates.at(stripIdx-1).first<<std::endl;
	    }
	  
	    if(stripPlus1) { //if nexts strip is consecutive
	      //case1: next strip also exceed the seed threshold 
	      newVetraCluster->addStripToVetraCluster(candidates.at(stripIdx+1).first, candidates.at(stripIdx+1).second, 
						      candidates.at(stripIdx+1).second/vetraNoise.getNoiseFromStrip[candidates.at(stripIdx+1).first], 
						      vetraReorder.StripToPosition[candidates.at(stripIdx+1).first]);
	      stripAlreadyInCluster[stripIdx+1]=true;
	      if((candidates.at(stripIdx+1).second > vetraNoise.getNoiseFromStrip[candidates.at(stripIdx+1).first]*clusterSeedCut) && //next strip is also a seed
		 (stripPlus2)){		
		//	std::cout <<" Case1-1; next strip exceed seed threshold. Next strip = "<< strip+1<<std::endl;
		//case1.2: two next strips belongs to cluster
		newVetraCluster->addStripToVetraCluster(candidates.at(stripIdx+2).first, candidates.at(stripIdx+2).second, 
							candidates.at(stripIdx+2).second/vetraNoise.getNoiseFromStrip[candidates.at(stripIdx+2).first], 
							vetraReorder.StripToPosition[candidates.at(stripIdx+2).first]);
		stripAlreadyInCluster[stripIdx+2]=true;
		  // 		    std::cout << "Case1-2: posible cluster of 4 strips. Last strip = "<<strip+2<<std::endl;
	      }
	    }
	    // -------- End of clustering ---------------//

	    removeBorderClusters(newVetraCluster);
	    if(newVetraCluster->getSumAdcCluster() > clusterTotalChargeCut){

	      vetraClustersInFrame->push_back(newVetraCluster);
	      
	      if(m_debug) {
		cout<<"  New Cluster:" <<endl;
		for(unsigned int iStrip=0;iStrip<newVetraCluster->getStripsVetraCluster().size();iStrip++){
		  cout<<"    strip = "<<newVetraCluster->getStripsVetraCluster().at(iStrip)<<"  adc = "<<newVetraCluster->getADCVetraCluster().at(iStrip)<<endl;
		}
	      }

	      float newVetraClusterPosition=newVetraCluster->getVetraClusterPosition();

	      telescopesHitmapTimeMatchedWithVetraCluster->Fill(xintercept, yintercept);
	      
	      //Efficiency
	      if(((xintercept>-4.6) && (xintercept<-3.8)) ||
		 ((xintercept>-2.6) && (xintercept<-1.8)) ||
		 ((xintercept>-0.8) && (xintercept< 0.2)) ||
		 ((xintercept> 1.2) && (xintercept< 2.0)) ||
		 ((xintercept> 3.0) && (xintercept< 3.8))){
		if(fabs(x_inter_local-newVetraClusterPosition)<1) efficiency_clusterNr_1000um ++;
		if(fabs(x_inter_local-newVetraClusterPosition)<0.5) efficiency_clusterNr_500um ++;
		if(fabs(x_inter_local-newVetraClusterPosition)<0.2) efficiency_clusterNr_200um ++;
	      }



	      // 	    cout.precision(3);
	      // 	    cout<<"newVetraClusterPosition = "<<newVetraClusterPosition<<"  x_inter_local = "<<x_inter_local<<endl;

	      //------- checking spatial match -------------//
// 	      if((newVetraCluster->getClusterSize() == 2) && ((newVetraCluster->getStripsVetraCluster().at(0)%4 == 1)||
// 							     (newVetraCluster->getStripsVetraCluster().at(1)%4 == 1)||
// 							     (newVetraCluster->getStripsVetraCluster().at(0)%4 == 2)||
// 							     (newVetraCluster->getStripsVetraCluster().at(1)%4 == 2)) )
// 		continue;


	      if(fabs(newVetraClusterPosition-x_inter_local) < spatialWindowVetraTrack){ //in milimiters

		

		// -------- assign newVetraCluster to a track -------------//
		// 	      if(newVetraCluster->getClusterSize() > 1){
		track->addVetraAlignmentClusterToTrack(newVetraCluster);

		alignedTracks->push_back(track);
		// 	      }

		// Filling some histograms
		telescopesHitmapTimeAndSpatialMatchedWithVetraCluster->Fill(xintercept, yintercept);
		residualsVetraPositionTrackX->Fill(newVetraClusterPosition-x_inter_local);
		if(newVetraCluster->getClusterSize() == 1)
		  residualsClusterSize1->Fill(newVetraClusterPosition-x_inter_local);
		else if(newVetraCluster->getClusterSize() == 2){
		  etaDistribution->Fill(newVetraCluster->getEtaVariable());
		  etaInverse->Fill(newVetraCluster->getEtaInverse());
		  etaVsPosition->Fill(newVetraCluster->getVetraClusterPosition(), newVetraCluster->getEtaVariable());
		  residualsClusterSize2->Fill(newVetraClusterPosition-x_inter_local);
		  int leftStrip=newVetraCluster->getStripsVetraCluster().at(0);
		  etaVsTrackx->Fill((x_inter_local-leftStrip*0.06)/0.06, newVetraCluster->getEtaVariable());
		  cogVsTrackx->Fill((x_inter_local-leftStrip*0.06)/0.06, (newVetraClusterPosition-leftStrip*0.06)/0.06);
		  totalChargeVsTrackx->Fill((x_inter_local-leftStrip*0.06)/0.06, newVetraCluster->getSumAdcCluster());
		  chargeLeftVsTrackx->Fill((x_inter_local-leftStrip*0.06)/0.06, newVetraCluster->getADCVetraCluster().at(0));
		  chargeRightVsTrackx->Fill((x_inter_local-leftStrip*0.06)/0.06, newVetraCluster->getADCVetraCluster().at(1));
		  if((x_inter_local-leftStrip*0.06)/0.06 < 0.5) {
		    chargeLeftVsChargeRight_0_05->Fill(newVetraCluster->getADCVetraCluster().at(1),newVetraCluster->getADCVetraCluster().at(0));
		  }
		  if((x_inter_local-leftStrip*0.06)/0.06 > 0.5) {
		    chargeLeftVsChargeRight_05_1->Fill(newVetraCluster->getADCVetraCluster().at(1),newVetraCluster->getADCVetraCluster().at(0));
		  }
		}

		vetraPositionMatchedClusterH->Fill(newVetraClusterPosition, 1.);
		if(newVetraCluster->getClusterSize()==1)
		  vetraPositionMatchedCluster1H->Fill(newVetraClusterPosition, 1.);
		else if(newVetraCluster->getClusterSize()==2)
		  vetraPositionMatchedCluster2H->Fill(newVetraClusterPosition, 1.);

		

		float distTrackToStripCenter;
 		for(unsigned int iStrip=0;iStrip<newVetraCluster->getStripsVetraCluster().size();iStrip++){
		  int stripNr=newVetraCluster->getStripsVetraCluster().at(iStrip);
		  if(fabs((stripNr*0.06)-newVetraClusterPosition) < 0.03){ //we are in the central strip
		    distTrackToStripCenter=x_inter_local-(stripNr*0.06);
		  }
		}
		myBranch.distTrackToStripCenter=distTrackToStripCenter;
		myBranch.eventIdx=eventIndex;
		myBranch.clusterSize=newVetraCluster->getClusterSize();
		for(int i=0;i<4;i++){
		  myBranch.stripsInCluster[i]=0;
		  myBranch.tell1ChInCluster[i]=0;
		  myBranch.adcsInCluster[i]=0;
		}
		for(unsigned int iStrip=0;iStrip<newVetraCluster->getStripsVetraCluster().size();iStrip++){
		  myBranch.stripsInCluster[iStrip]= newVetraCluster->getStripsVetraCluster().at(iStrip);
		  myBranch.tell1ChInCluster[iStrip]= vetraReorder.getTell1chFromStrip(newVetraCluster->getStripsVetraCluster().at(iStrip));
		  myBranch.adcsInCluster[iStrip]= newVetraCluster->getADCVetraCluster().at(iStrip);
		}
		myBranch.clusterResiduals=newVetraClusterPosition-x_inter_local;
		myBranch.etaDistribution=newVetraCluster->getEtaVariable();
		myBranch.weightedEtaDistribution=newVetraCluster->getWeightedEtaVariable();
		myBranch.track_x=x_inter_local;
		myBranch.track_y=y_inter_local;
		myBranch.cluster_x=newVetraClusterPosition;

		tbtree->Fill();

	      } // if spatial matching
	    }// if SumAdcCluster > totalChargeCut
	  } // if adcValue > seed threshold
	} // loop over candidates

	thisTrigger ++;
	nValidTriggers ++;

	if(vetraClustersInFrame->size()!=0)
	  syncDelayTriggerAfterClustering->Fill((*iTrigger)->syncDelay());

      } // end of loop over triggers
    } // end of loop over tracks
  } // end of loop over nVetraElements(==1)

  vetraSeedsInMatchedTriggerInEvent->Fill(eventIndex, seedsInEvent);
  vetraHitsInMatchedTriggerInEvent->Fill(eventIndex, hitsInMatchedTriggerEvent);
  vetraHitsInEvent->Fill(eventIndex, hitsInEvent);

  vetraClustersInEvent->Fill(eventIndex, vetraClustersInFrame->size());
  vetraMatchedClustersInEvent->Fill(eventIndex, alignedTracks->size());
			     
  if(vetraClustersInFrame->size()<1){
    cout<<"       ------------------------------"<<endl;
    cout<<"       No vetraClusters in this frame"<<endl;
    cout<<"       ------------------------------"<<endl;
    return;
  }
  else{
    cout<<"       ------------------------------"<<endl;
    cout<<"       "<<tracks->size()<<" Tracks in this frame"<<endl;
    cout<<"       "<<vetraClustersInFrame->size()<<" vetraClusters in this frame"<<endl;
    cout<<"       "<<alignedTracks->size()<<" aligned vetraClusters in this frame"<<endl;
    cout<<"       ------------------------------"<<endl;
  }
			     
  vetraClustersInFrameH->Fill(vetraClustersInFrame->size());
  fillEtaDistribution(vetraClustersInFrame);

  //   watch.Stop();
  //   cout<< " nTriggers  = "<<nTriggers <<" nVetraHits = "<<nVetraHits<<" cpuTime = "<<watch.CpuTime() <<endl;


  clipboard->put("VetraAlignmentTracks",(TestBeamObjects*)alignedTracks,CREATED);
  if(alignedTracks->size()>0){
    efficiency_trackNr+=temp_efficiency_matchedTrigger;
  }

  //Fill histograms after clustering
  std::vector <int> stripsInCluster;
  std::vector <int> adcsInCluster;
  VetraClusters::iterator iVetra = vetraClustersInFrame->begin();
  for(;iVetra!=vetraClustersInFrame->end();iVetra++){
    if(((int)(*iVetra)->getSyncDelay() > syncDelayLowCutRegion)&&((int)(*iVetra)->getSyncDelay() < syncDelayHighCutRegion)){
      landauAfterClustering->Fill((*iVetra)->getSumAdcCluster());
      if((*iVetra)->getClusterSize()==1)  landauAfterClustering1->Fill((*iVetra)->getSumAdcCluster());
      else if((*iVetra)->getClusterSize()==2)  landauAfterClustering2->Fill((*iVetra)->getSumAdcCluster());
      else if((*iVetra)->getClusterSize()==3)  landauAfterClustering3->Fill((*iVetra)->getSumAdcCluster());
      else if((*iVetra)->getClusterSize()==4)  landauAfterClustering4->Fill((*iVetra)->getSumAdcCluster());
    }
    landauAfterClusteringWithoutCut->Fill((*iVetra)->getSumAdcCluster());
    syncDelayHitsAfterClustering->Fill((*iVetra)->getSyncDelay());

    stripsInCluster = (*iVetra)->getStripsVetraCluster();
    adcsInCluster = (*iVetra)->getADCVetraCluster();
    vetraClusterSizeH->Fill(stripsInCluster.size());
    vetraADCspectrumH->Fill((*iVetra)->getSumAdcCluster());
    vetraPositionClusterH->Fill((*iVetra)->getVetraClusterPosition(), 1.);
    if(stripsInCluster.size()==1)
      vetraPositionCluster1H->Fill((*iVetra)->getVetraClusterPosition(), 1.);
    else if(stripsInCluster.size()==2)
      vetraPositionCluster2H->Fill((*iVetra)->getVetraClusterPosition(), 1.);

    float adcs = (*iVetra)->getSumAdcCluster();
    float syncDelay = (*iVetra)->getSyncDelay();
    accumulatedChargeMod25[(int)syncDelay%25]+=adcs;
    accumulatedEntriesMod25[(int)syncDelay%25]++;
    accumulatedCharge[(int)syncDelay]+=adcs;
    accumulatedEntries[(int)syncDelay]++;
    averageChargeVsBeetleClock->Fill((int)syncDelay, adcs);
  }
}


void NewVetraClusterMaker::removeSpecialClustersForAlignment(std::vector<VetraCluster*>* vetraClustersInFrame){

  VetraClusters::iterator iVetra = vetraClustersInFrame->begin();
  //   for(;iVetra!=vetraClustersInFrame->end();iVetra++)
  //     std::cout<<" iVetra->getVetraClusterPosition() = "<<(*iVetra)->getVetraClusterPosition()<<std::endl;

  iVetra = vetraClustersInFrame->begin();
  for(;iVetra!=vetraClustersInFrame->end();iVetra++){
    //     std::cout<<vetraClustersInFrame->size()<<"  (*iVetra)->getVetraClusterPosition() = "<<(*iVetra)->getVetraClusterPosition()<<std::endl;
    if(((*iVetra)->getVetraClusterPosition() < 0.8)||((*iVetra)->getVetraClusterPosition() > 17.5)){ //in milimeters
      //       std::cout<<"              (*iVetra)->getVetraClusterPosition() = "<<(*iVetra)->getVetraClusterPosition()<<std::endl;
      vetraClustersInFrame->erase(iVetra);
      iVetra--;
    }
  }
}

  
void NewVetraClusterMaker::removeBorderClusters(VetraCluster* currentVetraCluster){
  std::sort(rejectedStrips.begin(), rejectedStrips.end());
  std::vector <int> stripsInCluster;
  stripsInCluster = currentVetraCluster->getStripsVetraCluster();
  for(int iStrip=0;iStrip<(int)stripsInCluster.size();iStrip++){
    int windowCheck;
    if(stripsInCluster.size()==1) windowCheck=2;
    else windowCheck=1;
    int strip = stripsInCluster.at(iStrip);
    for(int jStrip=strip-windowCheck;jStrip<=strip+windowCheck;jStrip++){
      if(rejectedStrips.end() != find(rejectedStrips.begin(), rejectedStrips.end(), jStrip)){
	currentVetraCluster->clear();
	return;
      }
    }
  }
}


void NewVetraClusterMaker::end()
{
  cout<<""<<endl;
  cout<<" --------------------------- NewVetraClusterMaker -----------------------"<<endl;
  //   for(int i=0;i<25;i++){
  for(int i=0;i<200;i++){
    if(globalAccumulatedEntries[i]!=0){
      float ratio = globalAccumulatedCharge[i]/globalAccumulatedEntries[i];
      globalChargeVsBeetleClock->Fill(i, ratio);
    }
    //     if((accumulatedCharge[i]!=0)&&(accumulatedEntries[i]>200)){
    if(accumulatedCharge[i]!=0){
      float ratio=accumulatedCharge[i]/accumulatedEntries[i];
      std::cout<<"accumulatedCharge["<<i<<"] = "<<(int)accumulatedCharge[i]<<"   accumulatedEntries["<<i<<"] = "<<(int)accumulatedEntries[i];
      std::cout<<"  -->  averageCharge = "<<(int)ratio<<std::endl;
      chargeVsBeetleClock->Fill(i, ratio);
    }
  }
  for(int i=0;i<25;i++){
    if(globalAccumulatedEntriesMod25[i]!=0){
      float ratio = globalAccumulatedChargeMod25[i]/globalAccumulatedEntriesMod25[i];
      globalChargeVsBeetleClockMod25->Fill(i, ratio);
    }
  }
  for(int i=0;i<25;i++){
    if(accumulatedEntriesMod25[i]!=0){
      float ratio = accumulatedChargeMod25[i]/accumulatedEntriesMod25[i];
      chargeVsBeetleClockMod25->Fill(i, ratio);
    }
  }
  

  for(int i=0;i<200;i++){
    if(globalEntriesPerSyncDelay[i]!=0){
      float ratio = globalChargePerSyncDelay[i]/globalEntriesPerSyncDelay[i];
      //       std::cout<<"globalChargePerSyncDelay["<<i<<"] = "<<globalChargePerSyncDelay[i];
      //       std::cout<<"   globalEntries["<<i<<"] = "<<globalEntriesPerSyncDelay[i];
      //       std::cout<<"  -->  Charge/Entries = "<<ratio<<std::endl;
      globalChargeVsSyncDelay->Fill(i,ratio);
    }
  }

  etaVsPosition->ProjectionY();

  averageChargeVsBeetleClockProf = averageChargeVsBeetleClock->ProfileX();

  cout<<""<<endl;
  std::cout << "  total frames = " << nTotalFrames<<std::endl;
  std::cout << "  frames with TDC info= " << nFramesWithTDCinfo<<std::endl;
  std::cout << "  TDC elements = " << nTDCElements<<std::endl;
  std::cout << "  matched TDC elements = " << nMatchedTDCElements<<std::endl;
  std::cout << "  total triggers = " << nTotalTriggers<<std::endl;
  std::cout << "  valid triggers = " << nValidTriggers << std::endl;
  cout<<endl<<" Cluster Size Summary:"<<endl;
  cout.precision(1);
  cout<<"    1 Strip Clusters -> "<<(100*vetraClusterSizeH->GetBinContent(2)/vetraClusterSizeH->GetEntries())<<"%"<<endl;
  cout<<"    2 Strip Clusters -> "<<(100*vetraClusterSizeH->GetBinContent(3)/vetraClusterSizeH->GetEntries())<<"%"<<endl;
  cout<<"    3 Strip Clusters -> "<<(100*vetraClusterSizeH->GetBinContent(4)/vetraClusterSizeH->GetEntries())<<"%"<<endl;
  cout<<"    4 Strip Clusters -> "<<(100*vetraClusterSizeH->GetBinContent(5)/vetraClusterSizeH->GetEntries())<<"%"<<endl;

  cout<<endl<<" Efficiency: "<<endl;
  cout.precision(2);
  cout<<"  Window = 1000um -> E="<<100*efficiency_clusterNr_1000um/efficiency_trackNr<<"%"<<endl;
  cout<<"  Window = 500um -> E="<<100*efficiency_clusterNr_500um/efficiency_trackNr<<"%"<<endl;
  cout<<"  Window = 200um -> E="<<100*efficiency_clusterNr_200um/efficiency_trackNr<<"%"<<endl;

  efficiencyPlot->Fill(0.2,efficiency_clusterNr_200um/efficiency_trackNr);
  efficiencyPlot->Fill(0.5,efficiency_clusterNr_500um/efficiency_trackNr);
  efficiencyPlot->Fill(1,efficiency_clusterNr_1000um/efficiency_trackNr);

  if(!display || !parameters->verbose) return; 
  gStyle->SetOptStat(1011);

}



// -----------------  Malcolm's example code   --------------------//

//   for(unsigned int j=0;j<event->nVetraElements();j++){
//     ele=event->getVetraElement(j);
//     std::string chip = ele->detectorId();
//     if(chip!=std::string("PR01")) continue;
//     TDCFrame* frame = (TDCFrame*)ele;
//     std::cout<<" Indexed TDC frame "<<frame->positionInFrame()
// 	     <<" at "<<std::cout.precision(15)
// 	     <<std::cout.width(18)<<frame->timeStamp()
// 	     <<" with "<<frame->nTriggersInFrame()
// 	     <<" triggers in "<<int(frame->timeShutterIsOpen()/1000)
// 	     <<"us ("<<frame->nTriggersUnmatchedInSpill()
// 	     <<" mismatch in frame)"<<std::endl;
//     TDCTriggers* triggers=frame->triggers();
//     for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
//       std::cout<<"        trigger "<<(itr-triggers->begin())
// 	       <<"/"<<frame->nTriggersInFrame()
//       	       <<" which occured "<<(*itr)->timeAfterShutterOpen()
// 	       <<" ns after shutter-open, "<<(*itr)->timeAfterShutterOpen()-frame->timeShutterIsOpen()
// 	       <<" ns before shutter-closed"<<std::endl;
//       for(int ih=0;ih<(*itr)->nVetraHits();ih++){
// 	std::cout<<"               link: "<<(*itr)->vetraLink(ih)
// 		 <<" chan: "<<(*itr)->vetraChannel(ih)<<" = "
// 		 <<(*itr)->vetraADC(ih)<<std::endl;
//       }
//     }
//   }

void NewVetraClusterMaker::defineHistograms(){
  std::string title;
  std::string name;


  title=std::string("Telescope's Hitmap");
  name=std::string("Telescopes_Hitmap");
  telescopesHitmap = new TH2F(name.c_str(),title.c_str(), 256,-7.04,7.04, 256., -7.04, 7.04);
  telescopesHitmap->SetXTitle("mm");
  telescopesHitmap->SetYTitle("mm");

  title=std::string("Telescope's Hitmap Time Matched With Trigger");
  name=std::string("Telescopes_Hitmap_Time_Matched_With_Trigger");
  telescopesHitmapTimeMatchedWithTrigger = new TH2F(name.c_str(),title.c_str(), 256,-7.04,7.04, 256., -7.04, 7.04);
  telescopesHitmapTimeMatchedWithTrigger->SetXTitle("mm");
  telescopesHitmapTimeMatchedWithTrigger->SetYTitle("mm");

  title=std::string("Telescope's Hitmap Time Matched With Vetra Cluster");
  name=std::string("Telescopes_Hitmap_Time_Matched_With_Vetra_Cluster");
  telescopesHitmapTimeMatchedWithVetraCluster = new TH2F(name.c_str(),title.c_str(), 256,-7.04,7.04, 256., -7.04, 7.04);
  telescopesHitmapTimeMatchedWithVetraCluster->SetXTitle("mm");
  telescopesHitmapTimeMatchedWithVetraCluster->SetYTitle("mm");

  title=std::string("Telescope's Hitmap Time And Spatial Matched With Vetra Cluster");
  name=std::string("Telescopes_Hitmap_Time And Spatial_Matched_With_Vetra_Cluster");
  telescopesHitmapTimeAndSpatialMatchedWithVetraCluster = new TH2F(name.c_str(),title.c_str(), 256,-7.04,7.04, 256., -7.04, 7.04);
  telescopesHitmapTimeAndSpatialMatchedWithVetraCluster->SetXTitle("mm");
  telescopesHitmapTimeAndSpatialMatchedWithVetraCluster->SetYTitle("mm");

  correlationDUTTrack = new TH1F("Spatial_correlation_DUT_Track", "correlation_DUT_Track", 500, -25, 25);
  correlationDUTTrack->SetXTitle("mm");

  title=std::string("Track_tdctimestamp_minus_trigger_tdctimestamp");
  name=std::string("Track_tdctimestamp_minus_trigger_tdctimestamp");
  diffTdcTrackTdcVetraH = new TH1F(name.c_str(),title.c_str(),2000,-10,10);
  diffTdcTrackTdcVetraH->SetXTitle("#micros");

  title=std::string("Track_toatimestamp_minus_track_tdctimestamp");
  name=std::string("Track_toatimestamp_minus_track_tdctimestamp");
  diffToaTrackTdcTrackH = new TH1F(name.c_str(),title.c_str(),2000,-1000,1000);
  diffToaTrackTdcTrackH->SetXTitle("ns");

  title=std::string("Valid Tracks In Frame");
  name=std::string("Valid_Tracks_In_Frame");
  validTracksInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  validTracksInFrameH->SetXTitle("# tracks");

  title=std::string("Tracks with TOA info In Frame");
  name=std::string("Tracks_only_TOA_infoIn_Frame");
  onlyToaTracksInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  onlyToaTracksInFrameH->SetXTitle("# tracks");

  title=std::string("Empty Tracks In Frame");
  name=std::string("Empty_Tracks_In_Frame");
  emptyTracksInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  emptyTracksInFrameH->SetXTitle("# tracks");

  title=std::string("Total Tracks In Frame");
  name=std::string("Total_Tracks_In_Frame");
  totalTracksInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  totalTracksInFrameH->SetXTitle("# tracks");

  tracksInEvent = new TH1F("totalTracksInEvent","total Tracks In Event",20000, 0, 20000);
  tracksInEvent->SetXTitle("TB Event");

  validTracksInEvent = new TH1F("validTracksInEvent","Tracks with timestamp In Event",20000, 0, 20000);
  validTracksInEvent->SetXTitle("TB Event");

  title=std::string("Vetra Hits per Trigger");
  name=std::string("Vetra_Hits_per_Trigger");
  vetraHitsPerTriggerH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  vetraHitsPerTriggerH->SetXTitle("hits");

  title=std::string("Triggers In Frame");
  name=std::string("Triggers_In_Frame");
  triggersInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  triggersInFrameH->SetXTitle("# triggers");
 
  title=std::string("Number of TDCElements in Frame");
  name=std::string("nTDCElementsInFrame");
  nTDCElementsH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  nTDCElementsH->SetXTitle("# clusters");

  vetraHitsInEvent= new TH1F("vetraHitsInEvent","vetraHitsInEvent",20000, 0, 20000);
  vetraHitsInEvent->SetXTitle("TB Event");

  vetraHitsInMatchedTriggerInEvent= new TH1F("vetraHitsInTimeMatchedTriggersPerEvent","vetra Hits In TimeMatched Triggers Per Event",20000, 0, 20000);
  vetraHitsInMatchedTriggerInEvent->SetXTitle("TB Event");

  vetraSeedsInMatchedTriggerInEvent= new TH1F("vetraSeedsInTimeMatchedTriggersPerEvent","vetra Seeds In TimeMatched Triggers Per Event",20000, 0, 20000);
  vetraSeedsInMatchedTriggerInEvent->SetXTitle("TB Event");

  name=std::string("Histogram_VetraClusters_In_Frame");
  title=std::string("Histogram VetraClusters In Frame");
  vetraClustersInFrameH = new TH1F(name.c_str(),title.c_str(),100,0,100);
  vetraClustersInFrameH->SetXTitle("# vetraClusters");

  vetraClustersInEvent= new TH1F("distributionVetraClustersInEvent","distributionVetraClustersInEvent",20000, 0, 20000);
  vetraClustersInEvent->SetXTitle("TB Event");

  vetraMatchedClustersInEvent = new TH1F("vetraMatchedClustersInEvent","vetraMatchedClustersInEvent",20000, 0, 20000);
  vetraMatchedClustersInEvent->SetXTitle("TB Event");

  title=std::string("tell1Ch's noise distribution");
  name=std::string("tell1Ch_noise_distribution");
  tell1ChNoiseDistributionH = new TH1F(name.c_str(),title.c_str(),2048,0,2048);
  tell1ChNoiseDistributionH->SetXTitle("Strip number");

  title=std::string("sensor's noise distribution");
  name=std::string("sensor_noise_distribution");
  sensorNoiseDistributionH = new TH1F(name.c_str(),title.c_str(),2048,0,2048);
  sensorNoiseDistributionH->SetXTitle("Strip number");

  title=std::string("sensor's noise residuals");
  name=std::string("sensor_noise_residuals");
  sensorNoiseResiduals = new TH1F(name.c_str(),title.c_str(),100,0,10);
  sensorNoiseResiduals->SetXTitle("ADC");

  title=std::string("TELL1 channel occupancy");
  name=std::string("TELL1_channel_occupancy");
  vetraChannelH = new TH1F(name.c_str(),title.c_str(),2048,0,2048);
  vetraChannelH->SetXTitle("TELL1 channel number");

  title=std::string("sensor strip occupancy");
  name=std::string("sensor_strip_occupancy");
  vetraStripH = new TH1F(name.c_str(),title.c_str(),2048,0,2048);
  vetraStripH->SetXTitle("Strip number");

  title=std::string("sensor's seed distribution");
  name=std::string("sensor_seed_distribution");
  vetraClusterSeedH = new TH1F(name.c_str(),title.c_str(),2048,0,2048);
  vetraClusterSeedH->SetXTitle("Strip number");

  title=std::string("sensor's ADC Spectrum");
  name=std::string("sensor_ADC_Spectrum");
  vetraADCspectrumH = new TH1F(name.c_str(),title.c_str(),150,0,150);
  vetraADCspectrumH->SetXTitle("ADC");

  title=std::string("Initial ADC Spectrum Vs Strip number");
  name=std::string("initial_ADC_spectrum_Vs_Strip_number");
  initialAdcVsStrip = new TH2F(name.c_str(),title.c_str(),1004,0,1003,100,0,99);
  initialAdcVsStrip->SetXTitle("strip number");

  title=std::string("Landau before clustering with TDC cut");
  name=std::string("Landau_before_clustering_with_TDC_cut");
  landauBeforeClustering = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauBeforeClustering->SetXTitle("#ADC");

  title=std::string("Landau after clustering with TDC cut");
  name=std::string("Landau_after_clustering_with_TDC_cut");
  landauAfterClustering = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClustering->SetXTitle("#ADC");

  title=std::string("sensor Cluster Position");
  name=std::string("sensor_Cluster_Position");
  if(parameters->dut=="PR01")
    vetraPositionClusterH = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionClusterH = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionClusterH = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionClusterH->SetXTitle("mm");

  title=std::string("sensor ClusterSize1 Position");
  name=std::string("sensor_ClusterSize1_Position");
  if(parameters->dut=="PR01")
    vetraPositionCluster1H = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionCluster1H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionCluster1H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionCluster1H->SetXTitle("mm");

  title=std::string("sensor ClusterSize2 Position");
  name=std::string("sensor_ClusterSize2_Position");
  if(parameters->dut=="PR01")
    vetraPositionCluster2H = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionCluster2H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionCluster2H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionCluster1H->SetXTitle("mm");

  title=std::string("sensor Matched Cluster Position");
  name=std::string("sensor_Matched_Cluster_Position");
  if(parameters->dut=="PR01")
    vetraPositionMatchedClusterH = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionMatchedClusterH = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionMatchedClusterH = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionMatchedClusterH->SetXTitle("mm");

  title=std::string("sensor Matched ClusterSize1 Position");
  name=std::string("sensor_Matched_ClusterSize1_Position");
  if(parameters->dut=="PR01")
    vetraPositionMatchedCluster1H = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionMatchedCluster1H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionMatchedCluster1H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionMatchedCluster1H->SetXTitle("mm");

  title=std::string("sensor Matched ClusterSize2 Position");
  name=std::string("sensor_Matched_ClusterSize2_Position");
  if(parameters->dut=="PR01")
    vetraPositionMatchedCluster2H = new TH1F(name.c_str(),title.c_str(),25*50,0,50);
  else if(parameters->dut=="D0")
    vetraPositionMatchedCluster2H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/60,0,50);
  else if(parameters->dut=="BCB")
    vetraPositionMatchedCluster2H = new TH1F(name.c_str(),title.c_str(),(int)50*1000/80,0,50);
  vetraPositionMatchedCluster1H->SetXTitle("mm");

  title=std::string("sensor's cluster size");  
  name=std::string("sensor_cluster_size");
  vetraClusterSizeH = new TH1F(name.c_str(),title.c_str(),10,0,10);
  vetraClusterSizeH->SetXTitle("strips into each cluster");

  title=std::string("Residuals Vetra Cluster Position minus Track's X Coordinate");
  name=std::string("Residuals_VetraClusterPosition_TrackX");
  residualsVetraPositionTrackX = new TH1F(name.c_str(),title.c_str(),100,-0.05,0.05);
  residualsVetraPositionTrackX->SetXTitle("milimeters");

  title=std::string("Residuals for cluster size 1");
  name=std::string("Residuals_ClusterSize1");
  residualsClusterSize1 = new TH1F(name.c_str(),title.c_str(),100,-0.05,0.05);
  residualsClusterSize1->SetXTitle("milimeters");

  title=std::string("Residuals for cluster size 2");
  name=std::string("Residuals_ClusterSize2");
  residualsClusterSize2 = new TH1F(name.c_str(),title.c_str(),100,-0.05,0.05);
  residualsClusterSize2->SetXTitle("milimeters");


  title=std::string("Total charge in 1stripClusters after clustering with TDC cut");
  name=std::string("totalCharte_1stripClusters_after_clustering_with_TDC_cut");
  landauAfterClustering1 = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClustering1->SetXTitle("#ADC");

  title=std::string("Total charge in 2stripClusters after clustering with TDC cut");
  name=std::string("totalCharte_2stripClusters_after_clustering_with_TDC_cut");
  landauAfterClustering2 = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClustering2->SetXTitle("#ADC");

  title=std::string("Total charge in 3stripClusters after clustering with TDC cut");
  name=std::string("totalCharte_3stripClusters_after_clustering_with_TDC_cut");
  landauAfterClustering3 = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClustering3->SetXTitle("#ADC");

  title=std::string("Total charge in 4stripClusters after clustering with TDC cut");
  name=std::string("totalCharte_4stripClusters_after_clustering_with_TDC_cut");
  landauAfterClustering4 = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClustering4->SetXTitle("#ADC");



  title=std::string("Landau before clustering whithout TDC cut");
  name=std::string("Landau_before_clustering_without_TDC_cut");
  landauBeforeClusteringWithoutCut = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauBeforeClusteringWithoutCut->SetXTitle("#ADC");

  title=std::string("Landau after clustering whithout TDC cut");
  name=std::string("Landau_after_clustering_without_TDC_cut");
  landauAfterClusteringWithoutCut = new TH1F(name.c_str(),title.c_str(),300,0,300);
  landauAfterClusteringWithoutCut->SetXTitle("#ADC");

  title=std::string("syncDelay Trigger Before Clustering");
  name=std::string("syncDelay_trigger_before_clustering");
  syncDelayTriggerGlobal = new TH1F(name.c_str(),title.c_str(),200,0,200);
  syncDelayTriggerGlobal->SetXTitle("ns");

  title=std::string("syncDelay Trigger After Clustering");
  name=std::string("syncDelay_trigger_after_clustering");
  syncDelayTriggerAfterClustering = new TH1F(name.c_str(),title.c_str(),200,0,200);
  syncDelayTriggerAfterClustering->SetXTitle("ns");

  title=std::string("syncDelay Hits Before Clustering");
  name=std::string("syncDelay_hits_before_clustering");
  syncDelayBeforeClustering = new TH1F(name.c_str(),title.c_str(),200,0,200);
  syncDelayBeforeClustering->SetXTitle("ns");

  title=std::string("syncDelay Hits After Clustering");
  name=std::string("syncDelay_hits_after_clustering");
  syncDelayHitsAfterClustering = new TH1F(name.c_str(),title.c_str(),200,0,200);
  syncDelayHitsAfterClustering->SetXTitle("ns");

  title=std::string("Global accumulated charge versus Beetle clock");
  name=std::string("global_accumulated_charge_versus_Beetle_clock");
  globalChargeVsBeetleClock = new TH1F(name.c_str(),title.c_str(),200,0,200);
  globalChargeVsBeetleClock->SetXTitle("ns");

  title=std::string("Accumulated charge versus Beetle clock");
  name=std::string("accumulated_charge_versus_Beetle_clock");
  chargeVsBeetleClock = new TH1F(name.c_str(),title.c_str(),200,0,200);
  chargeVsBeetleClock->SetXTitle("ns");

  title=std::string("Global accumulated charge versus Beetle clock Module 25");
  name=std::string("global_accumulated_charge_versus_Beetle_clock_Mod25");
  globalChargeVsBeetleClockMod25 = new TH1F(name.c_str(),title.c_str(),25,0,25);
  globalChargeVsBeetleClockMod25->SetXTitle("ns");

  title=std::string("Accumulated charge versus Beetle clock Module 25");
  name=std::string("accumulated_charge_versus_Beetle_clock_Mod25");
  chargeVsBeetleClockMod25 = new TH1F(name.c_str(),title.c_str(),25,0,25);
  chargeVsBeetleClockMod25->SetXTitle("ns");

  title=std::string("Global accumulated charge versus syncDelay");
  name=std::string("global_accumulated_charge_versus_syncDelay");
  globalChargeVsSyncDelay = new TH1F(name.c_str(),title.c_str(),200,0,200);
  globalChargeVsSyncDelay->SetXTitle("ns");

  averageChargeVsBeetleClock = new TH2F("Average_Charge_versus_Beetle_Clock", "average_charge_vs_beetle_clock", 200, 0, 200, 100, 0, 100);
  averageChargeVsBeetleClock->SetXTitle("ns");

  title=string("ETA Correction Distribution");
  name=string("ETA_Correction_Distribution");
  etaDistribution = new TH1F(name.c_str(), title.c_str(), 20, 0, 1);
  etaDistribution->SetXTitle("#eta");

  etaVsPosition = new TH2F("eta_vs_position", "#eta versus strip", 25*50, 0, 50, 20, 0, 1);
  etaVsPosition->SetXTitle("mm");
  etaVsPosition->SetYTitle("#eta");

  title=string("ADC of strips adjacents to a track");
  name=string("adjacent_strips");
  adcAdjacentStrips = new TH1F(name.c_str(), title.c_str(), 120, -100, 20);
  adcAdjacentStrips->SetXTitle("ADC");
 
  checkImpulseADC_central = new TH1F("impulseAdc_Central","Adc in strip central",100,0,100);
  checkImpulseADC_central->SetXTitle("ADC");
  checkImpulseADC_p1 = new TH1F("impulseAdc_p1","Adc in strip+1",100,0,100);
  checkImpulseADC_p1->SetXTitle("ADC");
  checkImpulseADC_p2 = new TH1F("impulseAdc_p2","Adc in strip+2",100,0,100);
  checkImpulseADC_p2->SetXTitle("ADC");
  checkImpulseADC_m1 = new TH1F("impulseAdc_m1","Adc in strip-1",100,0,100);
  checkImpulseADC_m1->SetXTitle("ADC");
  checkImpulseADC_m2 = new TH1F("impulseAdc_m2","Adc in strip-2",100,0,100);
  checkImpulseADC_m2->SetXTitle("ADC");


  checkImpulseCenter = new TH1F("impulseCenter","impulseCenter",5,-2.5,2.5);
  checkImpulseCenter->SetXTitle("strip");

  ratioAdcNeighbor_m2 = new TH1F("ratioAdcNeighbor_m2","ADC(Strip-2) / ADC(Strip0)",200.,0.,2.);
  ratioAdcNeighbor_m2->SetXTitle("ratio");
  ratioAdcNeighbor_m1 = new TH1F("ratioAdcNeighbor_m1","ADC(Strip-1) / ADC(Strip0)",200.,0.,2.);
  ratioAdcNeighbor_m1->SetXTitle("ratio");
  ratioAdcNeighbor_p1 = new TH1F("ratioAdcNeighbor_p1","ADC(Strip+1) / ADC(Strip0)",200.,0.,2.);
  ratioAdcNeighbor_p1->SetXTitle("ratio");
  ratioAdcNeighbor_p2 = new TH1F("ratioAdcNeighbor_p2","ADC(Strip+2) / ADC(Strip0)",200.,0.,2.);
  ratioAdcNeighbor_p2->SetXTitle("ratio");


  ratioAdcNeighbor_m2_15 = new TH1F("ratioAdcNeighbor_m2_15","ADC(Strip-2) / ADC(Strip0) with shift in 15 um",200.,0.,2.);
  ratioAdcNeighbor_m2_15->SetXTitle("ratio");
  ratioAdcNeighbor_m1_15 = new TH1F("ratioAdcNeighbor_m1_15","ADC(Strip-1) / ADC(Strip0) with shift in 15 um",200.,0.,2.);
  ratioAdcNeighbor_m1_15->SetXTitle("ratio");
  ratioAdcNeighbor_p1_15 = new TH1F("ratioAdcNeighbor_p1_15","ADC(Strip+1) / ADC(Strip0) with shift in 15 um",200.,0.,2.);
  ratioAdcNeighbor_p1_15->SetXTitle("ratio");
  ratioAdcNeighbor_p2_15 = new TH1F("ratioAdcNeighbor_p2_15","ADC(Strip+2) / ADC(Strip0) with shift in 15 um",200.,0.,2.);
  ratioAdcNeighbor_p2_15->SetXTitle("ratio");


  ratioAdcNeighbor_m2_30 = new TH1F("ratioAdcNeighbor_m2_30","ADC(Strip-2) / ADC(Strip0) with shift in 30 um",200.,0.,2.);
  ratioAdcNeighbor_m2_30->SetXTitle("ratio");
  ratioAdcNeighbor_m1_30 = new TH1F("ratioAdcNeighbor_m1_30","ADC(Strip-1) / ADC(Strip0) with shift in 30 um",200.,0.,2.);
  ratioAdcNeighbor_m1_30->SetXTitle("ratio");
  ratioAdcNeighbor_p1_30 = new TH1F("ratioAdcNeighbor_p1_30","ADC(Strip+1) / ADC(Strip0) with shift in 30 um",200.,0.,2.);
  ratioAdcNeighbor_p1_30->SetXTitle("ratio");
  ratioAdcNeighbor_p2_30 = new TH1F("ratioAdcNeighbor_p2_30","ADC(Strip+2) / ADC(Strip0) with shift in 30 um",200.,0.,2.);
  ratioAdcNeighbor_p2_30->SetXTitle("ratio");

  etaVsTrackx = new TH2F("etaVsTrackx","etaVsTrackx",100.,0,1, 100,0,1);
  etaVsTrackx->SetXTitle("(Track_x - LeftStrip_x)/pitch");
  etaVsTrackx->SetYTitle("ETA");

  cogVsTrackx = new TH2F("cogVsTrackx","cogVsTrackx",100.,0,1,100,0,1);
  cogVsTrackx->SetXTitle("(Track_x - LeftStrip_x)/pitch");
  cogVsTrackx->SetYTitle("(COG - Leftstrip_X)/pitch");

  totalChargeVsTrackx = new TH2F("totalChargeVsTracx","totalChargeVsTracx",200,-0.5,1.5,100,0,100);
  totalChargeVsTrackx->SetXTitle("(Track_x - LeftStrip_x)/pitch");
  totalChargeVsTrackx->SetYTitle("Total Charge");

  chargeLeftVsTrackx = new TH2F("chargeLeftVsTracx","chargeLeftVsTracx",200,-0.5,1.5,100,0,100);
  chargeLeftVsTrackx->SetXTitle("(Track_x - LeftStrip_x)/pitch");
  chargeLeftVsTrackx->SetYTitle("Charge at Left strip");

  chargeRightVsTrackx = new TH2F("chargeRightVsTracx","chargeRightVsTracx",200,-0.5,1.5,100,0,100);
  chargeRightVsTrackx->SetXTitle("(Track_x - LeftStrip_x)/pitch");
  chargeRightVsTrackx->SetYTitle("Charge at Right strip");

  chargeLeftVsChargeRight_0_05 = new TH2F("chargeLeftVsChargeRight_0_05","chargeLeftVsChargeRight_0_05",100,0,100,100,0,100);
  chargeLeftVsChargeRight_0_05->SetXTitle("Charge at Right");
  chargeLeftVsChargeRight_0_05->SetYTitle("Charge at Left");

  chargeLeftVsChargeRight_05_1 = new TH2F("chargeLeftVsChargeRight_05_1","chargeLeftVsChargeRight_05_1",100,0,100,100,0,100);
  chargeLeftVsChargeRight_05_1->SetXTitle("Charge at Right");
  chargeLeftVsChargeRight_05_1->SetYTitle("Charge at Left");

  etaInverse = new TH1F("etaInverse","etaInverse",20,0,1);
  etaInverse->SetXTitle("eta inverse");

  efficiencyPlot = new TH1F("efficiency","efficiency",12,0,1.2);
  efficiencyPlot->SetXTitle("Window Size");

  title=std::string("test");
  name=std::string("test");
  test = new TH1F(name.c_str(),title.c_str(),1000,0,1);
  test->SetXTitle("");

  tbtree = new TTree("tbtree", "vetra_nTuple");
  tbtree->Branch("eventIdx", &myBranch.eventIdx, "eventIdx/I");
  tbtree->Branch("clusterSize", &myBranch.clusterSize, "clusterSize/I");
  tbtree->Branch("stripsInCluster", myBranch.stripsInCluster, "stripsInCluster[4]/I");
  tbtree->Branch("tell1ChInCluster", myBranch.tell1ChInCluster, "tell1ChInCluster[4]/I");
  tbtree->Branch("adcsInCluster", myBranch.adcsInCluster, "adcsInCluster[4]/I");
  tbtree->Branch("clusterResiduals", &myBranch.clusterResiduals, "clusterResiduals/F");
  tbtree->Branch("distTrackToStripCenter", &myBranch.distTrackToStripCenter, "distTrackToStripsCenter/F");
  tbtree->Branch("etaDistribution", &myBranch.etaDistribution, "etaDistribution/F");
  tbtree->Branch("weightedEtaDistribution", &myBranch.weightedEtaDistribution, "weightedEtaDistribution/F");
  tbtree->Branch("track_x", &myBranch.track_x, "track_x/F");
  tbtree->Branch("track_y", &myBranch.track_y, "track_y/F");
  tbtree->Branch("cluster_x", &myBranch.cluster_x, "cluster_x/F");
}

//------------ Eta Correction Algorithm ------------------//
void NewVetraClusterMaker::fillEtaDistribution(std::vector<VetraCluster*>* vetraClustersInFrame){
  VetraClusters::iterator iVetra = vetraClustersInFrame->begin();
  for(;iVetra!=vetraClustersInFrame->end();iVetra++){
  }
}
/*
  VetraCluster* vetraCluster;
  VetraMapping vetraReorder;

  for(int i=0;i<(int)clusterList.size();i++){
  vetraCluster = clusterList.at(i);
  float eta = vetraCluster->getEtaVariable();
  //if(eta == 0) continue; //1 strip clusters are not relevant here
    
  int syncDelay = ((int)vetraCluster->getSyncDelay())%25;

  if(vetraAccumulatedChargeH->GetBinContent(syncDelay) > peakThreshold){

  std::vector <int> stripsInCluster = vetraCluster->getStripsVetraCluster();
  std::vector <int> adcsInCluster = vetraCluster->getADCVetraCluster();

  if(stripsInCluster.size() == 2){
  if(vetraCluster->is40microns())
  etaVariable40H->Fill(eta);
  else
  etaVariable60H->Fill(eta);
  }
  else if(stripsInCluster.size() == 3){
  if(vetraCluster->is40microns())
  eta3Variable40H->Fill(eta);
  else
  eta3Variable60H->Fill(eta);
  }
  else if(stripsInCluster.size() == 4){
  if(vetraCluster->is40microns())
  eta4Variable40H->Fill(eta);
  else
  eta4Variable60H->Fill(eta);
  }
  }
  }

  if (clusterList.size()>0){

  TF1* g1=new TF1("m1", "gaus", 0., 0.5);
  TF1* g2=new TF1("m2", "gaus", 0.5, 1.0);
  TF1* etaFunction=new TF1("etaTotal", "gaus(0)+gaus(3)", 0., 1.0);
  double par[9];
  etaVariable40H->Fit(g1, "QR");
  etaVariable40H->Fit(g2, "QR+");
  g1->GetParameters(&par[0]);
  g2->GetParameters(&par[3]);
  etaFunction->SetParameters(par);
  etaVariable40H->Fit(etaFunction, "QR+");

  float trackMinusPosition=0;
  float trackMinusEta=0;
  for(int i=0;i<(int)clusterList.size();i++){
  vetraCluster = clusterList.at(i);
  double track = vetraCluster->getTrackPosition();
  if(track == -1) continue; // this cluster has no associated track

  int syncDelay = ((int)vetraCluster->getSyncDelay())%25;
  if(vetraAccumulatedChargeH->GetBinContent(syncDelay) > peakThreshold) {

  float eta = vetraCluster->getEtaVariable();
  //if(eta == 0) continue; //1 strip clusters are not relevant here
    
  std::vector <int> stripsInCluster = vetraCluster->getStripsVetraCluster();
  std::vector <int> adcsInCluster = vetraCluster->getADCVetraCluster();
  float Position = vetraCluster->getVetraClusterPosition();
  float leftPosition = vetraReorder.StripToPosition[stripsInCluster.at(0)];

  residualTrackPositionH->Fill(track-Position);


  if(stripsInCluster.size() < 3) //clusters of 1 and 2 strips (98.5%)
  if(vetraCluster->is40microns())
  eta1_2VariableVsPitch40H->Fill((track-leftPosition)/0.04, eta);

  if(stripsInCluster.size() == 1) //71.7% of the clusters
  landau1stripsClusterH->Fill(vetraCluster->getSumAdcCluster());

  if(stripsInCluster.size() == 2){ //26.8% of the clusters
  float etaCorrection = etaFunction->Integral(0.,eta)/etaFunction->Integral(0., 1.);//Gaus Fit
  float etaCorrection1 = etaVariable40H->Integral(0.,eta*etaVariable40H->GetNbinsX())/etaVariable40H->Integral();//Integral

  int etaZero=0;
  for(int iBin=1;iBin<eta*etaVariable40H->GetNbinsX();iBin++){
  etaZero+=etaVariable40H->GetBinContent(iBin);
  }
  float etaCorrection2=etaZero/etaVariable40H->GetEntries();//Summatory
  trackMinusPosition += track-Position;
  trackMinusEta += track-(leftPosition+0.04*etaCorrection);
  EtaVariableVsEtaFunctionH->Fill(eta, etaCorrection);
  trackVsEtaFunctionH->Fill(track-leftPosition, etaCorrection*0.04);
  residualAfterEtaIntegralH->Fill(track-(leftPosition+0.04*etaCorrection));
  residualAfterEtaGausFitH->Fill(track-(leftPosition+0.04*etaCorrection1));
  residualAfterEtaSummatoryH->Fill(track-(leftPosition+0.04*etaCorrection2));
  residualTrackPosition2StripsH->Fill(track-Position);
  landau2stripsClusterH->Fill(vetraCluster->getSumAdcCluster());

  if(vetraCluster->is40microns()){
  etaVariableVsPitch40H->Fill((track-leftPosition)/0.04, eta);
  etaFunctionVsPitch40H->Fill((track-leftPosition)/0.04, etaCorrection);
  }
  else
  etaVariableVsPitch60H->Fill((eta-leftPosition)/0.06, eta);
  }
  else if(stripsInCluster.size() == 3){
  if(vetraCluster->is40microns())
  eta3VariableVsPitch40H->Fill((Position-leftPosition)/0.04, eta);
  else
  eta3VariableVsPitch60H->Fill((Position-leftPosition)/0.06, eta);
  }
  else if(stripsInCluster.size() == 4){
  if(vetraCluster->is40microns())
  eta4VariableVsPitch40H->Fill((Position-leftPosition)/0.04, eta);
  else
  eta4VariableVsPitch60H->Fill((Position-leftPosition)/0.06, eta);
  }
  }
  }
  TF1* fit1 = LanGau::FitLanGau(landau1stripsClusterH);
  landau1stripsClusterH->Fit(fit1, "Q");
  TF1* fit2 = LanGau::FitLanGau(landau2stripsClusterH);
  landau2stripsClusterH->Fit(fit2, "Q");

  std::cout<<""<<endl;
  std::cout <<"Accumulated Track-Position = " <<trackMinusPosition<<endl;
  std::cout <<"Accumulated Track-ETA = " <<trackMinusEta<<endl;

  printf("\n\n");
  std::cout<<"Residual Track-EtaCorrection: Strategy = Summatory"<<endl;
  TF1* g6=new TF1("m6", "gaus", -0.1, 0.1);
  residualAfterEtaSummatoryH->Fit(g6, "R+");
  std::cout<<"Residual Track-EtaCorrection: Strategy = Integral"<<endl;
  TF1* g3=new TF1("m3", "gaus", -0.1, 0.1);
  residualAfterEtaIntegralH->Fit(g3, "R+");
  std::cout<<"Residual Track-EtaCorrection: Strategy = gaus fit"<<endl;
  TF1* g4=new TF1("m4", "gaus", -0.1, 0.1);
  residualAfterEtaGausFitH->Fit(g4, "R+");
  std::cout <<"Residual Track-Position:"<<endl;
  TF1* g5=new TF1("m5", "gaus", -0.1, 0.1);
  residualTrackPosition2StripsH->Fit(g5, "R+");
  printf("\n\n");

  //------------ End of Eta Correction Algorithm ------------------//
  }
  clusterList.clear();
  }
*/
