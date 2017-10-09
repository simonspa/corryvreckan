#include "GUI.h"
#include "TApplication.h"
#include "TPolyLine3D.h"
#include "TROOT.h"
#include "TSystem.h"

using namespace corryvreckan;
using namespace std;

GUI::GUI(Configuration config, Clipboard* clipboard) : Algorithm(std::move(config), clipboard) {
    // Update every X events:
    updateNumber = m_config.get<int>("updateNumber",500);
}

void GUI::initialise(Parameters* par) {

    // Make the local pointer to the global parameters
    parameters = par;

    // Check the number of devices
    nDetectors = parameters->nDetectors;

    // TApplication keeps the canvases persistent
    app = new TApplication("example", 0, 0);

    //========= Add each canvas that is wanted =========//

    TCanvas* trackCanvas = new TCanvas("TrackCanvas", "Track canvas");
    addCanvas(trackCanvas);

    TCanvas* hitmapCanvas = new TCanvas("HitMapCanvas", "Hit map canvas");
    addCanvas(hitmapCanvas);

    TCanvas* residualsCanvas = new TCanvas("ResidualsCanvas", "Residuals canvas");
    addCanvas(residualsCanvas);

    TCanvas* chargeCanvas = new TCanvas("chargeCanvas", "Charge deposit canvas");
    addCanvas(chargeCanvas);

    //========= Add each histogram =========//

    // Individual plots
    addPlot(trackCanvas, "/corryvreckan/BasicTracking/trackChi2");
    addPlot(trackCanvas, "/corryvreckan/BasicTracking/trackAngleX");

    // Per detector histograms
    for(auto detectorID : parameters->detectors) {
        string hitmap = "/corryvreckan/TestAlgorithm/hitmap_" + detectorID;
        addPlot(hitmapCanvas, hitmap, "colz");

        string residualHisto = "/corryvreckan/BasicTracking/residualsX_" + detectorID;
        if(detectorID == parameters->DUT)
            residualHisto = "/corryvreckan/DUTAnalysis/residualsX";
        addPlot(residualsCanvas, residualHisto);

        string chargeHisto = "/corryvreckan/TestAlgorithm/clusterTot_" + detectorID;
        addPlot(chargeCanvas, chargeHisto);
    }

    // Divide the canvases if needed
    for(auto& canvas : canvases) {
        int nHistograms = histograms[canvas].size();
        if(nHistograms == 1)
            continue;
        if(nHistograms < 4)
            canvas->Divide(nHistograms);
        else
            canvas->Divide(ceil(nHistograms / 2.), 2);
    }

    // Draw all histograms
    for(auto& canvas : canvases) {
        canvas->cd();
        vector<TH1*> histos = histograms[canvas];
        int nHistograms = histos.size();
        for(int iHisto = 0; iHisto < nHistograms; iHisto++) {
            canvas->cd(iHisto + 1);
            string style = styles[histos[iHisto]];
            if(histos[iHisto] != nullptr) histos[iHisto]->Draw(style.c_str());
        }
    }

    // Set event counter
    eventNumber = 1;
}

StatusCode GUI::run(Clipboard* clipboard) {

    // Draw all histograms
    if(eventNumber % updateNumber == 0) {
        for(int iCanvas = 0; iCanvas < canvases.size(); iCanvas++) {
            canvases[iCanvas]->Paint();
            canvases[iCanvas]->Update();
        }
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

void GUI::finalise() {}

void GUI::addCanvas(TCanvas* canvas) {
    canvases.push_back(canvas);
}

void GUI::addPlot(TCanvas* canvas, string histoName, std::string style) {
    TH1* histogram = (TH1*)gDirectory->Get(histoName.c_str());
    histograms[canvas].push_back(histogram);
    styles[histogram] = style;
}
