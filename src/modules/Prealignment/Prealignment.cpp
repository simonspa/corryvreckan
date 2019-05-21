#include "Prealignment.h"

using namespace corryvreckan;
using namespace std;

Prealignment::Prealignment(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    max_correlation_rms = m_config.get<double>("max_correlation_rms", Units::get<double>(6, "mm"));
    damping_factor = m_config.get<double>("damping_factor", 1.0);
    timingCut = m_config.get<double>("timing_cut", Units::get<double>(100, "ns"));
    LOG(DEBUG) << "Setting max_correlation_rms to : " << max_correlation_rms;
    LOG(DEBUG) << "Setting damping_factor to : " << damping_factor;
}

void Prealignment::initialise() {

    // get the reference detector:
    std::shared_ptr<Detector> reference = get_reference();

    // Correlation plots
    std::string title = m_detector->name() + ": correlation X;x_{ref}-x [mm];events";
    correlationX = new TH1F("correlationX", title.c_str(), 1000, -10., 10.);
    title = m_detector->name() + ": correlation Y;y_{ref}-y [mm];events";
    correlationY = new TH1F("correlationY", title.c_str(), 1000, -10., 10.);
    // 2D correlation plots (pixel-by-pixel, local coordinates):
    title = m_detector->name() + ": 2D correlation X (local);x [px];x_{ref} [px];events";
    correlationX2Dlocal = new TH2F("correlationX_2Dlocal",
                                   title.c_str(),
                                   m_detector->nPixels().X(),
                                   0,
                                   m_detector->nPixels().X(),
                                   reference->nPixels().X(),
                                   0,
                                   reference->nPixels().X());
    title = m_detector->name() + ": 2D correlation Y (local);y [px];y_{ref} [px];events";
    correlationY2Dlocal = new TH2F("correlationY_2Dlocal",
                                   title.c_str(),
                                   m_detector->nPixels().Y(),
                                   0,
                                   m_detector->nPixels().Y(),
                                   reference->nPixels().Y(),
                                   0,
                                   reference->nPixels().Y());
    title = m_detector->name() + ": 2D correlation X (global);x [mm];x_{ref} [mm];events";
    correlationX2D = new TH2F("correlationX_2D", title.c_str(), 100, -10., 10., 100, -10., 10.);
    title = m_detector->name() + ": 2D correlation Y (global);y [mm];y_{ref} [mm];events";
    correlationY2D = new TH2F("correlationY_2D", title.c_str(), 100, -10., 10., 100, -10., 10.);
}

StatusCode Prealignment::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the clusters
    auto clusters = clipboard->get<Cluster>(m_detector->name());
    if(clusters == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any clusters on the clipboard";
        return StatusCode::Success;
    }

    // Get clusters from reference detector
    auto reference = get_reference();
    auto referenceClusters = clipboard->get<Cluster>(reference->name());
    if(referenceClusters == nullptr) {
        LOG(DEBUG) << "Reference detector " << reference->name() << " does not have any clusters on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all clusters and fill histograms
    for(auto& cluster : (*clusters)) {
        // Loop over reference plane pixels to make correlation plots
        for(auto& refCluster : (*referenceClusters)) {
            double timeDifference = refCluster->timestamp() - cluster->timestamp();

            // Correlation plots
            if(abs(timeDifference) < timingCut) {
                correlationX->Fill(refCluster->global().x() - cluster->global().x());
                correlationX2D->Fill(cluster->global().x(), refCluster->global().x());
                correlationX2Dlocal->Fill(cluster->column(), refCluster->column());
                correlationY->Fill(refCluster->global().y() - cluster->global().y());
                correlationY2D->Fill(cluster->global().y(), refCluster->global().y());
                correlationY2Dlocal->Fill(cluster->row(), refCluster->row());
            }
        }
    }

    return StatusCode::Success;
}

void Prealignment::finalise() {

    double rmsX = correlationX->GetRMS();
    double rmsY = correlationY->GetRMS();
    if(rmsX > max_correlation_rms or rmsY > max_correlation_rms) {
        LOG(ERROR) << "Detector " << m_detector->name() << ": RMS is too wide for prealignment shifts";
        LOG(ERROR) << "Detector " << m_detector->name() << ": RMS X = " << Units::display(rmsX, {"mm", "um"})
                   << " , RMS Y = " << Units::display(rmsY, {"mm", "um"});
    }

    // Move all but the reference:
    if(!m_detector->isReference()) {
        double mean_X = correlationX->GetMean();
        double mean_Y = correlationY->GetMean();
        LOG(INFO) << "Detector " << m_detector->name() << ": x = " << Units::display(mean_X, {"mm", "um"})
                  << " , y = " << Units::display(mean_Y, {"mm", "um"});
        LOG(INFO) << "Move in x by = " << Units::display(mean_X * damping_factor, {"mm", "um"})
                  << " , and in y by = " << Units::display(mean_Y * damping_factor, {"mm", "um"});
        m_detector->displacement(XYZPoint(m_detector->displacement().X() + damping_factor * mean_X,
                                          m_detector->displacement().Y() + damping_factor * mean_Y,
                                          m_detector->displacement().Z()));
    }
}
