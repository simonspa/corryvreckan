#ifndef CLICpixEventLoader_H
#define CLICpixEventLoader_H 1

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
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
    class CLICpixEventLoader : public Module {

    public:
        // Constructors and destructors
        CLICpixEventLoader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~CLICpixEventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

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
#endif // CLICpixEventLoader_H
