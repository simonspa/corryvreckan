#ifndef TDCMANAGER_H 
#define TDCMANAGER_H 1

// Include files
#include <string>
#include <vector>
#include <math.h>

#include "TH1.h"

#include "Parameters.h"
#include "TDCFrame.h"

/** @class TDCManager
 *  
 *
 *  @author Malcolm John
 *  @date   2010-06-02
 */




//----- TDCSignal -----//

class TDCSignal {
 public:
  TDCSignal(int a,int b,float c,float d){m_fine=a;m_coarse=b;m_clock=c;m_since=d;}
  int coarse(){return m_coarse;}
  int fine(){return m_fine;}
  float clock(){return m_clock;}
  float since(){return m_since;}
 private:
  int m_fine;
  int m_coarse;
  float m_clock;
  float m_since;
};
typedef std::vector<TDCSignal> TDCSignals;




//----- TDCFile -----//

class TDCFile {
 public:
  TDCFile(int,std::string);
  TDCFrame* push_back(TDCTrigger*,bool);
  TDCFrames* frames(){return &m_frames;}
  std::vector<int>* zeroFrames(){return &m_zeroFrames;}//dhynds
  int nFramesInFile(){return m_nFramesInFile;}
  int timeStamp(){return m_timeStamp;}
  int nTriggersInFile();
  void nUnmatchedTriggers(int);
  int nUnmatchedTriggers(){return m_unmatched;}
  std::string filename(){return m_tdcFilename;}
  void trim();
 private:
  std::string m_tdcFilename;
  TDCFrames m_frames;
  std::vector<int> m_zeroFrames; //dhynds
  int m_mostRecentTimeStamp;
  int m_nFramesInFile;
  int m_timeStamp;
  int m_unmatched;
};

typedef std::vector<TDCFile*> TDCFiles;

inline TDCFile::TDCFile(int a, std::string b) 
: m_frames()
{
  m_timeStamp=a;
  m_tdcFilename=b;
  m_nFramesInFile=0;
  m_unmatched=0;
}

inline TDCFrame* TDCFile::push_back(TDCTrigger* t,bool newFrame)
{
  if(newFrame){
    m_frames.push_back(new TDCFrame(m_nFramesInFile));
    m_nFramesInFile++;
  }
  if(m_frames.size()&&t){
    m_frames.back()->push_back(t);
    m_frames.back()->timeStamp(m_timeStamp+0.001*m_frames.size());
  }
  if(0==m_frames.size()) return 0;
  return m_frames.back();
}

inline int TDCFile::nTriggersInFile()
{
  int sum=0;
  TDCFrames::iterator it=m_frames.begin();
  for(;it!=m_frames.end();it++){
    sum+=(*it)->nTriggersInFrame();
  }
  return sum;
}


inline void TDCFile::nUnmatchedTriggers(int i)
{
  m_unmatched=i;
  TDCFrames::iterator it=m_frames.begin();
  for(;it!=m_frames.end();it++){
    (*it)->nTriggersUnmatchedInSpill(i);
  }
}


inline void TDCFile::trim()
{
  TDCFrames::iterator it=m_frames.end()-1;
  if((*it)->nTriggersInFrame()==0){
    m_frames.pop_back();
  }
}





//----- TDCManager -----//

class TDCManager {
public:
  TDCManager(){}
  TDCManager(Parameters*);
  ~TDCManager(){}
  int readFile(int,std::string);
  void registerSignal(int);
  int getNTriggers(){return m_nTriggers;}
  void resetNTriggers(){m_nTriggers=0;}
  int totalNFrames(){return m_totalNFrames;}
  TDCFiles* files(){return &m_files;}
  int detectSyncDelay(std::string);
  float convert(int,unsigned int&,unsigned int&,unsigned int&);
  void store(int, int=-1,float=-999,bool=false);
  void createTimeTrendHistos(int,int);
protected:
  void clear();
private:
  Parameters *parameters;
  unsigned int m_coarseRange;
  unsigned int m_fineRange;
  unsigned int m_flagRange;
  float m_clockcycle;
  float m_previousRawClock;
  TDCSignals m_unsyncTriggers;
  TDCSignals m_syncTriggers;
  float m_openShutterTime;
  float m_shutterLength;
  bool m_shutterOpen;
  int m_nTriggers;
  int m_totalNFrames;
  TDCFiles m_files;
  float m_known08Offset;
  bool m_newFrame;
  bool m_print;
  TDCFrame* m_currentFrame;
public:
  TH1F* hNumberSync;
  TH1F* hNumberUnsync;
  TH1F* hNumberPairs;
  TH1F* hTDCShutterTime;
  TH1F* hTDCUnsyncTriggerTime;
  TH1F* hSynchronisationDelay;
  TH1F* hExtraSyncTriggers;
  TH1F* hExtraUnsyncTriggers;
  TH1F* hUnmatched;
  TH1F* hFEI4TimeDiff;
  TH1F* hFEI4MatchTime;
  TH1F* hFEI4FractionMatched;
  TH1F* hTDCSyncTriggerTime;
  TH1F* hMedipixTime;
  TH1F* hDUTTime;
  TH1F* hTDCTime;
  bool includingaDUT;
};

#endif // TDCMANAGER_H
