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

        bool temporalSplit;
        double m_eventLength;
        int m_numberPixelHits;

        // Member variables
        std::map<std::string, std::vector<std::unique_ptr<std::ifstream>>> m_files;
        std::map<std::string, std::vector<std::unique_ptr<std::ifstream>>::iterator> m_file_iterator;

        std::map<std::string, long long int> m_syncTime;
        std::map<std::string, bool> m_clearedHeader;
        long long int m_currentTime;
        long long int m_currentEvent;

        long long int m_prevTime;
        bool m_shutterOpen;
        std::map<std::string, Pixels*> bufferedData;
        std::map<std::string, SpidrSignals*> bufferedSignals;
    };
} // namespace corryvreckan
#endif // TIMEPIX3EVENTLOADER_H
