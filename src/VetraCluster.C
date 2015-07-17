#include <vector>

#include "VetraCluster.h"
#include "VetraMapping.h"

// ClassImp(VetraCluster)

VetraCluster::~VetraCluster(){
  m_strips.clear();
  m_adc.clear();
  m_stripPosition.clear();
}

VetraCluster::VetraCluster(){
  m_triggerNr = -1;
  m_trackPosition = -1;
  m_clusterSize = -1;
  m_syncDelay = -1;
  m_frameTimeStamp = -1;
  m_timeAfterShutterOpen = -1;
  m_strips.clear();
  m_adc.clear();
  m_stripPosition.clear();
  m_etaDistributionFile = "";
}

void VetraCluster::clear(){
  m_triggerNr = -1;
  m_trackPosition = -1;
  m_clusterSize = -1;
  m_syncDelay = -1;
  m_frameTimeStamp = -1;
  m_timeAfterShutterOpen = -1;
  m_strips.clear();
  m_adc.clear();
  m_stripPosition.clear();
  m_etaDistributionFile = "";
}

void VetraCluster::addStripToVetraCluster(int str, int signal, double position){
//   cout<<"0. m_strip.size()="<<m_strips.size()<<" str="<<str<<"  adc="<<signal<<endl;
  if(m_strips.size() == 0){
    m_strips.push_back(str);
    m_adc.push_back(signal);
    m_stripPosition.push_back(position);
  }
  else{
    std::vector<int>::iterator iterStr = m_strips.begin();
    std::vector<int>::iterator iterAdc = m_adc.begin();
    std::vector<double>::iterator iterPos = m_stripPosition.begin();
    unsigned int idx=0;
    for(;iterStr!=m_strips.end();iterStr++){
//       cout<<"  test1: "<<idx<<endl;
//       cout<<" str="<<str << " m_strips.at("<<idx<<")="<<m_strips.at(idx)<<endl;
      if(str < m_strips.at(idx)){
	iterStr=m_strips.insert(iterStr, str);
	iterAdc=m_adc.insert(iterAdc, signal);
	iterPos=m_stripPosition.insert(iterPos, position);
  	return;
      }
      if(idx<m_strips.size()-1){
	idx ++;
	iterAdc ++;
	iterPos ++;
      }
    }
    m_strips.push_back(str);
    m_adc.push_back(signal);
    m_stripPosition.push_back(position);
  }
//   cout<<"3. m_strip.size()="<<m_strips.size()<<" str="<<str<<"  adc="<<signal<<endl;
//   cout<<""<<endl;
}

void VetraCluster::addStripToVetraCluster(int str, int signal, int wsignal, double position){
//   cout<<"0. m_strip.size()="<<m_strips.size()<<" str="<<str<<"  adc="<<signal<<endl;
  if(m_strips.size() == 0){
    m_strips.push_back(str);
    m_adc.push_back(signal);
    m_wadc.push_back(wsignal);
    m_stripPosition.push_back(position);
  }
  else{
    std::vector<int>::iterator iterStr = m_strips.begin();
    std::vector<int>::iterator iterAdc = m_adc.begin();
    std::vector<int>::iterator iterWAdc = m_wadc.begin();
    std::vector<double>::iterator iterPos = m_stripPosition.begin();
    unsigned int idx=0;
    for(;iterStr!=m_strips.end();iterStr++){
      if(str < m_strips.at(idx)){
	iterStr=m_strips.insert(iterStr, str);
	iterAdc=m_adc.insert(iterAdc, signal);
	iterWAdc=m_wadc.insert(iterWAdc, signal);
	iterPos=m_stripPosition.insert(iterPos, position);
  	return;
      }
      if(idx<m_strips.size()-1){
	idx ++;
	iterAdc ++;
	iterWAdc ++;
	iterPos ++;
      }
    }
    m_strips.push_back(str);
    m_adc.push_back(signal);
    m_wadc.push_back(wsignal);
    m_stripPosition.push_back(position);
  }
}

int VetraCluster::getClusterSize() {
  if((m_strips.size()==m_adc.size())){
    m_clusterSize=m_strips.size();
    return m_clusterSize;
  }
  else
    return -1;
}

int VetraCluster::getSumAdcCluster(){
  int sumADC = 0;
  for(unsigned int i=0;i<m_adc.size();i++){
    sumADC += m_adc.at(i);
  }
  return sumADC;
}

float VetraCluster::getCogCluster(){
  if(m_stripPosition.size()==0) return 0;
  float cog=0;
  for(unsigned int i=0;i<m_strips.size();i++){
    cog += m_stripPosition.at(i)*m_adc.at(i)/getSumAdcCluster();
  }
  return cog;
}

float VetraCluster::getEtaVariable(){
  float eta=0;
  if(m_adc.size()==2){
    float V_L=m_adc.at(0);
    float V_R=m_adc.at(1);
    eta = V_R/(V_L+V_R);
  }
  return eta;
}
float VetraCluster::getEtaInverse(){
  float eta=0;
  if(m_adc.size()==2){
    float V_L=m_adc.at(0);
    float V_R=m_adc.at(1);
    eta = V_L/(V_L+V_R);
  }
  return eta;
}

float VetraCluster::getWeightedEtaVariable(){
  float eta=0;
  if(m_wadc.size()==2){
    float V_L=m_wadc.at(0);
    float V_R=m_wadc.at(1);
    eta = V_R/(V_L+V_R);
  }
  return eta;
}

float VetraCluster::getVetraClusterPosition(){
  if(getClusterSize()!=2)
    return getCogCluster();
  if(m_etaDistributionFile=="")
    return getCogCluster();
  else{
    return getCogCluster();
//     cout<<"test"<<endl;
  }
}

void VetraCluster::setTimestamp(double timeS, float shutterO)
{
  m_frameTimeStamp = timeS; 
  m_timeAfterShutterOpen = shutterO;
}


bool VetraCluster::is40microns(){
  float cogCluster = getCogCluster();
  if(cogCluster<27.92)
    return 1;
  else 
    return 0;
}

bool VetraCluster::is60microns(){
  float cogCluster = getCogCluster();
  if(cogCluster>27.92)
    return 1;
  else 
    return 0;
}
