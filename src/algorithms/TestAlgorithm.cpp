#include "TestAlgorithm.h"

using namespace corryvreckan;
using namespace std;

TestAlgorithm::TestAlgorithm(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    makeCorrelations = m_config.get<bool>("makeCorrelations", false);
    LOG(DEBUG) << "Setting makeCorrelations to: " << makeCorrelations;
}

void TestAlgorithm::initialise(Parameters* par) {

    parameters = par;
    // Make histograms for each Timepix3
    for(int det = 0; det < parameters->nDetectors; det++) {

        // Check if they are a Timepix3
        string detectorID = parameters->detectors[det];
        if(parameters->detector[detectorID]->type() != "Timepix3")
            continue;

        // Simple hit map
        string name = "hitmap_" + detectorID;
        hitmap[detectorID] = new TH2F(name.c_str(), name.c_str(), 256, 0, 256, 256, 0, 256);

        // Cluster plots
        name = "clusterSize_" + detectorID;
        clusterSize[detectorID] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);
        name = "clusterTot_" + detectorID;
        clusterTot[detectorID] = new TH1F(name.c_str(), name.c_str(), 200, 0, 1000);
        name = "clusterPositionGlobal_" + detectorID;
        clusterPositionGlobal[detectorID] = new TH2F(name.c_str(), name.c_str(), 400, -10., 10., 400, -10., 10.);

        // Correlation plots
        name = "correlationX_" + detectorID;
        correlationX[detectorID] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationY_" + detectorID;
        correlationY[detectorID] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationTime_" + detectorID;
        correlationTime[detectorID] = new TH1F(name.c_str(), name.c_str(), 2000000, -0.5, 0.5);
        name = "correlationTimeInt_" + detectorID;
        correlationTimeInt[detectorID] = new TH1F(name.c_str(), name.c_str(), 8000, -40000, 40000);

        // Timing plots
        name = "eventTimes_" + detectorID;
        eventTimes[detectorID] = new TH1F(name.c_str(), name.c_str(), 3000000, 0, 300);
    }
}

StatusCode TestAlgorithm::run(Clipboard* clipboard) {

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

        // Loop over all pixels and make hitmaps
        for(int iP = 0; iP < pixels->size(); iP++) {

            // Get the pixel
            Pixel* pixel = (*pixels)[iP];

            // Hitmap
            hitmap[detectorID]->Fill(pixel->m_column, pixel->m_row);

            // Timing plots
            eventTimes[detectorID]->Fill((double)pixel->m_timestamp / (4096. * 40000000.));
        }

        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detectorID, "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any clusters on the clipboard";
            continue;
        }

        // Get clusters from reference detector
        Clusters* referenceClusters = (Clusters*)clipboard->get(parameters->reference, "clusters");
        if(referenceClusters == NULL) {
            LOG(DEBUG) << "Reference detector " << parameters->reference << " does not have any clusters on the clipboard";
            //      continue;
        }

        // Loop over all clusters and fill histograms
        for(int iCluster = 0; iCluster < clusters->size(); iCluster++) {

            // Get the cluster
            Cluster* cluster = (*clusters)[iCluster];

            // Fill cluster histograms
            clusterSize[detectorID]->Fill(cluster->size());
            clusterTot[detectorID]->Fill(cluster->tot());
            clusterPositionGlobal[detectorID]->Fill(cluster->globalX(), cluster->globalY());

            // Loop over reference plane pixels to make correlation plots
            if(!makeCorrelations)
                continue;
            if(referenceClusters == NULL)
                continue;
            for(int iRefCluster = 0; iRefCluster < referenceClusters->size(); iRefCluster++) {
                Cluster* refCluster = (*referenceClusters)[iRefCluster];

                long long int timeDifferenceInt = (refCluster->timestamp() - cluster->timestamp()) / 4096;

                double timeDifference = (double)(refCluster->timestamp() - cluster->timestamp()) / (4096. * 40000000.);

                // Correlation plots
                if(abs(timeDifference) < 0.000001)
                    correlationX[detectorID]->Fill(refCluster->globalX() - cluster->globalX());
                if(abs(timeDifference) < 0.000001)
                    correlationY[detectorID]->Fill(refCluster->globalY() - cluster->globalY());
                correlationTime[detectorID]->Fill(timeDifference);
                correlationTimeInt[detectorID]->Fill(timeDifferenceInt);
            } //*/
        }
    }

    return Success;
}

void TestAlgorithm::finalise() {}
