#ifndef OnlineMonitor_H
#define OnlineMonitor_H 1

#include <iostream>
#include "core/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/GuiDisplay.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

// ROOT includes
#include <RQ_OBJECT.h>
#include "TApplication.h"
#include "TCanvas.h"
#include "TGCanvas.h"
#include "TGDockableFrame.h"
#include "TGFrame.h"
#include "TGMenu.h"
#include "TGTextEntry.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TROOT.h"
#include "TRootEmbeddedCanvas.h"
#include "TSystem.h"

namespace corryvreckan {
    class OnlineMonitor : public Algorithm {

    public:
        // Constructors and destructors
        OnlineMonitor(Configuration config, Clipboard* clipboard);
        ~OnlineMonitor() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Application to allow display persistancy
        TApplication* app;
        GuiDisplay* gui;

        void AddHisto(string, string, string style = "");
        void AddButton(string, string);

        // Member variables
        int eventNumber;
        int updateNumber;
    };
}
#endif // OnlineMonitor_H
