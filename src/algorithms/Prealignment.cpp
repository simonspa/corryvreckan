#include "Prealignment.h"

using namespace corryvreckan;
using namespace std;

Prealignment::Prealignment(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    LOG(INFO) << "Starting prealignment of detectors";
}

void Prealignment::initialise() {
    // Make histograms for each Timepix3
    for(auto& detector : get_detectors()) {
        LOG(INFO) << "Booking histograms for detector " << detector->name();

        // get the reference detector:
        Detector* reference = get_detector(m_config.get<std::string>("reference"));

        // Correlation plots
        string name = "correlationX_" + detector->name();
        correlationX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationY_" + detector->name();
        correlationY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationTime_" + detector->name();
        correlationTime[detector->name()] = new TH1F(name.c_str(), name.c_str(), 2000000, -0.5, 0.5);
        name = "correlationTimeInt_" + detector->name();
        correlationTimeInt[detector->name()] = new TH1F(name.c_str(), name.c_str(), 8000, -40000, 40000);

        // 2D correlation plots (pixel-by-pixel, local coordinates):
        name = "correlationX_2Dlocal_" + detector->name();
        correlationX2Dlocal[detector->name()] = new TH2F(name.c_str(),
                                                         name.c_str(),
                                                         detector->nPixelsX(),
                                                         0,
                                                         detector->nPixelsX(),
                                                         reference->nPixelsX(),
                                                         0,
                                                         reference->nPixelsX());
        name = "correlationY_2Dlocal_" + detector->name();
        correlationY2Dlocal[detector->name()] = new TH2F(name.c_str(),
                                                         name.c_str(),
                                                         detector->nPixelsY(),
                                                         0,
                                                         detector->nPixelsY(),
                                                         reference->nPixelsY(),
                                                         0,
                                                         reference->nPixelsY());
        name = "correlationX_2D_" + detector->name();
        correlationX2D[detector->name()] = new TH2F(name.c_str(), name.c_str(), 100, -10., 10., 100, -10., 10.);
        name = "correlationY_2D_" + detector->name();
        correlationY2D[detector->name()] = new TH2F(name.c_str(), name.c_str(), 100, -10., 10., 100, -10., 10.);
    }
}

StatusCode Prealignment::run(Clipboard* clipboard) {

    // Loop over all Timepix3 and make plots
    for(auto& detector : get_detectors()) {
        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detector->name(), "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any clusters on the clipboard";
            continue;
        }

        // Get clusters from reference detector
        Clusters* referenceClusters = (Clusters*)clipboard->get(m_config.get<std::string>("reference"), "clusters");
        if(referenceClusters == NULL) {
            LOG(DEBUG) << "Reference detector " << m_config.get<std::string>("reference")
                       << " does not have any clusters on the clipboard";
            //      continue;
        }

        // Loop over all clusters and fill histograms
        for(auto& cluster : (*clusters)) {
            // Loop over reference plane pixels to make correlation plots
            if(referenceClusters == NULL)
                continue;

            for(auto& refCluster : (*referenceClusters)) {
                long long int timeDifferenceInt = (refCluster->timestamp() - cluster->timestamp()) / 4096;

                double timeDifference = (double)(refCluster->timestamp() - cluster->timestamp()) / (4096. * 40000000.);

                // Correlation plots
                if(abs(timeDifference) < 0.000001) {
                    correlationX[detector->name()]->Fill(refCluster->globalX() - cluster->globalX());
                    correlationX2D[detector->name()]->Fill(cluster->globalX(), refCluster->globalX());
                    correlationX2Dlocal[detector->name()]->Fill(cluster->column(), refCluster->column());
                }
                if(abs(timeDifference) < 0.000001) {
                    correlationY[detector->name()]->Fill(refCluster->globalY() - cluster->globalY());
                    correlationY2D[detector->name()]->Fill(cluster->globalY(), refCluster->globalY());
                    correlationY2Dlocal[detector->name()]->Fill(cluster->row(), refCluster->row());
                }
                correlationTime[detector->name()]->Fill(timeDifference);
                correlationTimeInt[detector->name()]->Fill(timeDifferenceInt);
            } //*/
        }
    }

    return Success;
}

void Prealignment::finalise() {
    Detector* reference = get_detector(m_config.get<std::string>("reference"));
    for(auto& detector : get_detectors()) {
        if(detector != reference) {
            double mean_X = correlationX[detector->name()]->GetMean();
            double mean_Y = correlationY[detector->name()]->GetMean();
            LOG(DEBUG) << "Detector " << detector->name() << ": x = " << mean_X << " , y = " << mean_Y;
            LOG(DEBUG) << "Move in x by = " << mean_X * 0.2 << " , and in y by = " << mean_Y * 0.2;
            detector->displacementX(0.8 * mean_X);
            detector->displacementY(0.8 * mean_Y);
        }
    }
}
