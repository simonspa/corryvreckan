// $Id: TestBeamEvent.C,v 1.8 2010-06-11 13:05:31 mjohn Exp $
// Include files 
#include "TestBeamEvent.h"

ClassImp(TestBeamEvent)

//-----------------------------------------------------------------------------
// Implementation file for class : TestBeamEvent
//
// 2009-06-22 : Malcolm John
//-----------------------------------------------------------------------------

TestBeamEvent::TestBeamEvent() : m_count(0) {}
TestBeamEvent::~TestBeamEvent(){} 

void TestBeamEvent::push_back(double t, std::string s, std::string d, RowColumnEntries e)
{
  TestBeamEventElement element;
  element.timeStamp(t);
  element.sourceName(s);
  element.detectorId(d);
  element.data(e);
  m_data.push_back(element);
  if(s==std::string("timepix")){
    m_timepixIndex.push_back(m_count);
  }else if(s==std::string("medipix")){
    m_medipixIndex.push_back(m_count);
  }else{
    std::cout<<"Warning: do not recognise source type '"<<s<<"'"<<std::endl;
  }
  m_count++;
  if(e.size()) m_countWithHit++;
}

void TestBeamEvent::push_back(TDCFrame* f)
{
  m_tdcData.push_back(*f);
}

void TestBeamEvent::clear()
{
  for(unsigned int j=0;j<nElements();j++){
    getElement(j)->clear();
  }
  m_count=0;
  m_data.clear();
  m_tdcData.clear();
  m_medipixIndex.clear();
  m_timepixIndex.clear();
  m_tdcIndex.clear();
}

unsigned int TestBeamEvent::nElements(){return m_count;}
unsigned int TestBeamEvent::nElementsWithHits(){return m_countWithHit;}
unsigned int TestBeamEvent::nMediPixElements(){return m_medipixIndex.size();}
unsigned int TestBeamEvent::nTimePixElements(){return m_timepixIndex.size();}
unsigned int TestBeamEvent::nTDCElements(){return m_tdcData.size();}

TestBeamEventElement* TestBeamEvent::getElement(unsigned int i){return &m_data[i];}
TestBeamEventElement* TestBeamEvent::getMediPixElement(unsigned int i){return &m_data[m_medipixIndex[i]];}
TestBeamEventElement* TestBeamEvent::getTimePixElement(unsigned int i){return &m_data[m_timepixIndex[i]];}
TDCFrame* TestBeamEvent::getTDCElement(unsigned int i){return &m_tdcData[i];}
