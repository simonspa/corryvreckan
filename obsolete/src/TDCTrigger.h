#ifndef TDCTRIGGER_H 
#define TDCTRIGGER_H 1

// Include files
#include <string>
#include <vector>
#include <math.h>
#include <map>

#include "TH1.h"
#include "TObject.h"

#include "Parameters.h"

/** @class TDCTrigger
 *
 *  @author Malcolm John
 *  @date   2010-06-02
 *
 */

class TDCTrigger : public TObject {
 public:
  TDCTrigger(){}
  TDCTrigger(float,int,int,float);
  float timeAfterShutterOpen(){return m_timeAfter;}
  int coarseTimeWord(){return m_coarse;}
  int fineTimeWord(){return m_fine;}
  float syncDelay(){return m_delay;}
  void push_back(int,double,long int);
  void push_back(int,int,int,int);
  void push_back(int,int,int,int,int,int,int);
	void clear();
  int nVetraHits(){return m_vetraADC.size();}
  int vetraLink(int i){return m_vetraLink[i];}
  int vetraChannel(int i){return m_vetraChannel[i];}
  int vetraStrip(int i){return m_vetraStrip[i];}
  int vetraADC(int i){return m_vetraADC[i];}
  int nFibreTrackerHits(){return m_fibreTrackerADC.size();}
  int fibreTrackerChannel(int i){return m_fibreTrackerChannel[i];}
  double fibreTrackerADC(int i){return m_fibreTrackerADC[i];}
  long int fibreTrackerClock(int i){return m_fibreTrackerClock[i];}
  int fei4Count(){return m_fei4Count;}
  int nFei4Hits(){return m_fei4Col.size();}
	int firstFei4Hit(){return m_fei4FirstBcid;}
  std::vector<int>* hitNumber(){return &m_hitNumber;}
  std::vector<int>* fei4BCIDs(int hit_no){return &m_fei4BCID[hit_no];}//dhynds1
  std::vector<int>* fei4LV1ID(int hit_no){return &m_fei4LV1ID[hit_no];}//dhynds1
  std::vector<int>* fei4Col(int hit_no){return &m_fei4Col[hit_no];}
  std::vector<int>* fei4Row(int hit_no){return &m_fei4Row[hit_no];}
  std::vector<int>* fei4Tot(int hit_no){return &m_fei4Tot[hit_no];}
  int clock(){return m_clock;}
  void clock(int c){m_clock=c;}

  ClassDef(TDCTrigger,1);  //Event structure

 private:
  std::vector<int> m_vetraChannel;
  std::vector<int> m_vetraLink;
  std::vector<int> m_vetraADC;
  std::vector<int> m_vetraStrip;
  
  std::vector<int> m_fibreTrackerChannel;
  std::vector<double> m_fibreTrackerADC;
  std::vector<long> m_fibreTrackerClock;
  
  float m_timeAfter;
  int m_coarse;
  int m_fine;
  int m_clock;
  float m_delay;
  int m_fei4Count;
	int m_fei4FirstBcid;
  std::vector<int> m_hitNumber;
  std::map<int,std::vector<int> > m_fei4LV1ID;//dhynds1
  std::map<int,std::vector<int> > m_fei4BCID;//dhynds1
  std::map<int,std::vector<int> > m_fei4Col;
  std::map<int,std::vector<int> > m_fei4Row;
  std::map<int,std::vector<int> > m_fei4Tot;
};

typedef std::vector<TDCTrigger*> TDCTriggers;

inline TDCTrigger::TDCTrigger(float a,int b,int c,float d)
{
  m_timeAfter=a;
  m_coarse=b;
  m_fine=c;
  m_delay=d;
}

inline void TDCTrigger::push_back(int chan, double adc, long int clock)
{
  m_fibreTrackerChannel.push_back(chan);
  m_fibreTrackerADC.push_back(adc);
  m_fibreTrackerClock.push_back(clock);
}

inline void TDCTrigger::push_back(int link, int chan, int adc, int strip)
{
  m_vetraLink.push_back(link);
  m_vetraChannel.push_back(chan);
  m_vetraADC.push_back(adc);
  m_vetraStrip.push_back(strip);
}

inline void TDCTrigger::push_back(int tdid, int lv1id, int bcid, int col, int row, int tot, int hit_no)
{
  m_fei4Count=tdid;
	if(m_fei4BCID.size()==0){
		m_fei4FirstBcid=bcid;
	}
  m_hitNumber.push_back(hit_no);
  m_fei4BCID[hit_no].push_back(bcid);
  m_fei4LV1ID[hit_no].push_back(lv1id);
  m_fei4Col[hit_no].push_back(col);
  m_fei4Row[hit_no].push_back(row);
  m_fei4Tot[hit_no].push_back(tot);
}

inline void TDCTrigger::clear()
{
	m_fei4LV1ID.clear();
  m_fei4BCID.clear();
  m_fei4Col.clear();
  m_fei4Row.clear();
	m_fei4Tot.clear();
}

#endif // TDCTRIGGER_H
