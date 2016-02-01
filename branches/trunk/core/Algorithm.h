#ifndef ALGORITHM_H 
#define ALGORITHM_H 1

// Include files
#include <string>
#include "TStopwatch.h"
#include "Clipboard.h"
#include "Parameters.h"
#include "Tee.h"

//-------------------------------------------------------------------------------
// The algorithm class is the base class that all user algorithms are built on. It
// allows the analysis class to hold algorithms of different types, without knowing
// what they are, and provides the functions initialise, run and finalise. It also
// gives some basic tools like the "tcout" replacement for "cout" (appending the
// algorithm name) and the stopwatch for timing measurements.
//-------------------------------------------------------------------------------

enum StatusCode {
  Success,
  NoData,
  Failure,
} ;

class Algorithm{

public:

  // Constructors and destructors
  Algorithm(){}
  Algorithm(string name){
    m_name = name;
    m_stopwatch = new TStopwatch();
    tcout.m_algorithmName = m_name;
  }
  virtual ~Algorithm(){}
  
  // Three main functions - initialise, run and finalise. Called for every algorithm
  virtual void initialise(Parameters*){}
  virtual StatusCode run(Clipboard*){}
  virtual void finalise(){}
  
  // Methods to get member variables
  string getName(){return m_name;}
  TStopwatch* getStopwatch(){return m_stopwatch;}
  
  // Simple cout replacement
  tee tcout;
  bool debug;

protected:

  // Member variables
  Parameters* parameters;
  TStopwatch* m_stopwatch;
  string m_name;
  
};


#endif
