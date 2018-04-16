#ifndef ATLASpixEventLoader_H
#define ATLASpixEventLoader_H 1

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class ATLASpixEventLoader : public Module {

    public:
        // Constructors and destructors
        ATLASpixEventLoader(Configuration config, std::vector<Detector*> detectors);
        ~ATLASpixEventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        // Single histograms
        TH1F* singlePlot;

        // Member variables
        int m_eventNumber;
        unsigned long long int m_oldtoa;
        unsigned long long int m_overflowcounter;
        std::string detectorID;
        std::string m_filename;
        std::ifstream m_file;
        double m_clockFactor;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hPixelToTCal;
        TH1F* hPixelToA;
        TH1F* hPixelsPerFrame;

        // Parameters:
        std::vector<double> m_timewalkCorrectionFactors;
        std::vector<double> m_calibrationFactors;
        double m_timestampPeriod;
        std::string m_inputDirectory;
        std::string m_calibrationFile;
        double m_eventLength;
        double m_startTime;
        bool m_toaMode;
    };
} // namespace corryvreckan
#endif // ATLASpixEventLoader_H
