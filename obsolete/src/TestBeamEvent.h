// $Id: TestBeamEvent.h,v 1.8 2010-06-11 13:05:31 mjohn Exp $
#ifndef TESTBEAMEVENT_H 
#define TESTBEAMEVENT_H 1

// Include files
#include <iostream>
#include <ctype.h>
#include <string>

#include "TObject.h"

#include "TestBeamEventElement.h"
#include "TDCFrame.h"

/** @class TestBeamEvent TestBeamEvent.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-22
 */

class TestBeamEvent : public TObject {
public: 
  TestBeamEvent( ); 
  virtual ~TestBeamEvent( );
  void push_back(double, std::string, std::string, RowColumnEntries);
  void push_back(TDCFrame*);
  void doesNothing(){};
  void clear();

  unsigned int nElements();
  unsigned int nElementsWithHits();
  unsigned int nMediPixElements();
  unsigned int nTimePixElements();
  unsigned int nTDCElements();

  TestBeamEventElement* getElement(unsigned int i);
  TestBeamEventElement* getMediPixElement(unsigned int i);
  TestBeamEventElement* getTimePixElement(unsigned int i);
  TDCFrame* getTDCElement(unsigned int i);

  ClassDef(TestBeamEvent,1)  //Event structure
private:
  unsigned int m_count;
  unsigned int m_countWithHit;
  std::vector< unsigned int > m_medipixIndex;
  std::vector< unsigned int > m_timepixIndex;
  std::vector< unsigned int > m_tdcIndex;
  std::vector< TestBeamEventElement > m_data;
  std::vector< TDCFrame > m_tdcData;
};

#endif // TESTBEAMEVENT_H
