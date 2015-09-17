#include <bitset>
#include <limits>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "TDCManager.h"

TDCManager::TDCManager(Parameters* p)
: m_coarseRange(11)//bits
, m_fineRange(10)//bits
, m_flagRange(4)//bits
, m_clockcycle(25.0)//ns
, m_unsyncTriggers()
, m_syncTriggers()
, m_files()
, m_known08Offset(0)
, m_print(false)
, includingaDUT(false)
{
  parameters=p;
  m_nTriggers=0;
  m_newFrame=false;
  m_currentFrame=NULL;
  
  //m_print=true; //VERY VERBOSE TDC SIGNAL PRINTING
  
  hTDCShutterTime = new TH1F("hTDCShutterTime","TDC: T(close shutter) - T(open shutter)",100000,-0.5,99999.5);
  hTDCUnsyncTriggerTime = new TH1F("hTDCUnsyncTriggerTime","TDC: T(unsync.) - T(open shutter)",100000,-0.5,99999.5);
  hTDCSyncTriggerTime = new TH1F("hTDCSyncTriggerTime","TDC: T(sync.) - T(open shutter)",100000,-0.5,99999.5);
  hSynchronisationDelay = new TH1F("hSynchronisationDelay","TDC: T(sync.) - T(unsync.)",1000,10,75);
  hExtraUnsyncTriggers = new TH1F("hExtraUnsyncTriggers","TDC: N(unmatched)/frame (normalised)",21,-0.5,20.5);
  hExtraSyncTriggers = new TH1F("hExtraSyncTriggers","TDC: N(unmatched)/frame (normalised)",21,-0.5,20.5);
  hNumberSync = new TH1F("hNumberSync","TDC: N(signals)/frame",251,-0.5,250.5);
  hNumberUnsync = new TH1F("hNumberUnsync","TDC: N(signals)/frame",251,-0.5,250.5);
  hNumberPairs = new TH1F("hNumberPairs","TDC: N(signals)/frame",251,-0.5,250.5);
  hUnmatched = new TH1F("hUnmatched","(N_tr(vetra)-N_tr(tdc))/spill",19,-9.5,9.5);
  hFEI4TimeDiff = new TH1F("hFEI4TimeDiff","#DeltaT_{trig}(FEI4)-#DeltaT_{trig}(TDC)",101,-51,51);
  hFEI4MatchTime = new TH1F("hFEI4MatchTime","Time difference between matched FEI4 hits and TDC entries",1000,-2,2);
  hFEI4FractionMatched = new TH1F("hFEI4FractionMatched","Fraction of TDC triggers matched to FEI4 hits in each frame",100,0,1);

  hMedipixTime = 0;
  hDUTTime = 0;
  hTDCTime = 0;

}

float TDCManager::convert(int dec, unsigned int& fine, unsigned int& coarse, unsigned int& flag)
{
  typedef std::bitset<std::numeric_limits<unsigned int>::digits> bitset; 
  
  bitset bits = bitset(dec);
  fine    = (bits                             &bitset(int(pow(2,m_fineRange)-1))  ).to_ulong();
  coarse  = (bits>>m_fineRange                &bitset(int(pow(2,m_coarseRange)-1))).to_ulong();
  flag    = (bits>>(m_fineRange+m_coarseRange)&bitset(int(pow(2,m_flagRange)-1))  ).to_ulong();
  
  return m_clockcycle*(float(coarse)+float(fine)/pow(2,m_fineRange));
}

void TDCManager::registerSignal(int dec)
{
  unsigned int fine,coarse,flag;  
  float rawClock=convert(dec,fine,coarse,flag);
  
  //detect coarse-timer wrapping. 10% of coarseRange = 205*25ns ~5us.
  float depth = m_clockcycle*pow(2,m_coarseRange);
  float wrapThreshold = -0.1*m_clockcycle*pow(2,m_coarseRange);
  if( rawClock-m_previousRawClock < wrapThreshold ){
    m_openShutterTime-=depth;
  }
  if(12==flag){
	  int nMatchedSignals = getNTriggers();
    hNumberSync->Fill(nMatchedSignals+m_syncTriggers.size());
    hNumberUnsync->Fill(nMatchedSignals+m_unsyncTriggers.size());
    hNumberPairs->Fill(nMatchedSignals);
    hExtraSyncTriggers->Fill(m_syncTriggers.size());
    hExtraUnsyncTriggers->Fill(m_unsyncTriggers.size());
    while(m_unsyncTriggers.size()){store(-1,0);}
    while(m_syncTriggers.size()){store(0);}
    if(m_currentFrame!=NULL){
      m_currentFrame->sort();
      if(m_print) std::cout <<"  closed frame "<< m_files.back()->frames()->size() <<" after " <<int(m_shutterLength/1000)<<"us & " << m_currentFrame->nTriggersInFrame()<< " triggers "<< std::endl;
    }
    m_openShutterTime=rawClock;
    m_shutterOpen=true;
    m_currentFrame=NULL;
    m_newFrame=true;
    resetNTriggers();
    clear();
  }
  if(m_print && (flag!=4||m_shutterOpen) ) std::cout<<"("<<flag<<") "/*<<coarse<<":"<<fine<<" = "*/<<rawClock<<"ns T="<<std::setprecision(10)<<rawClock-m_openShutterTime<<"ns     "<<std::flush;
  if(13==flag){
    m_shutterLength=rawClock-m_openShutterTime;
    if(m_currentFrame==NULL){
      //does little more than increment nFrames in the case of an [triggerless] empty frame
      m_currentFrame=m_files.back()->push_back(0,m_newFrame);
      m_currentFrame->openTime(m_openShutterTime);
    }
  	m_currentFrame->timeShutterIsOpen(m_shutterLength);
    hTDCShutterTime->Fill(m_shutterLength/1000.);//ns->us
    m_shutterOpen=false;
    m_totalNFrames++;
  }
  if(0==flag){
    m_unsyncTriggers.push_back(TDCSignal(fine,coarse,rawClock,rawClock-m_openShutterTime));
  }
  if(8==flag){
    m_syncTriggers.push_back(TDCSignal(fine,coarse,rawClock,rawClock-m_openShutterTime));
  }
  if(0==flag||8==flag){
    for(unsigned int iS=0;iS<m_syncTriggers.size();iS++){
      for(unsigned int iU=0;iU<m_unsyncTriggers.size();iU++){
        float delay=-999;
        float sClock=m_syncTriggers[iS].clock();
        float uClock=m_unsyncTriggers[iU].clock();
        if( sClock-uClock < wrapThreshold ) sClock+=depth;
        if(fabs(sClock-uClock-m_known08Offset)<m_clockcycle){
          delay=sClock-uClock;
          store(iS,iU,delay,(0==flag));
        }
        if(!m_syncTriggers.size()) break;
      }
    }
  }
  m_previousRawClock=rawClock;
}

void TDCManager::store(int iS,int iU,float delay,bool syncBeforeUnsync)
{
  TDCTrigger* trigger = 0;
  if(iS<0){
  	trigger = new TDCTrigger(m_unsyncTriggers[iU].since(),m_unsyncTriggers[iU].coarse(),m_unsyncTriggers[iU].fine(),0);
  }else{
  	trigger = new TDCTrigger(m_syncTriggers[iS].since(),m_syncTriggers[iS].coarse(),m_syncTriggers[iS].fine(),delay);
  }
  if(m_currentFrame or m_newFrame){
    int nf=m_files.back()->frames()->size();
  	if(nf>0 or (iS>=0&&iU>=0)){
		  m_currentFrame=m_files.back()->push_back(trigger,m_newFrame);
  		if(m_newFrame){m_currentFrame->openTime(m_openShutterTime);}
    	if(m_print){
      	if(iS>=0&&iU>=0) std::cout<<"syn["<<iS<<"/"<<m_syncTriggers.size()<<"]-usyn["<<iU<<"/"<<m_unsyncTriggers.size()<<"] gives delay = "<<delay<<"ns (nT="<<m_nTriggers<<" in nF="<<nf<<")"<<std::endl;
      	if(iU<0) std::cout<<"Unmatched syn["<<iS<<"/"<<m_syncTriggers.size()<<"] assign delay = "<<delay<<"ns (nT="<<m_nTriggers<<" in nF="<<nf<<")"<<std::endl;
      	if(iS<0) std::cout<<"Unmatched unsyn["<<iU<<"/"<<m_unsyncTriggers.size()<<"] assign delay = "<<0<<"ns (nT="<<m_nTriggers<<" in nF="<<nf<<")"<<std::endl;
    	}
	  	m_newFrame=false;
  		m_nTriggers++;
    	if( syncBeforeUnsync) hSynchronisationDelay->Fill(delay);
    	if(!syncBeforeUnsync) hSynchronisationDelay->Fill(delay);
    	if(iU>=0) hTDCUnsyncTriggerTime->Fill(m_unsyncTriggers[iU].since()/1000.);//ns->us
  	}
  }
  if(iS>=0) m_syncTriggers.erase(m_syncTriggers.begin()+iS);
  if(iU>=0) m_unsyncTriggers.erase(m_unsyncTriggers.begin()+iU);
}

void TDCManager::clear()
{
  m_unsyncTriggers.clear();
  m_syncTriggers.clear();
}

int TDCManager::readFile(int ts, std::string fullpath)
{
	int status=0;
  if(0==m_files.size()) status=detectSyncDelay(fullpath);
  if(status) return 1;
  std::fstream file_op(fullpath.c_str(),std::ios::in);
  
  if(m_print
//     ||fullpath==std::string("RawDataLocal/Run3589/tdc/TDC_08262011_020616AM.txt")
  ){m_print=true;}else{m_print=false;}  
  if(m_print) std::cout << " TDCfile: " << fullpath << std::endl;

  int line=0;
  std::string str;
  bool newfile=false;
  if(     m_files.size()==0             ){ newfile=true; }
  else if(m_files.back()->timeStamp()<ts){ newfile=true; }  
  while( std::getline(file_op,str) ){line++;}
	if(line<5) newfile=false;

  if(newfile)	m_files.push_back(new TDCFile(ts,fullpath));

	file_op.clear();
	file_op.seekg(0,std::fstream::beg);
  while( std::getline(file_op,str) ){
    registerSignal(atoi(str.c_str()));
  }
  //m_files.back()->trim();
  return (not newfile);
}

int TDCManager::detectSyncDelay(std::string fullpath)
{ 
	// average delay between "0" and "8" triggers.
  // simple algorithm. Doesn't bother with reordering signals. 
  // So only uses the cases where "8"s follow "0"s
  int line=0;
  int npairs=0;
  float sumDiff=0;
  char str[2000];
  float zeroClock=0;
  unsigned int fine,coarse,flag;
  std::fstream file_op(fullpath.c_str(),std::ios::in);
  bool seen0readyfor8=false;
  while(!file_op.eof()){
    line++;
    if(line<=2) continue;
    file_op.getline(str,2000);
    convert(atoi(str),fine,coarse,flag);
    float clock=float(coarse)+float(fine)/pow(2,m_fineRange);
    //if(4!=flag) std::cout<<" ("<<flag<<") "<<clock*m_clockcycle<<"ns  "<<std::flush;
    if(0==flag){
      zeroClock=clock;
      seen0readyfor8=true;
    }
    if(8==flag){
      float diff=clock-zeroClock;
      seen0readyfor8=false;
      if(diff>0&&fabs(diff)<10){
				sumDiff+=diff;
				npairs++;
        //std::cout<<m_clockcycle*diff<<"ns <DT>="<<m_clockcycle*sumDiff/float(npairs)<<"ns"<<std::endl;
      }
    }
  }
  if(npairs) m_known08Offset=m_clockcycle*sumDiff/float(npairs);
  if(parameters->verbose) std::cout<<"\nDetecting an average TDC sync. offset signal[8]-signal[0] of: "<<m_known08Offset<<" ns   "
  																 <<(npairs?"":" WARNING!!! NO sync-unsync pairs detected!!! (continue though)")<<std::endl;
  hSynchronisationDelay->GetXaxis()->Set(1000,m_known08Offset-m_clockcycle,m_known08Offset+m_clockcycle);
  return 0;
}

void TDCManager::createTimeTrendHistos(int firstTime, int lastTime)
{
	hMedipixTime = new TH1F("hTelescopeTime","Run time",lastTime-firstTime+200,-100,lastTime-firstTime+100);
	hDUTTime = new TH1F("hDUTTime","Run time",lastTime-firstTime+200,-100,lastTime-firstTime+100);
	hTDCTime = new TH1F("hTDCTime","Run time",lastTime-firstTime+200,-100,lastTime-firstTime+100);
}
