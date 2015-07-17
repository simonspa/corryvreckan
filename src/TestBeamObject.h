// $Id: TestBeamObject.h,v 1.6 2010-06-07 11:08:34 mjohn Exp $
#ifndef TESTBEAMOBJECT_H 
#define TESTBEAMOBJECT_H 1

// Include files
#include <vector>

/** @class TestBeamObject TestBeamObject.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-05
 */

enum {COPY,CLONE};

class TestBeamObject{
public:
  TestBeamObject(){}
  TestBeamObject(char* n){m_name=n;}
  ~TestBeamObject(){}
  std::string name(){return m_name;}
protected:
  std::string m_name;
};
typedef std::vector<TestBeamObject*> TestBeamObjects;

#endif // TESTBEAMOBJECT_H
