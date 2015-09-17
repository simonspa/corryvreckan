#ifndef TESTBEAMUSERINFO_H 
#define TESTBEAMUSERINFO_H 1

#include <vector>
#include <string>
#include <map>

#include "TH1.h"

#include "TestBeamEvent.h"

class TestBeamDataSummary : public TObject {
public:
  TestBeamDataSummary();
  virtual ~TestBeamDataSummary(){}
  int deviceUnderTest(std::string s);
  std::string deviceUnderTest(){return m_dut;}
  void dataSources(bool t,bool m){m_usingTelescope=t;m_usingMedipix=m;}
  bool usingSiBT(){return m_usingTelescope;}
  bool usingMedipixOrTimepix(){return m_usingMedipix;}
  void origins(std::string t,std::string m,std::string e){m_telescopeFile=t;m_medipixFile=m;m_eventFile=e;}
  std::string siBTFile(){return m_telescopeFile;}
  std::string medipixFile(){return m_medipixFile;}
  std::string eventFile(){return m_eventFile;}
  void startTime(std::string s,double w){m_start=s;m_evtWin=w;}
  std::string startTime(){return m_start;}
  void registerDetector(std::string,int,int,float,int);
  int modeForDetector(std::string detector){return detectorMode[detector];}
  int clockForDetector(std::string detector){return detectorClock[detector];}
  int nFilesForDetector(std::string detector){return detectorCount[detector];}
  float acqTimeForDetector(std::string detector){return detectorAcqTime[detector];}
  int thresholdForDetector(std::string detector){return detectorThreshold[detector];}
  std::string source(int i){int m=detectorMode[detectorList[i]];return std::string(m==3?"timepix":m==0?"SiBT":"medipix");}
  std::string detectorId(int i){return detectorList[i];}
  int nDetectors(){return detectorList.size();}
  void eventSummary(TestBeamEvent *event, double c);
  TH1D *jitterPlot_L(std::string d){return timeJitter_L[d];}
  TH1D *jitterPlot_S(std::string d){return timeJitter_S[d];}
  TH1D *jitterPlot_Rel(std::string d){return timeJitter_R[d];}
  TH1F *nHitsPerEvent(std::string d){return nHitsPerEvt[d];}
  TH1F *nFilesPerEvent(std::string d){return nFilesPerEvt[d];}
  TH1F *detectorElementsPresent(){return elementsPresent;}
  TH1F *detectorElementsWithHit(){return elementsWithHit;}
  TH1F *framesPerSpill(std::string d){return nFramesPerSpill[d];}
  void firstTime(double t){m_firstTime=t;}
  double firstTime(){return m_firstTime;}
  void lastTime(double t){m_lastTime=t;}
  double lastTime(){return m_lastTime;}
  TH1F *timeSinceLast(std::string d){return timeSince[d];}
  void CopyTDCHistos(TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*,TH1F*);
  void tdcTimepixDiff(std::string d,int i){hUnmatchedToTimepix[d]->Fill(i);}
  TH1F *tdcTimepixDiff(std::string d){return hUnmatchedToTimepix[d];}
  double timing(std::string,double);
  void timingEnd();
  ClassDef(TestBeamDataSummary,1)
private:
  std::string m_dut;
  std::string m_start;
  double m_evtWin;
  bool m_usingTelescope;
  bool m_usingMedipix;
  std::string m_telescopeFile;
  std::string m_medipixFile;
  std::string m_eventFile;
  std::map<std::string,int> detectorCount;
  std::map<std::string,int> detectorMode;
  std::map<std::string,int> detectorClock;
  std::map<std::string,float> detectorAcqTime;
  std::map<std::string,int> detectorThreshold;
  std::vector< std::string > detectorList;
  std::map<std::string,TH1D*> timeJitter_L;
  std::map<std::string,TH1D*> timeJitter_S;
  std::map<std::string,TH1D*> timeJitter_R;
  std::map<std::string,TH1F*> nHitsPerEvt;
  std::map<std::string,int>   fileCountPerEvt;
  std::map<std::string,TH1F*> nFilesPerEvt;
  std::map<std::string,TH1F*> nFramesPerSpill;
  std::map<std::string,TH1F*> timeSince;
  std::map<std::string,TH1F*> hUnmatchedToTimepix;
  std::map<std::string,int>   frameCounter;
  std::map<std::string,double>lastFrameTime;
  TH1F *elementsPresent;
  TH1F *elementsWithHit;
  double storedCurrentTime;
  std::string firstNonSiBTElement;
  double m_firstTime;
  double m_lastTime;
public:
  TH1F* hNumberSync;
  TH1F* hNumberUnsync;
  TH1F* hNumberPairs;
  TH1F* hTDCShutterTime;
  TH1F* hTDCSyncTriggerTime;
  TH1F* hTDCUnsyncTriggerTime;
  TH1F* hSynchronisationDelay;
  TH1F* hExtraUnsyncTriggers;
  TH1F* hExtraSyncTriggers;
  TH1F* hMedipixTime;
  TH1F* hVetraTime;
  TH1F* hUnmatched;
  TH1F* hTDCTime;
  TH1F* hFEI4TimeDiff;
  TH1F* hFEI4MatchTime;
  TH1F* hFEI4FractionMatched;
};

inline TestBeamDataSummary::TestBeamDataSummary()
{
  storedCurrentTime=0.0;
  elementsPresent=new TH1F("elementsPresent","No. detector elements present in event",13,-0.5,12.5);
  elementsWithHit=new TH1F("elementsWithHit","No. detector elements with at least 1 hit",13,-0.5,12.5);
}

inline int TestBeamDataSummary::deviceUnderTest(std::string d)
{
  m_dut=d;
  std::cout<<std::endl;
  for(int i=0;i<nDetectors();i++){
    std::cout << detectorId(i) << " with " << detectorCount[detectorId(i)] <<" frames "<< (detectorId(i)==m_dut?"(DUT)":"") << std::endl;
  }
  return not(detectorCount.find(d)==detectorCount.end());
}
inline void TestBeamDataSummary::registerDetector(std::string d,int m, int cl,float a,int t)
{
  if(detectorCount.find(d)==detectorCount.end()){
    detectorCount.insert( std::make_pair(d,1) );
    detectorMode.insert( std::make_pair(d,m) );  //0=SiBT. Otherwise, 1-3: 1-2.1, 2-MXR, 3-TPX
    detectorClock.insert( std::make_pair(d,cl) );//0-3: 10MHz, 20MHz, 40MHz, 80MHz
    detectorAcqTime.insert( std::make_pair(d,a) );//in seconds
    detectorThreshold.insert( std::make_pair(d,t) );
    std::string name=std::string("FramePerSpill_")+d;
    TH1F *g = new TH1F(name.c_str(),"Frames per spill",1001,-0.5,1000.5);
    nFramesPerSpill.insert( std::make_pair(d,g) );
    name=std::string("timeSince_")+d;
    TH1F *h = new TH1F(name.c_str(),"Time since previous frame",201,-0.5,200.5);
    timeSince.insert( std::make_pair(d,h) );
    name=std::string("tdcMismatch_")+d;
    TH1F *f = new TH1F(name.c_str(),"N_fr(timepix)-N_fr(tdc)/spill",19,-9.5,9.5);
    hUnmatchedToTimepix.insert( std::make_pair(d,f) );
    frameCounter.insert( std::make_pair(d,0) );
    lastFrameTime.insert( std::make_pair(d,0) );
    detectorList.push_back(d);
  }else{
    detectorCount[d]++;
  }
}

inline double TestBeamDataSummary::timing(std::string d, double tm)
{
  double sinceLast=tm-lastFrameTime[d];
  timeSince[d]->Fill(sinceLast*1000.);
  if(lastFrameTime[d]!=0 and sinceLast>10){//10sec
    //if(std::string("D04-W001")==d){ std::cout<<sinceLast<<" "<<frameCounter[d]<<" fill "<<std::endl;}
    nFramesPerSpill[d]->Fill(frameCounter[d]);
    frameCounter[d]=0;
  }
  lastFrameTime[d]=tm;
  frameCounter[d]++;
  //if(std::string("D04-W001")==d){ std::cout<<sinceLast<<" "<<frameCounter[d]<<(sinceLast<0.00005?"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!":"")<<std::endl;}
  return sinceLast;
}

inline void TestBeamDataSummary::timingEnd()
{
  for(int i=0;i<nDetectors();i++){
    //if(std::string("D04-W001")==detectorId(i)){ std::cout<<frameCounter[detectorId(i)]<<" fill "<<std::endl; }
    nFramesPerSpill[detectorId(i)]->Fill(frameCounter[detectorId(i)]);
    frameCounter[detectorId(i)]=0;
  }
}

inline void TestBeamDataSummary::eventSummary(TestBeamEvent *event, double c)
{
  double referenceTime=-1;
  int nDetectorsSeeingSomething=0;
  for(unsigned int j=0;j<event->nElements();j++){
    TestBeamEventElement *element=event->getElement(j);
    double t=element->timeStamp();
    std::string d=element->detectorId();
    unsigned int n=element->data()->size();
    if(n) nDetectorsSeeingSomething++;
    if(m_dut==d) referenceTime=t;

    if(timeJitter_L.find(d)==timeJitter_L.end()){
      std::string title=std::string("Time Jitter (sequential)");

      std::string name=std::string("timeJitter_L_")+d;
      TH1D *h = new TH1D(name.c_str(),title.c_str(),105,-0.005,0.1 );
      h->GetXaxis()->SetTitle("seconds");
      timeJitter_L.insert( std::make_pair(d,h));

      name=std::string("timeJitter_S_")+d;
      TH1D *g = new TH1D(name.c_str(),title.c_str(),105,-0.05,1.0 );
      g->GetXaxis()->SetTitle("milliseconds");
      timeJitter_S.insert( std::make_pair(d,g));

      title=std::string("Time Jitter (relative)");
      name=std::string("timeJitter_R_")+d;
      TH1D *f = new TH1D(name.c_str(),title.c_str(),100,-100,100 );
      f->GetXaxis()->SetTitle("milliseconds");
      timeJitter_R.insert( std::make_pair(d,f));

      title=std::string("Number of hit-pixels per frame");
      name=std::string("nHitsPerEvent_")+d;
      TH1F *e = new TH1F(name.c_str(),title.c_str(),101,-4,804 );
      e->GetXaxis()->SetTitle("n(pixels)");
      nHitsPerEvt.insert( std::make_pair(d,e));

      char buffer[1024];
      sprintf(buffer,"Number of frames in %fms window",m_evtWin/1000.);
      title=std::string(buffer);
      name=std::string("nFilesPerEvent_")+d;
      TH1F *b = new TH1F(name.c_str(),title.c_str(),6,-0.5,5.5 );
      b->GetXaxis()->SetTitle("n(files)");
      nFilesPerEvt.insert(std::make_pair(d,b));
      fileCountPerEvt.insert( std::make_pair(d,0));
    }
    timeJitter_L[d]->Fill((t-c));
    timeJitter_S[d]->Fill((t-c)*1000.);

    nHitsPerEvt[d]->Fill(n);
    fileCountPerEvt[d]++;
  }
  elementsPresent->Fill(event->nElements());
  elementsWithHit->Fill(nDetectorsSeeingSomething);

  std::map<std::string,int>::const_iterator iter=fileCountPerEvt.begin();
  for(;iter!=fileCountPerEvt.end();iter++){
    std::string chip = (*iter).first;
    nFilesPerEvt[chip]->Fill((*iter).second );
    fileCountPerEvt[chip]=0;
  }

  for(unsigned int j=0;j<event->nElements();j++){
    TestBeamEventElement *element=event->getElement(j);
    std::string d=element->detectorId();
    double t=element->timeStamp();
    timeJitter_R[d]->Fill((t-referenceTime)*1000.);
  }
}

inline void TestBeamDataSummary::CopyTDCHistos(TH1F* h1a,TH1F* h1b,TH1F* h1c,TH1F* h2,TH1F* h3,TH1F* h4,TH1F* h5,TH1F* h5bis,TH1F* h6,TH1F* h7,TH1F* h8,TH1F* h9,TH1F* h10,TH1F* h11,TH1F* h12,TH1F* h13)
{
  hNumberSync=(TH1F*)h1a->Clone();
  hNumberUnsync=(TH1F*)h1b->Clone();
  hNumberPairs=(TH1F*)h1c->Clone();
  hTDCShutterTime=(TH1F*)h2->Clone();
  hTDCUnsyncTriggerTime=(TH1F*)h3->Clone();
  hSynchronisationDelay=(TH1F*)h4->Clone();
  hTDCSyncTriggerTime=(TH1F*)h7->Clone();
  hExtraUnsyncTriggers=(TH1F*)h5->Clone();
  hExtraSyncTriggers=(TH1F*)h5bis->Clone();
  if(h8) hMedipixTime=(TH1F*)h8->Clone();
  hUnmatched=(TH1F*)h6->Clone();
  if(h9) hVetraTime=(TH1F*)h9->Clone();
  if(h10)hTDCTime=(TH1F*)h10->Clone();
  if(h11)hFEI4TimeDiff=(TH1F*)h11->Clone();
  if(h12)hFEI4MatchTime=(TH1F*)h12->Clone();
  if(h13)hFEI4FractionMatched=(TH1F*)h13->Clone();
}

#endif
