// $Id: TestBeamEventElement.h,v 1.7 2010-06-11 13:05:31 mjohn Exp $
#ifndef TESTBEAMEVENTELEMENT_H 
#define TESTBEAMEVENTELEMENT_H 1

// Include files
#include<iostream>
#include<ctype.h>
#include <string>
#include <cstring>

#include "TObject.h"

#include "RowColumnEntry.h"

/** @class TestBeamEventElement TestBeamEventElement.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-22
 */

class TestBeamEventElement : public TObject {
public: 
  TestBeamEventElement();
  void timeStamp(double);
  void sourceName(std::string);
  void detectorId(std::string);
  void data(RowColumnEntries);

  double timeStamp();
  std::string sourceName();
  std::string detectorId();
  RowColumnEntries* data();

  virtual ~TestBeamEventElement( ){}
  void clear();

  ClassDef(TestBeamEventElement,1)
protected:

private:
  double m_time;
  char m_source[64];
  char m_detector[64];
  RowColumnEntries m_rowColumnEntries;
};
inline TestBeamEventElement::TestBeamEventElement() 
{
	std::strcpy(m_source,"NULL");
  std::strcpy(m_detector,"NULL");
}

inline void TestBeamEventElement::timeStamp(double t){m_time=t;}
inline void TestBeamEventElement::sourceName(std::string s){  std::strcpy(m_source,s.c_str());}
inline void TestBeamEventElement::detectorId(std::string d){std::strcpy(m_detector,d.c_str());}
inline void TestBeamEventElement::data(RowColumnEntries r){m_rowColumnEntries=r;}

inline double TestBeamEventElement::timeStamp(){return m_time;}
inline std::string TestBeamEventElement::sourceName(){return std::string(m_source);}
inline std::string TestBeamEventElement::detectorId(){return std::string(m_detector);}
inline RowColumnEntries* TestBeamEventElement::data( ){return &m_rowColumnEntries;}

inline void TestBeamEventElement::clear(){
  RowColumnEntries::iterator it=m_rowColumnEntries.begin();
  for(;it!=m_rowColumnEntries.end();it++){
    delete (*it); 
  }
}
#endif // TESTBEAMEVENTELEMENT_H
