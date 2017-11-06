#ifndef Clicpix2EventLoader_H
#define Clicpix2EventLoader_H 1

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class Clicpix2EventLoader : public Algorithm {

    public:
        // Constructors and destructors
        Clicpix2EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~Clicpix2EventLoader() {}

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
        std::string m_filename;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hPixelsPerFrame;
    };
}
#endif // Clicpix2EventLoader_H