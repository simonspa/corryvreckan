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
    for(auto& detector : m_detectors) {
        LOG(DEBUG) << "Booking histograms for detector " << detector->name();

        // Simple hit map
        string name = "hitmap_" + detector->name();
        hitmap[detector->name()] = new TH2F(name.c_str(),
                                            name.c_str(),
                                            detector->nPixelsX(),
                                            0,
                                            detector->nPixelsX(),
                                            detector->nPixelsY(),
                                            0,
                                            detector->nPixelsY());

        // Cluster plots
        name = "clusterSize_" + detector->name();
        clusterSize[detector->name()] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);
        name = "clusterTot_" + detector->name();
        clusterTot[detector->name()] = new TH1F(name.c_str(), name.c_str(), 200, 0, 1000);
        name = "clusterPositionGlobal_" + detector->name();
        clusterPositionGlobal[detector->name()] = new TH2F(name.c_str(), name.c_str(), 400, -10., 10., 400, -10., 10.);

        // Correlation plots
        name = "correlationX_" + detector->name();
        correlationX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationY_" + detector->name();
        correlationY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationTime_" + detector->name();
        correlationTime[detector->name()] = new TH1F(name.c_str(), name.c_str(), 2000000, -0.5, 0.5);
        name = "correlationTimeInt_" + detector->name();
        correlationTimeInt[detector->name()] = new TH1F(name.c_str(), name.c_str(), 8000, -40000, 40000);

        // Timing plots
        name = "eventTimes_" + detector->name();
        eventTimes[detector->name()] = new TH1F(name.c_str(), name.c_str(), 3000000, 0, 300);
    }
}

StatusCode TestAlgorithm::run(Clipboard* clipboard) {

    // Loop over all Timepix3 and make plots
    for(auto& detector : m_detectors) {
        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }

        // Loop over all pixels and make hitmaps
        for(auto& pixel : (*pixels)) {
            // Hitmap
            hitmap[detector->name()]->Fill(pixel->m_column, pixel->m_row);
            // Timing plots
            eventTimes[detector->name()]->Fill((double)pixel->m_timestamp / (4096. * 40000000.));
        }

        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detector->name(), "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any clusters on the clipboard";
            continue;
        }

        // Get clusters from reference detector
        Clusters* referenceClusters = (Clusters*)clipboard->get(parameters->reference, "clusters");
        if(referenceClusters == NULL) {
            LOG(DEBUG) << "Reference detector " << parameters->reference << " does not have any clusters on the clipboard";
            //      continue;
        }

        // Loop over all clusters and fill histograms
        for(auto& cluster : (*clusters)) {
            // Fill cluster histograms
            clusterSize[detector->name()]->Fill(cluster->size());
            clusterTot[detector->name()]->Fill(cluster->tot());
            clusterPositionGlobal[detector->name()]->Fill(cluster->globalX(), cluster->globalY());

            // Loop over reference plane pixels to make correlation plots
            if(!makeCorrelations)
                continue;
            if(referenceClusters == NULL)
                continue;

            for(auto& refCluster : (*referenceClusters)) {
                long long int timeDifferenceInt = (refCluster->timestamp() - cluster->timestamp()) / 4096;

                double timeDifference = (double)(refCluster->timestamp() - cluster->timestamp()) / (4096. * 40000000.);

                // Correlation plots
                if(abs(timeDifference) < 0.000001)
                    correlationX[detector->name()]->Fill(refCluster->globalX() - cluster->globalX());
                if(abs(timeDifference) < 0.000001)
                    correlationY[detector->name()]->Fill(refCluster->globalY() - cluster->globalY());
                correlationTime[detector->name()]->Fill(timeDifference);
                correlationTimeInt[detector->name()]->Fill(timeDifferenceInt);
            } //*/
        }
    }

    return Success;
}

void TestAlgorithm::finalise() {}
