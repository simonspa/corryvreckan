#include "Prealignment.h"

using namespace corryvreckan;
using namespace std;

Prealignment::Prealignment(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    LOG(INFO) << "Starting prealignment of detectors";
    max_correlation_rms = m_config.get<double>("max_correlation_rms", static_cast<double>(Units::convert(6.0, "mm")));
    damping_factor = m_config.get<double>("damping_factor", 1.0);
    timingCut = m_config.get<double>("timingCut", static_cast<double>(Units::convert(100, "ns")));
    LOG(DEBUG) << "Setting max_correlation_rms to : " << max_correlation_rms;
    LOG(DEBUG) << "Setting damping_factor to : " << damping_factor;
}

void Prealignment::initialise() {
    // Make histograms for each Timepix3
    for(auto& detector : get_detectors()) {
        LOG(INFO) << "Booking histograms for detector " << detector->name();

        // get the reference detector:
        Detector* reference = get_reference();

        // Correlation plots
        string name = "correlationX_" + detector->name();
        correlationX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationY_" + detector->name();
        correlationY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
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
        Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(detector->name(), "clusters"));
        if(clusters == nullptr) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any clusters on the clipboard";
            continue;
        }

        // Get clusters from reference detector
        auto reference = get_reference();
        Clusters* referenceClusters = reinterpret_cast<Clusters*>(clipboard->get(reference->name(), "clusters"));
        if(referenceClusters == nullptr) {
            LOG(DEBUG) << "Reference detector " << reference->name() << " does not have any clusters on the clipboard";
            continue;
        }

        // Loop over all clusters and fill histograms
        for(auto& cluster : (*clusters)) {
            // Loop over reference plane pixels to make correlation plots
            for(auto& refCluster : (*referenceClusters)) {
                double timeDifference = refCluster->timestamp() - cluster->timestamp();

                // Correlation plots
                if(abs(timeDifference) < timingCut) {
                    correlationX[detector->name()]->Fill(refCluster->globalX() - cluster->globalX());
                    correlationX2D[detector->name()]->Fill(cluster->globalX(), refCluster->globalX());
                    correlationX2Dlocal[detector->name()]->Fill(cluster->column(), refCluster->column());
                    correlationY[detector->name()]->Fill(refCluster->globalY() - cluster->globalY());
                    correlationY2D[detector->name()]->Fill(cluster->globalY(), refCluster->globalY());
                    correlationY2Dlocal[detector->name()]->Fill(cluster->row(), refCluster->row());
                }
            }
        }
    }

    return Success;
}

void Prealignment::finalise() {
    for(auto& detector : get_detectors()) {
        double rmsX = correlationX[detector->name()]->GetRMS();
        double rmsY = correlationY[detector->name()]->GetRMS();
        if(rmsX > max_correlation_rms or rmsY > max_correlation_rms) {
            LOG(ERROR) << "Detector " << detector->name() << ": RMS is too wide for prealignment shifts";
            LOG(ERROR) << "Detector " << detector->name() << ": RMS X = " << Units::display(rmsX, {"mm", "um"})
                       << " , RMS Y = " << Units::display(rmsY, {"mm", "um"});
        }
        if(detector->isReference()) {
            double mean_X = correlationX[detector->name()]->GetMean();
            double mean_Y = correlationY[detector->name()]->GetMean();
            LOG(INFO) << "Detector " << detector->name() << ": x = " << Units::display(mean_X, {"mm", "um"})
                      << " , y = " << Units::display(mean_Y, {"mm", "um"});
            LOG(INFO) << "Move in x by = " << Units::display(mean_X * damping_factor, {"mm", "um"})
                      << " , and in y by = " << Units::display(mean_Y * damping_factor, {"mm", "um"});
            double x = detector->displacement().X();
            double y = detector->displacement().Y();
            detector->displacementX(x + damping_factor * mean_X);
            detector->displacementY(y + damping_factor * mean_Y);
        }
    }
}
