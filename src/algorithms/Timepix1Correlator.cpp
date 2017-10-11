#include "Timepix1Correlator.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

using namespace corryvreckan;
using namespace std;

Timepix1Correlator::Timepix1Correlator(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {}

void Timepix1Correlator::initialise(Parameters* par) {

    parameters = par;

    // Initialise histograms per device
    for(auto& detector : m_detectors) {

        // Check if they are a Timepix3
        string detectorID = detector->name();
        if(detector->type() != "Timepix1")
            continue;

        // Simple histogram per device
        string name = "correlationsX_" + detectorID;
        correlationPlotsX[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -3, 3);

        name = "correlationsY_" + detectorID;
        correlationPlotsY[detectorID] = new TH1F(name.c_str(), name.c_str(), 500, -3, 3);

        int nPixelsRow = detector->nPixelsY();
        int nPixelsCol = detector->nPixelsX();
        name = "hitmaps_" + detectorID;
        hitmaps[detectorID] = new TH2F(name.c_str(), name.c_str(), nPixelsCol, 0, nPixelsCol, nPixelsRow, 0, nPixelsRow);

        name = "hitmapsGlobal_" + detectorID;
        hitmapsGlobal[detectorID] = new TH2F(name.c_str(), name.c_str(), 200, -10., 10., 200, -10., 10.);

        name = "clusterSize_" + detectorID;
        clusterSize[detectorID] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);

        name = "clustersPerEvent_" + detectorID;
        clustersPerEvent[detectorID] = new TH1F(name.c_str(), name.c_str(), 200, 0, 200);
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode Timepix1Correlator::run(Clipboard* clipboard) {

    // Get the clusters for the reference detector
    string referenceDetector = parameters->reference;
    Clusters* referenceClusters = (Clusters*)clipboard->get(referenceDetector, "clusters");
    if(referenceClusters == NULL) {
        LOG(DEBUG) << "Detector " << referenceDetector << " does not have any clusters on the clipboard";
        return Success;
    }

    // Loop over all Timepix1 and make plots
    for(auto& detector : m_detectors) {

        // Check if they are a Timepix1
        string detectorID = detector->name();
        if(detector->type() != "Timepix1")
            continue;

        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
            continue;
        }

        // Loop over all clusters and make correlations
        for(auto& cluster : (*clusters)) {

            for(auto& refCluster : (*referenceClusters)) {
                // Fill the plots for this device
                if(fabs(cluster->globalY() - refCluster->globalY()) < 1.)
                    correlationPlotsX[detectorID]->Fill(cluster->globalX() - refCluster->globalX());
                if(fabs(cluster->globalX() - refCluster->globalX()) < 1.)
                    correlationPlotsY[detectorID]->Fill(cluster->globalY() - refCluster->globalY());
            }

            hitmaps[detectorID]->Fill(cluster->column(), cluster->row());
            hitmapsGlobal[detectorID]->Fill(cluster->globalX(), cluster->globalY());
            clusterSize[detectorID]->Fill(cluster->size());
        }
        clustersPerEvent[detectorID]->Fill(m_eventNumber, clusters->size());
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void Timepix1Correlator::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
