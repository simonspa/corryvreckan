#ifndef ALGORITHM_H 
#define ALGORITHM_H 1

// Include files
#include <string>
#include "TStopwatch.h"

class Clipboard;
class TestBeamEvent;

/*
 *  @author Malcolm John
 *  @date   2009-07-01
 */

class Algorithm{
public:
  Algorithm(){}
  Algorithm(std::string n){m_name=n;sw=new TStopwatch();}
  virtual ~Algorithm(){}
  virtual void initial(){}
  virtual void run(TestBeamEvent*,Clipboard*){}
  virtual void end(){}
  std::string name(){return m_name;}
  TStopwatch* stopwatch(){return sw;}
protected:
  TStopwatch* sw;
  std::string m_name;
};
#endif
