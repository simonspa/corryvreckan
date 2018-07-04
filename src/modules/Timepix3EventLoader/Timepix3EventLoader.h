#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include <stdio.h>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Timepix3EventLoader : public Module {

    public:
        // Constructors and destructors
        Timepix3EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~Timepix3EventLoader() {}

        // Standard algorithm functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // ROOT graphs
        TH1F* pixelToT_beforecalibration;
        TH1F* pixelToT_aftercalibration;
        TH2F* pixelTOTParameterA;
        TH2F* pixelTOTParameterB;
        TH2F* pixelTOTParameterC;
        TH2F* pixelTOTParameterT;
        TH2F* pixelTOAParameterC;
        TH2F* pixelTOAParameterD;
        TH2F* pixelTOAParameterT;
        TH1F* timeshiftPlot;

    private:
        bool loadData(Clipboard* clipboard, Detector* detector, Pixels*, SpidrSignals*);
        void loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat);
        void maskPixels(Detector*, std::string);

        // configuration paramaters:
        double m_triggerLatency;
        std::string m_inputDirectory;
        int m_minNumberOfPlanes;

        bool temporalSplit;
        int m_numberPixelHits;

        bool applyCalibration;
        std::string calibrationPath;
        std::string threshold;

        std::vector<std::vector<float>> vtot;
        std::vector<std::vector<float>> vtoa;

        // Member variables
        std::map<std::string, std::vector<std::unique_ptr<std::ifstream>>> m_files;
        std::map<std::string, std::vector<std::unique_ptr<std::ifstream>>::iterator> m_file_iterator;

        std::map<std::string, long long int> m_syncTime;
        std::map<std::string, bool> m_clearedHeader;
        std::map<std::string, long long int> m_syncTimeTDC;
        std::map<std::string, int> m_TDCoverflowCounter;

        long long int m_currentEvent;

        long long int m_prevTime;
        bool m_shutterOpen;
        std::map<std::string, Pixels*> bufferedData;
        std::map<std::string, SpidrSignals*> bufferedSignals;
    };
} // namespace corryvreckan
#endif // TIMEPIX3EVENTLOADER_H
