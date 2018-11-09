#ifndef EventLoaderCLICpix_H
#define EventLoaderCLICpix_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderCLICpix : public Module {

    public:
        // Constructors and destructors
        EventLoaderCLICpix(Configuration config, std::shared_ptr<Detector> detector);
        ~EventLoaderCLICpix() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;
        // Member variables
        int m_eventNumber;
        std::string m_filename;
        std::ifstream m_file;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hShutterLength;
        TH1F* hPixelsPerFrame;
    };
} // namespace corryvreckan
#endif // EventLoaderCLICpix_H
