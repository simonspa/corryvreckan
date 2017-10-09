#include "GenericAlgorithm.h"

using namespace corryvreckan;

GenericAlgorithm::GenericAlgorithm(Configuration config, Clipboard* clipboard) : Algorithm(std::move(config), clipboard) {}

void GenericAlgorithm::initialise(Parameters* par) {

    parameters = par;

    // Initialise histograms per device
    for(int det = 0; det < parameters->nDetectors; det++) {

        // Check if they are a Timepix3
        string detectorID = parameters->detectors[det];
        if(parameters->detector[detectorID]->type() != "Timepix3")
            continue;

        // Simple histogram per device
        string name = "plotForDevice_" + detectorID;
        plotPerDevice[detectorID] = new TH2F(name.c_str(), name.c_str(), 256, 0, 256, 256, 0, 256);
    }

    // Initialise single histograms
    string name = "singlePlot";
    singlePlot = new TH1F(name.c_str(), name.c_str(), 1000, 0, 1000);

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode GenericAlgorithm::run(Clipboard* clipboard) {

    // Loop over all Timepix3 and make plots
    for(int det = 0; det < parameters->nDetectors; det++) {

        // Check if they are a Timepix3
        string detectorID = parameters->detectors[det];
        if(parameters->detector[detectorID]->type() != "Timepix3")
            continue;

        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detectorID, "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any pixels on the clipboard";
            continue;
        }

        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
            continue;
        }

        // Loop over all pixels and make hitmaps
        for(int iP = 0; iP < pixels->size(); iP++) {

            // Get the pixel
            Pixel* pixel = (*pixels)[iP];

            // Fill the plots for this device
            plotPerDevice[detectorID]->Fill(pixel->m_column, pixel->m_row);
        }
    }

    // Fill single histogram
    singlePlot->Fill(m_eventNumber);

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void GenericAlgorithm::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
