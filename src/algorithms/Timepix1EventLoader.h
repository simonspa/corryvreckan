#ifndef Timepix1EventLoader_H
#define Timepix1EventLoader_H 1

#include "core/Algorithm.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include <sstream>
#include <fstream>
#include <string>

namespace corryvreckan {
  class Timepix1EventLoader : public Algorithm {

  public:
    // Constructors and destructors
    Timepix1EventLoader(Configuration config, Clipboard* clipboard);
    ~Timepix1EventLoader(){}

    // Functions
    void initialise(Parameters*);
    StatusCode run(Clipboard*);
    void finalise();

    void processHeader(string,string&,long long int&);

    // Member variables
    int m_eventNumber;
    string m_inputDirectory;
    vector<string> m_inputFilenames;
    bool m_fileOpen;
    int m_fileNumber;
    long long int m_eventTime;
    ifstream m_currentFile;
    string m_currentDevice;
    bool m_newFrame;
    string m_prevHeader;
  };
}
#endif // Timepix1EventLoader_H
