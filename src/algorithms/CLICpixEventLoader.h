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
#include "core/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class CLICpixEventLoader : public Algorithm {

    public:
        // Constructors and destructors
        CLICpixEventLoader(Configuration config, Clipboard* clipboard);
        ~CLICpixEventLoader() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Member variables
        int m_eventNumber;
        string m_filename;
        ifstream m_file;

        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hShutterLength;
        TH1F* hPixelsPerFrame;
    };
}
#endif // CLICpixEventLoader_H
