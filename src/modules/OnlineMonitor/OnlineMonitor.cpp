/**
 * @file
 * @brief Implementation of module OnlineMonitor
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "OnlineMonitor.h"
#include <TGButtonGroup.h>
#include <TVirtualPadEditor.h>
#include <regex>

using namespace corryvreckan;
using namespace std;

OnlineMonitor::OnlineMonitor(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    config_.setDefault<std::string>("canvas_title", "Corryvreckan Testbeam Monitor");
    canvasTitle = config_.get<std::string>("canvas_title");

    config_.setDefault<int>("update", 200);
    updateNumber = config_.get<int>("update");

    config_.setDefault<bool>("ignore_aux", true);
    ignoreAux = config_.get<bool>("ignore_aux");

    config_.setDefault<std::string>("clustering_module", "Clustering4D");
    clusteringModule = config_.get<std::string>("clustering_module");

    config_.setDefault<std::string>("tracking_module", "Tracking4D");
    trackingModule = config_.get<std::string>("tracking_module");

    // Set up overview plots:
    // config_.setDefaultMatrix...
    canvas_overview = config_.getMatrix<std::string>("overview",
                                                     {{trackingModule + "/trackChi2"},
                                                      {clusteringModule + "/%REFERENCE%/clusterCharge"},
                                                      {"Correlations/%REFERENCE%/hitmap", "colz"},
                                                      {trackingModule + "/%REFERENCE%/residualsX"}});

    // Set up individual plots for the DUT
    canvas_dutplots = config_.getMatrix<std::string>("dut_plots",
                                                     {{"EventLoaderEUDAQ2/%DUT%/hitmap", "colz"},
                                                      {"EventLoaderEUDAQ2/%DUT%/hPixelTimes"},
                                                      {"EventLoaderEUDAQ2/%DUT%/hPixelRawValues"},
                                                      {"EventLoaderEUDAQ2/%DUT%/pixelMultiplicity", "log"},
                                                      {"AnalysisDUT/%DUT%/clusterChargeAssociated"},
                                                      {"AnalysisDUT/%DUT%/associatedTracksVersusTime"}});
    canvas_tracking = config_.getMatrix<std::string>("tracking",
                                                     {{trackingModule + "/trackChi2"},
                                                      {trackingModule + "/trackAngleX"},
                                                      {trackingModule + "/trackAngleY"},
                                                      {trackingModule + "/trackChi2ndof"},
                                                      {trackingModule + "/tracksPerEvent"},
                                                      {trackingModule + "/clustersPerTrack"}});
    canvas_hitmaps = config_.getMatrix<std::string>("hitmaps", {{"Correlations/%DETECTOR%/hitmap", "colz"}});
    canvas_residuals = config_.getMatrix<std::string>("residuals", {{trackingModule + "/%DETECTOR%/residualsX"}});

    canvas_cx = config_.getMatrix<std::string>("correlation_x", {{"Correlations/%DETECTOR%/correlationX"}});
    canvas_cx2d =
        config_.getMatrix<std::string>("correlation_x2d", {{"Correlations/%DETECTOR%/correlationX_2Dlocal", "colz"}});
    canvas_cy = config_.getMatrix<std::string>("correlation_y", {{"Correlations/%DETECTOR%/correlationY"}});
    canvas_cy2d =
        config_.getMatrix<std::string>("correlation_y2d", {{"Correlations/%DETECTOR%/correlationY_2Dlocal", "colz"}});

    canvas_charge =
        config_.getMatrix<std::string>("charge_distributions", {{clusteringModule + "/%DETECTOR%/clusterCharge"}});

    canvas_time = config_.getMatrix<std::string>("event_times", {{"Correlations/%DETECTOR%/eventTimes"}});
}

void OnlineMonitor::initialise() {

    // TApplication keeps the canvases persistent
    app = new TApplication("example", nullptr, nullptr);

    // Make the GUI
    gui = new GuiDisplay();

    // Make the main window object and set the attributes
    gui->m_mainFrame = new TGMainFrame(gClient->GetRoot(), 1200, 600);
    gui->buttonMenu = new TGHorizontalFrame(gui->m_mainFrame, 1200, 50);
    gui->canvas = new TRootEmbeddedCanvas("canvas", gui->m_mainFrame, 1200, 600);
    gui->m_mainFrame->AddFrame(gui->canvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));
    gui->m_mainFrame->SetCleanup(kDeepCleanup);
    gui->m_mainFrame->DontCallClose();

    // Add canvases and histograms
    AddCanvasGroup("Tracking");
    AddCanvas("Overview", "Tracking", canvas_overview);
    AddCanvas("Tracking Performance", "Tracking", canvas_tracking);
    AddCanvas("Residuals", "Tracking", canvas_residuals);

    AddCanvasGroup("Detectors");
    AddCanvas("Hitmaps", "Detectors", canvas_hitmaps);
    AddCanvas("Event Times", "Detectors", canvas_time);
    AddCanvas("Charge Distributions", "Detectors", canvas_charge);

    AddCanvasGroup("Correlations 1D");
    AddCanvas("1D X", "Correlations 1D", canvas_cx);
    AddCanvas("1D Y", "Correlations 1D", canvas_cy);

    AddCanvasGroup("Correlations 2D");
    AddCanvas("2D X", "Correlations 2D", canvas_cx2d);
    AddCanvas("2D Y", "Correlations 2D", canvas_cy2d);

    AddCanvasGroup("DUTs");
    for(auto& detector : get_detectors()) {
        if(detector->isDUT()) {
            AddCanvas(detector->getName(), "DUTs", canvas_dutplots, detector->getName());
        }
    }

    // Set up the main frame before drawing
    AddCanvasGroup("Controls");
    ULong_t color;

    // Pause button
    gClient->GetColorByName("green", color);
    gui->buttons["pause"] = new TGTextButton(gui->buttonGroups["Controls"], "   &Pause Monitoring   ");
    gui->buttons["pause"]->ChangeBackground(color);
    gui->buttons["pause"]->Connect("Pressed()", "corryvreckan::GuiDisplay", gui, "TogglePause()");
    gui->buttonGroups["Controls"]->AddFrame(gui->buttons["pause"], new TGLayoutHints(kLHintsTop | kLHintsExpandX));

    // Exit button
    gClient->GetColorByName("yellow", color);
    gui->buttons["exit"] = new TGTextButton(gui->buttonGroups["Controls"], "&Exit Monitor");
    gui->buttons["exit"]->ChangeBackground(color);
    gui->buttons["exit"]->Connect("Pressed()", "corryvreckan::GuiDisplay", gui, "Exit()");
    gui->buttonGroups["Controls"]->AddFrame(gui->buttons["exit"], new TGLayoutHints(kLHintsTop | kLHintsExpandX));

    // Main frame resizing
    gui->m_mainFrame->AddFrame(gui->buttonMenu, new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    gui->m_mainFrame->SetWindowName(canvasTitle.c_str());
    gui->m_mainFrame->MapSubwindows();
    gui->m_mainFrame->Resize(gui->m_mainFrame->GetDefaultSize());

    // Draw the main frame
    gui->m_mainFrame->MapWindow();

    // Plot the overview tab (if it exists)
    if(gui->histograms["OverviewCanvas"].size() != 0) {
        gui->Display(const_cast<char*>(std::string("OverviewCanvas").c_str()));
    }

    gui->canvas->GetCanvas()->Paint();
    gui->canvas->GetCanvas()->Update();
    gSystem->ProcessEvents();

    // Initialise member variables
    eventNumber = 0;
}

StatusCode OnlineMonitor::run(std::shared_ptr<Clipboard>) {

    if(!gui->isPaused()) {
        // Draw all histograms
        if(eventNumber % updateNumber == 0) {
            gui->Update();
        }
    }
    gSystem->ProcessEvents();

    // Increase the event number
    eventNumber++;
    return StatusCode::Success;
}

void OnlineMonitor::AddCanvasGroup(std::string group_title) {
    gui->buttonGroups[group_title] = new TGVButtonGroup(gui->buttonMenu, group_title.c_str());
    gui->buttonMenu->AddFrame(gui->buttonGroups[group_title], new TGLayoutHints(kLHintsLeft | kLHintsTop, 10, 10, 10, 10));
    gui->buttonGroups[group_title]->Show();
}

void OnlineMonitor::AddCanvas(std::string canvas_title,
                              std::string canvasGroup,
                              Matrix<std::string> canvas_plots,
                              std::string detector_name) {
    std::string canvas_name = canvas_title + "Canvas";

    if(canvasGroup.empty()) {
        gui->buttons[canvas_title] = new TGTextButton(gui->buttonMenu, canvas_title.c_str());
        gui->buttonMenu->AddFrame(gui->buttons[canvas_title], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    } else {
        gui->buttons[canvas_title] = new TGTextButton(gui->buttonGroups[canvasGroup], canvas_title.c_str());
        gui->buttonGroups[canvasGroup]->AddFrame(gui->buttons[canvas_title],
                                                 new TGLayoutHints(kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
    }

    string command = "Display(=\"" + canvas_name + "\")";
    LOG(INFO) << "Connecting button with command " << command.c_str();
    gui->buttons[canvas_title]->Connect("Pressed()", "corryvreckan::GuiDisplay", gui, command.c_str());

    AddPlots(canvas_name, canvas_plots, detector_name);
}

void OnlineMonitor::AddPlots(std::string canvas_name, Matrix<std::string> canvas_plots, std::string detector_name) {
    for(auto plot : canvas_plots) {

        // Add default plotting style if not set:
        plot.resize(2, "");

        // Do we need to plot with a LogY scale?
        bool log_scale = (plot.back().find("log") != std::string::npos) ? true : false;

        // Replace reference placeholders and add histogram
        std::string name = std::regex_replace(plot.front(), std::regex("%REFERENCE%"), get_reference()->getName());

        // Parse other placeholders:
        if(name.find("%DUT%") != std::string::npos) {
            // Do we have a DUT placeholder?
            if(!detector_name.empty()) {
                LOG(DEBUG) << "Adding plot " << name << " for detector " << detector_name;
                auto detector = get_detector(detector_name);
                AddHisto(
                    canvas_name, std::regex_replace(name, std::regex("%DUT%"), detector->getName()), plot.back(), log_scale);

            } else {
                LOG(DEBUG) << "Adding plot " << name << " for all DUTs.";
                for(auto& detector : get_detectors()) {
                    if(!detector->isDUT()) {
                        continue;
                    }
                    AddHisto(canvas_name,
                             std::regex_replace(name, std::regex("%DUT%"), detector->getName()),
                             plot.back(),
                             log_scale);
                }
            }
        } else if(name.find("%DETECTOR%") != std::string::npos) {
            // Do we have a detector placeholder?
            if(!detector_name.empty()) {
                LOG(DEBUG) << "Adding plot " << name << " for detector " << detector_name;
                auto detector = get_detector(detector_name);
                AddHisto(canvas_name,
                         std::regex_replace(name, std::regex("%DETECTOR%"), detector->getName()),
                         plot.back(),
                         log_scale);
            } else {
                LOG(DEBUG) << "Adding plot " << name << " for all detectors.";
                for(auto& detector : get_detectors()) {
                    // Ignore AUX detectors
                    if(ignoreAux && detector->isAuxiliary()) {
                        continue;
                    }

                    AddHisto(canvas_name,
                             std::regex_replace(name, std::regex("%DETECTOR%"), detector->getName()),
                             plot.back(),
                             log_scale);
                }
            }
        } else {
            // Single histogram only.
            AddHisto(canvas_name, name, plot.back(), log_scale);
        }
    }
}

void OnlineMonitor::AddHisto(string canvasName, string histoName, string style, bool logy) {

    // Add root directory to path:
    histoName = "/" + histoName;

    TH1* histogram = static_cast<TH1*>(gDirectory->Get(histoName.c_str()));
    if(histogram) {
        gui->histograms[canvasName].push_back(static_cast<TH1*>(gDirectory->Get(histoName.c_str())));
        gui->logarithmic[gui->histograms[canvasName].back()] = logy;
        gui->styles[gui->histograms[canvasName].back()] = style;
    } else {
        LOG(WARNING) << "Histogram " << histoName << " does not exist";
    }
}
