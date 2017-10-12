#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include <stdio.h>
#include "core/algorithm/Algorithm.h"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"

namespace corryvreckan {
    class Timepix3EventLoader : public Algorithm {

    public:
        // Constructors and destructors
        Timepix3EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~Timepix3EventLoader() {}

        // Standard algorithm functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        bool loadData(Clipboard* clipboard, Detector* detector, Pixels*, SpidrSignals*);
        void maskPixels(Detector*, std::string);

        // cofngiuration paramaters:
        bool applyTimingCut;
        double m_timingCut;
        std::string m_inputDirectory;
        int m_minNumberOfPlanes;

        // Member variables
        std::map<std::string, std::vector<std::string>> m_datafiles;
        std::map<std::string, int> m_nFiles;
        std::map<std::string, int> m_fileNumber;
        std::map<std::string, long long int> m_syncTime;
        std::map<std::string, FILE*> m_currentFile;
        std::map<std::string, bool> m_clearedHeader;
        long long int m_currentTime;

        long long int m_prevTime;
        bool m_shutterOpen;
        std::map<std::string, Pixels*> bufferedData;
        std::map<std::string, SpidrSignals*> bufferedSignals;

        double eventLength{0};
    };
}
#endif // TIMEPIX3EVENTLOADER_H
