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
#include "TProfile2D.h"

#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

#include "CLICpix2/clicpix2_pixels.hpp"
#include "CLICpix2/framedecoder/clicpix2_frameDecoder.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Clicpix2EventLoader : public Module {

    public:
        // Constructors and destructors
        Clicpix2EventLoader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Clicpix2EventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix_config;

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        bool comp, sp_comp;
        caribou::clicpix2_frameDecoder* decoder;

        // Member variables
        int m_eventNumber;
        std::string m_filename;
        std::string m_matrix;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH2F* hMaskMap;
        TH2F* hHitMapDiscarded;
        TProfile2D* hPixelToTMap;
        TH1F* hPixelToT;
        TH1F* hPixelToA;
        TH1F* hPixelCnt;
        TH1F* hPixelsPerFrame;

        bool discardZeroToT;
    };
} // namespace corryvreckan
#endif // Clicpix2EventLoader_H
