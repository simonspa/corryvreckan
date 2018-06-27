#ifndef OnlineMonitor_H
#define OnlineMonitor_H 1

#include <iostream>
#include "core/module/Module.hpp"
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
#include "TROOT.h"
#include "TRootEmbeddedCanvas.h"
#include "TSystem.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class OnlineMonitor : public Module {

    public:
        // Constructors and destructors
        OnlineMonitor(Configuration config, std::vector<Detector*> detectors);
        ~OnlineMonitor() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Application to allow display persistancy
        TApplication* app;
        GuiDisplay* gui;

    private:
        void AddPlots(std::string canvas_name, Matrix<std::string> canvas_plots);
        void AddHisto(std::string, std::string, std::string style = "", bool logy = false);
        void AddButton(std::string, std::string);

        // Member variables
        int eventNumber;
        int updateNumber;

        std::string canvasTitle;

        // Canvases and their plots:
        Matrix<std::string> canvas_dutplots;
        Matrix<std::string> canvas_overview;
    };
} // namespace corryvreckan
#endif // OnlineMonitor_H
