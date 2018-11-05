#include "OnlineMonitor.h"
#include <TVirtualPadEditor.h>
#include <regex>

using namespace corryvreckan;
using namespace std;

OnlineMonitor::OnlineMonitor(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    canvasTitle = m_config.get<std::string>("canvasTitle", "Corryvreckan Testbeam Monitor");
    updateNumber = m_config.get<int>("update", 500);

    // Set up overview plots:
    canvas_overview = m_config.getMatrix<std::string>("Overview",
                                                      {{"BasicTracking/trackChi2"},
                                                       {"TestAlgorithm/clusterTot_%REFERENCE%"},
                                                       {"TestAlgorithm/hitmap_%REFERENCE%", "colz"},
                                                       {"BasicTracking/residualsX_%REFERENCE%"}});

    // Set up individual plots for the DUT
    canvas_dutplots = m_config.getMatrix<std::string>("DUTPlots",
                                                      {{"Clicpix2EventLoader/hitMap", "colz"},
                                                       {"Clicpix2EventLoader/hitMapDiscarded", "colz"},
                                                       {"Clicpix2EventLoader/pixelToT"},
                                                       {"Clicpix2EventLoader/pixelToA"},
                                                       {"Clicpix2EventLoader/pixelCnt", "log"},
                                                       {"Clicpix2EventLoader/pixelsPerFrame", "log"},
                                                       {"DUTAnalysis/clusterTotAssociated"},
                                                       {"DUTAnalysis/associatedTracksVersusTime"}});
    canvas_tracking =
        m_config.getMatrix<std::string>("Tracking", {{"BasicTracking/trackChi2"}, {"BasicTracking/trackAngleX"}});
    canvas_hitmaps = m_config.getMatrix<std::string>("HitMaps", {{"TestAlgorithm/hitmap_%DETECTOR%", "colz"}});
    canvas_residuals = m_config.getMatrix<std::string>("Residuals", {{"BasicTracking/residualsX_%DETECTOR%"}});

    canvas_cx = m_config.getMatrix<std::string>("CorrelationX", {{"TestAlgorithm/correlationX_%DETECTOR%"}});
    canvas_cx2d =
        m_config.getMatrix<std::string>("CorrelationX2D", {{"TestAlgorithm/correlationX_2Dlocal_%DETECTOR%", "colz"}});
    canvas_cy = m_config.getMatrix<std::string>("CorrelationY", {{"TestAlgorithm/correlationY_%DETECTOR%"}});
    canvas_cy2d =
        m_config.getMatrix<std::string>("CorrelationY2D", {{"TestAlgorithm/correlationY_2Dlocal_%DETECTOR%", "colz"}});

    canvas_charge = m_config.getMatrix<std::string>("ChargeDistributions", {{"Timepix3Clustering/clusterTot_%DETECTOR%"}});

    canvas_time = m_config.getMatrix<std::string>("EventTimes", {{"TestAlgorithm/eventTimes_%DETECTOR%"}});
}

void OnlineMonitor::initialise() {

    // TApplication keeps the canvases persistent
    app = new TApplication("example", nullptr, nullptr);

    // Make the GUI
    gui = new GuiDisplay();

    // Make the main window object and set the attributes
    gui->m_mainFrame = new TGMainFrame(gClient->GetRoot(), 800, 600);
    gui->buttonMenu = new TGHorizontalFrame(gui->m_mainFrame, 800, 50);
    gui->canvas = new TRootEmbeddedCanvas("canvas", gui->m_mainFrame, 800, 600);
    gui->m_mainFrame->AddFrame(gui->canvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));
    gui->m_mainFrame->SetCleanup(kDeepCleanup);
    gui->m_mainFrame->DontCallClose();

    // Add canvases and histograms
    AddCanvas("Overview", canvas_overview);
    AddCanvas("Tracking", canvas_tracking);
    AddCanvas("HitMaps", canvas_hitmaps);
    AddCanvas("Residuals", canvas_residuals);
    AddCanvas("EventTimes", canvas_time);
    AddCanvas("ChargeDistributions", canvas_charge);

    AddCanvas("CorrelationsX", canvas_cx);
    AddCanvas("CorrelationsY", canvas_cy);
    AddCanvas("CorrelationsX2D", canvas_cx2d);
    AddCanvas("CorrelationsY2D", canvas_cy2d);

    AddCanvas("DUTPlots", canvas_dutplots);

    // Set up the main frame before drawing
    // Exit button
    string exitButton = "StopMonitoring";
    gui->buttons[exitButton] = new TGTextButton(gui->buttonMenu, exitButton.c_str());
    gui->buttonMenu->AddFrame(gui->buttons[exitButton], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    gui->buttons[exitButton]->Connect("Pressed()", "corryvreckan::GuiDisplay", gui, "Exit()");

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

StatusCode OnlineMonitor::run(Clipboard* clipboard) {

    // Draw all histograms
    if(eventNumber % updateNumber == 0) {
        gui->canvas->GetCanvas()->Paint();
        gui->canvas->GetCanvas()->Update();
        eventNumber++;
    }
    gSystem->ProcessEvents();

    // Get the tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        return Success;
    }

    // Otherwise increase the event number
    eventNumber++;
    return Success;
}

void OnlineMonitor::finalise() {

    LOG(DEBUG) << "Analysed " << eventNumber << " events";
}

void OnlineMonitor::AddCanvas(std::string canvas_title, Matrix<std::string> canvas_plots) {
    std::string canvas_name = canvas_title + "Canvas";

    gui->buttons[canvas_title] = new TGTextButton(gui->buttonMenu, canvas_title.c_str());
    gui->buttonMenu->AddFrame(gui->buttons[canvas_title], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    string command = "Display(=\"" + canvas_name + "\")";
    LOG(INFO) << "Connecting button with command " << command.c_str();
    gui->buttons[canvas_title]->Connect("Pressed()", "corryvreckan::GuiDisplay", gui, command.c_str());

    AddPlots(canvas_name, canvas_plots);
}

void OnlineMonitor::AddPlots(std::string canvas_name, Matrix<std::string> canvas_plots) {
    for(auto plot : canvas_plots) {

        // Add default plotting style if not set:
        plot.resize(2, "");

        // Do we need to plot with a LogY scale?
        bool log_scale = (plot.back().find("log") != std::string::npos) ? true : false;

        // Replace other placeholders and add histogram
        std::string name = std::regex_replace(plot.front(), std::regex("%DUT%"), get_dut()->name());
        name = std::regex_replace(name, std::regex("%REFERENCE%"), get_reference()->name());

        // Do we have a detector placeholder?
        if(name.find("%DETECTOR%") != std::string::npos) {
            LOG(DEBUG) << "Adding plot " << name << " for all detectors.";
            for(auto& detector : get_detectors()) {
                AddHisto(canvas_name,
                         std::regex_replace(name, std::regex("%DETECTOR%"), detector->name()),
                         plot.back(),
                         log_scale);
            }
        } else {
            // Single histogram only.
            AddHisto(canvas_name, name, plot.back(), log_scale);
        }
    }
}

void OnlineMonitor::AddHisto(string canvasName, string histoName, string style, bool logy) {

    // Add "corryvreckan" namespace:
    histoName = "/corryvreckan/" + histoName;

    TH1* histogram = static_cast<TH1*>(gDirectory->Get(histoName.c_str()));
    if(histogram) {
        gui->histograms[canvasName].push_back(static_cast<TH1*>(gDirectory->Get(histoName.c_str())));
        gui->styles[gui->histograms[canvasName].back()] = style;
        gui->logarithmic[gui->histograms[canvasName].back()] = logy;
    } else {
        LOG(WARNING) << "Histogram " << histoName << " does not exist";
    }
}
