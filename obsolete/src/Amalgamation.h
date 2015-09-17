// $Id: Amalgamation.h,v 1.8 2010/06/11 13:05:30 mjohn Exp $
#ifndef AMALGAMATION_H 
#define AMALGAMATION_H 1

// Include files
#include <vector>
#include <string>
#include <sstream>

#include "Parameters.h"
#include "IndexEntry.h"
#include "TestBeamDataSummary.h"
#include "TDCManager.h"

#include "TH2.h"

typedef std::pair< std::string, int > FileTime;
typedef std::vector< FileTime > FilesTimes;

class SciFiTrigger
{
public:
	SciFiTrigger(long int t,long int d){m_time=t;m_deltat=d;}
  virtual ~SciFiTrigger(){}
  float time(){return m_time;}
  float timeSince(){return m_deltat;}
  void addhit(int c,float a){m_hits.push_back(std::make_pair(c,a));}
  int nhits(){return m_hits.size();}
  int channel(int i){return m_hits[i].first;}
  float adc(int i){return m_hits[i].second;}
  void clear(){m_hits.clear();}
private:
	long int m_time;
  long int m_deltat;
  std::vector< std::pair<int,float> > m_hits;
};
typedef std::vector< SciFiTrigger > SciFiTriggers;
typedef std::vector< SciFiTriggers > SciFiFrames;

/** @class Amalgamation Amalgamation.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class Amalgamation
{
public: 
  /// Standard constructor
  Amalgamation(){}
  virtual ~Amalgamation(){}
  Amalgamation(Parameters*); 
  int indexPixFiles(std::string);
  int indexVetraFile();
  int indexFEI4Data();
  int indexSciFiData();
  int indexTDCFiles();
  int saveTDCData();
  int matchTDCandTelescope();
  int buildEventFile();
  int checkTimeRange(int,int);
protected:
  std::string de(int);
private:
  TestBeamDataSummary *summary;
  Parameters *parameters;
  Index index;
  bool compressedRawFormat;
  std::string slash;
  std::string hash;
  TDCManager tdc;
  int firstDate; 
  int firstTime;
  int lastDate; 
  int lastTime;
  int firstTimepixTime;
  int lastTimepixTime;
};

inline std::string Amalgamation::de(int i)
{
  int j=i/86400;
  int o=j/100;
  int d=j%100;
  int t=i%86400;
  int h=t/3600;
  int m=t%3600/60;
  int s=t%60;
  std::stringstream r;
  r<<(d<10?"0":"")<<d<<"/"<<(o<10?"0":"")<<o<<" "<<(h<10?"0":"")<<h<<":"<<(m<10?"0":"")<<m<<":"<<(s<10?"0":"")<<s;
  return r.str();
}

#endif // AMALGAMATION_H
