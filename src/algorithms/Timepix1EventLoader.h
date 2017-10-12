#ifndef Timepix1EventLoader_H
#define Timepix1EventLoader_H 1

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"

namespace corryvreckan {
    class Timepix1EventLoader : public Algorithm {

    public:
        // Constructors and destructors
        Timepix1EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~Timepix1EventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        void processHeader(std::string, std::string&, long long int&);

        // Member variables
        int m_eventNumber;
        std::string m_inputDirectory;
        std::vector<std::string> m_inputFilenames;
        bool m_fileOpen;
        int m_fileNumber;
        long long int m_eventTime;
        std::ifstream m_currentFile;
        std::string m_currentDevice;
        bool m_newFrame;
        std::string m_prevHeader;
    };
}
#endif // Timepix1EventLoader_H
