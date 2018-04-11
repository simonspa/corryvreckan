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

#include "clicpix2_frameDecoder.hpp"
#include "clicpix2_pixels.hpp"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class Clicpix2EventLoader : public Algorithm {

    public:
        // Constructors and destructors
        Clicpix2EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~Clicpix2EventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix_config;

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        // Single histograms
        TH1F* singlePlot;

        bool comp, sp_comp;
        caribou::clicpix2_frameDecoder* decoder;

        // Member variables
        int m_eventNumber;
        std::string m_filename;
        std::string m_matrix;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hPixelToA;
        TH1F* hPixelCnt;
        TH1F* hPixelsPerFrame;
    };
} // namespace corryvreckan
#endif // Clicpix2EventLoader_H
