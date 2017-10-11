#include "OnlineMonitor.h"
#include <TVirtualPadEditor.h>

using namespace corryvreckan;
using namespace std;

OnlineMonitor::OnlineMonitor(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    updateNumber = 500;
}

void OnlineMonitor::initialise(Parameters* par) {

    // Make the local pointer to the global parameters
    parameters = par;

    // TApplication keeps the canvases persistent
    app = new TApplication("example", 0, 0);

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

    //=== Overview canvas
    AddButton("Overview", "OverviewCanvas");
    // track chi2
    AddHisto("OverviewCanvas", "/corryvreckan/BasicTracking/trackChi2");
    // reference plane map, residuals
    string reference = parameters->reference;
    string tot = "/corryvreckan/TestAlgorithm/clusterTot_" + reference;
    AddHisto("OverviewCanvas", tot);
    string hitmap = "/corryvreckan/TestAlgorithm/hitmap_" + reference;
    AddHisto("OverviewCanvas", hitmap, "colz");
    string residuals = "/corryvreckan/BasicTracking/residualsX_" + reference;
    AddHisto("OverviewCanvas", residuals);

    //=== Track canvas
    AddButton("Tracking", "TrackCanvas");
    AddHisto("TrackCanvas", "/corryvreckan/BasicTracking/trackChi2");
    AddHisto("TrackCanvas", "/corryvreckan/BasicTracking/trackAngleX");

    //=== Per detector canvases
    AddButton("HitMaps", "HitmapCanvas");
    AddButton("Residuals", "ResidualCanvas");
    AddButton("EventTimes", "EventTimeCanvas");
    AddButton("CorrelationsX", "CorrelationXCanvas");
    AddButton("CorrelationsY", "CorrelationYCanvas");
    AddButton("ChargeDistributions", "ChargeDistributionCanvas");

    // Per detector histograms
    for(auto& detector : m_detectors) {
        string detectorID = detector->name();

        string hitmap = "/corryvreckan/TestAlgorithm/hitmap_" + detectorID;
        AddHisto("HitmapCanvas", hitmap, "colz");

        string chargeHisto = "/corryvreckan/TestAlgorithm/clusterTot_" + detectorID;
        AddHisto("ChargeDistributionCanvas", chargeHisto);

        string eventTimeHisto = "/corryvreckan/TestAlgorithm/eventTimes_" + detectorID;
        AddHisto("EventTimeCanvas", eventTimeHisto);

        string correlationXHisto = "/corryvreckan/TestAlgorithm/correlationX_" + detectorID;
        AddHisto("CorrelationXCanvas", correlationXHisto);

        string correlationYHisto = "/corryvreckan/TestAlgorithm/correlationY_" + detectorID;
        AddHisto("CorrelationYCanvas", correlationYHisto);

        if(parameters->excludedFromTracking.count(detectorID) != 0)
            continue;
        string residualHisto = "/corryvreckan/BasicTracking/residualsX_" + detectorID;
        AddHisto("ResidualCanvas", residualHisto);
    }

    // Set up the main frame before drawing

    // Exit button
    string exitButton = "StopMonitoring";
    gui->buttons[exitButton] = new TGTextButton(gui->buttonMenu, exitButton.c_str());
    gui->buttonMenu->AddFrame(gui->buttons[exitButton], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    gui->buttons[exitButton]->Connect("Pressed()", "GuiDisplay", gui, "Exit()");

    // Main frame resizing
    gui->m_mainFrame->AddFrame(gui->buttonMenu, new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    gui->m_mainFrame->SetWindowName("CLIC Tesbeam Monitoring");
    gui->m_mainFrame->MapSubwindows();
    gui->m_mainFrame->Resize(gui->m_mainFrame->GetDefaultSize());

    // Draw the main frame
    gui->m_mainFrame->MapWindow();

    // Plot the overview tab (if it exists)
    if(gui->histograms["OverviewCanvas"].size() != 0) {
        gui->Display("OverviewCanvas");
    }

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
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL)
        return Success;

    // Otherwise increase the event number
    eventNumber++;
    return Success;
}

void OnlineMonitor::finalise() {

    LOG(DEBUG) << "Analysed " << eventNumber << " events";
}

void OnlineMonitor::AddHisto(string canvasName, string histoName, string style) {

    TH1* histogram = (TH1*)gDirectory->Get(histoName.c_str());
    if(histogram) {
        gui->histograms[canvasName].push_back((TH1*)gDirectory->Get(histoName.c_str()));
        gui->styles[gui->histograms[canvasName].back()] = style;
    } else {
        LOG(WARNING) << "Histogram " << histoName << " does not exist";
    }
}

void OnlineMonitor::AddButton(string buttonName, string canvasName) {
    gui->buttons[buttonName] = new TGTextButton(gui->buttonMenu, buttonName.c_str());
    gui->buttonMenu->AddFrame(gui->buttons[buttonName], new TGLayoutHints(kLHintsLeft, 10, 10, 10, 10));
    string command = "Display(=\"" + canvasName + "\")";
    LOG(INFO) << "Connecting button with command " << command.c_str();
    gui->buttons[buttonName]->Connect("Pressed()", "GuiDisplay", gui, command.c_str());
}