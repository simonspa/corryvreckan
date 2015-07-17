#ifndef TDCFRAME_H 
#define TDCFRAME_H 1

// Include files
#include <string>
#include <vector>
#include <math.h>
#include <iostream>
#include <algorithm>

#include "TH1.h"

#include "TDCTrigger.h"
#include "TestBeamEventElement.h"

extern bool sortTriggers( TDCTrigger* t1, TDCTrigger* t2);
extern bool sortUnsyncTriggers( TDCTrigger* t1, TDCTrigger* t2);

/** @class TDCFrame
 *
 *  @author Malcolm John
 *  @date   2010-06-02
 *
 */

class TDCFrame : public TestBeamEventElement  {
 public:
  TDCFrame(){}
  TDCFrame(int);
  void push_back(TDCTrigger*);
  float openTime(){return m_openTime;}
  void openTime(float o){m_openTime=o;}
  float timeShutterIsOpen(){return m_length;}
  void timeShutterIsOpen(float m){m_length=m;}
  TDCTriggers* triggers(){return &m_triggers;}
  int nTriggersInFrame(){return m_nTriggersInFrame;}
  int nTriggersUnmatchedInSpill(){return m_nTriggersUnmatchedInSpill;}
  void nTriggersUnmatchedInSpill(int i){m_nTriggersUnmatchedInSpill=i;}
  int positionInSpill(){return m_numberInSpill;}
  void positionInSpill(int i){m_numberInSpill=i;}
  void sort(){std::sort(m_triggers.begin(),m_triggers.end(),sortTriggers);}
  void sortUnsync(){std::sort(m_triggers.begin(), m_triggers.end(), sortUnsyncTriggers);}
  void clear();
  

  ClassDef(TDCFrame,1);  //Event structure

 private:
  TDCTriggers m_triggers;
  int m_nTriggersInFrame;
  int m_nTriggersUnmatchedInSpill;
  int m_numberInSpill;
  float m_openTime;
  float m_length;
};

typedef std::vector<TDCFrame*> TDCFrames;

inline TDCFrame::TDCFrame(int i) 
: m_triggers()
, m_nTriggersInFrame(0)
, m_nTriggersUnmatchedInSpill(0)
, m_openTime(0)
, m_length(0)
{
  m_numberInSpill=i;
}

inline void TDCFrame::push_back(TDCTrigger* t)
{
  m_triggers.push_back(t);
  m_nTriggersInFrame++;
}

inline void TDCFrame::clear()
{
  TDCTriggers::iterator it=m_triggers.begin();
  for(;it!=m_triggers.end();it++){
    delete (*it);
  }
}

#endif // TDCFRAME_H
