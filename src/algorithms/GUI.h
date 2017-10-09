#ifndef GUI_H
#define GUI_H 1

#include <iostream>
#include "TApplication.h"
#include "TBrowser.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TThread.h"
#include "core/Algorithm.h"

#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class GUI : public Algorithm {

    public:
        // Constructors and destructors
        GUI(Configuration config, Clipboard* clipboard);
        ~GUI() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Plot holders
        vector<TCanvas*> canvases;
        map<TCanvas*, vector<TH1*>> histograms;
        map<TH1*, string> styles;

        // Add plots and canvases
        void addPlot(TCanvas*, string, string style = "");
        void addCanvas(TCanvas*);

        // Application to allow display of canvases
        TApplication* app;

        // Misc. member objects
        int nDetectors;
        int eventNumber;
        int updateNumber;
    };
}
#endif // GUI_H
