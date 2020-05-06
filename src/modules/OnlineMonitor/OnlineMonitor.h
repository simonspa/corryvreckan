/**
 * @file
 * @brief Definition of module OnlineMonitor
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef OnlineMonitor_H
#define OnlineMonitor_H 1

#include <RQ_OBJECT.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TGCanvas.h>
#include <TGDockableFrame.h>
#include <TGFrame.h>
#include <TGMenu.h>
#include <TGTextEntry.h>
#include <TH1F.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TSystem.h>
#include <iostream>

#include "GuiDisplay.hpp"
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class OnlineMonitor : public Module {

    public:
        // Constructors and destructors
        OnlineMonitor(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~OnlineMonitor() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

        // Application to allow display persistancy
        TApplication* app;
        GuiDisplay* gui;

    private:
        void AddCanvasGroup(std::string group_title);
        void AddCanvas(std::string canvas_title,
                       std::string canvasGroup,
                       Matrix<std::string> canvas_plots,
                       std::string detector_name = "");
        void AddPlots(std::string canvas_name, Matrix<std::string> canvas_plots, std::string detector_name = "");
        void AddHisto(std::string, std::string, std::string style = "", bool logy = false);

        // Member variables
        int eventNumber;
        int updateNumber;
        bool ignoreAux;

        std::string canvasTitle;

        std::string clusteringModule;
        std::string trackingModule;

        // Canvases and their plots:
        Matrix<std::string> canvas_dutplots, canvas_overview, canvas_tracking, canvas_hitmaps, canvas_residuals, canvas_cx,
            canvas_cy, canvas_cx2d, canvas_cy2d, canvas_charge, canvas_time;
    };
} // namespace corryvreckan
#endif // OnlineMonitor_H
