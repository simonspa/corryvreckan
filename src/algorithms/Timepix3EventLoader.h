#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include <stdio.h>
#include "core/Algorithm.h"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"

namespace corryvreckan {
    class Timepix3EventLoader : public Algorithm {

    public:
        // Constructors and destructors
        Timepix3EventLoader(Configuration config, Clipboard* clipboard);
        ~Timepix3EventLoader() {}

        // Standard algorithm functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Extra functions
        bool loadData(std::string, Pixels*, SpidrSignals*);
        void maskPixels(std::string, std::string);

        // Member variables
        std::string m_inputDirectory;
        std::map<std::string, std::vector<std::string>> m_datafiles;
        std::map<std::string, int> m_nFiles;
        std::map<std::string, int> m_fileNumber;
        std::map<std::string, long long int> m_syncTime;
        std::map<std::string, FILE*> m_currentFile;
        std::map<std::string, bool> m_clearedHeader;
        int m_minNumberOfPlanes;
        bool applyTimingCut;
        long long int m_currentTime;
        double m_timingCut;
        long long int m_prevTime;
        bool m_shutterOpen;
        std::map<std::string, Pixels*> bufferedData;
        std::map<std::string, SpidrSignals*> bufferedSignals;

        double eventLength{0};
    };
}
#endif // TIMEPIX3EVENTLOADER_H
