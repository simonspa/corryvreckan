#include "TestAlgorithm.h"

using namespace corryvreckan;
using namespace std;

TestAlgorithm::TestAlgorithm(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    makeCorrelations = m_config.get<bool>("make_correlations", false);
    timingCut = m_config.get<double>("timing_cut", Units::get<double>(100, "ns"));
    do_timing_cut_ = m_config.get<bool>("do_timing_cut", false);
}

void TestAlgorithm::initialise() {
    LOG(DEBUG) << "Booking histograms for detector " << m_detector->name();

    // get the reference detector:
    std::shared_ptr<Detector> reference = get_reference();

    // Simple hit map
    std::string title = m_detector->name() + ": hitmap;x [px];y [px];events";
    hitmap = new TH2F("hitmap",
                      title.c_str(),
                      m_detector->nPixels().X(),
                      0,
                      m_detector->nPixels().X(),
                      m_detector->nPixels().Y(),
                      0,
                      m_detector->nPixels().Y());

    if(makeCorrelations) {
        // Correlation plots
        title = m_detector->name() + ": correlation X;x_{ref}-x [mm];events";
        correlationX = new TH1F("correlationX", title.c_str(), 1000, -10., 10.);
        title = m_detector->name() + ": correlation Y;y_{ref}-y [mm];events";
        correlationY = new TH1F("correlationY", title.c_str(), 1000, -10., 10.);

        // time correlation plot range should cover length of events. nanosecond binning.
        title = m_detector->name() + "Reference cluster time stamp - cluster time stamp;t_{ref}-t [ns];events";
        correlationTime =
            new TH1F("correlationTime", title.c_str(), static_cast<int>(2. * timingCut), -1 * timingCut, timingCut);
        title = m_detector->name() + "Reference cluster time stamp - cluster time stamp;t_{ref}-t [1/40MHz];events";
        correlationTimeInt = new TH1F("correlationTimeInt", title.c_str(), 8000, -40000, 40000);

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
    // Timing plots
    title = m_detector->name() + ": event time;t [s];events";
    eventTimes = new TH1F("eventTimes", title.c_str(), 3000000, 0, 300);
}

StatusCode TestAlgorithm::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the pixels
    auto pixels = clipboard->get<Pixels>(m_detector->name());
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all pixels and make hitmaps
    for(auto& pixel : (*pixels)) {
        // Hitmap
        hitmap->Fill(pixel->column(), pixel->row());
        // Timing plots
        eventTimes->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "s")));
    }

    // Get the clusters
    auto clusters = clipboard->get<Clusters>(m_detector->name());
    if(clusters == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any clusters on the clipboard";
        return StatusCode::Success;
    }

    // Get clusters from reference detector
    auto reference = get_reference();
    auto referenceClusters = clipboard->get<Clusters>(reference->name());
    if(referenceClusters == nullptr) {
        LOG(DEBUG) << "Reference detector " << reference->name() << " does not have any clusters on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all clusters and fill histograms
    if(makeCorrelations) {
        for(auto& cluster : (*clusters)) {
            // Loop over reference plane pixels to make correlation plots
            for(auto& refCluster : (*referenceClusters)) {

                double timeDifference = refCluster->timestamp() - cluster->timestamp();
                // in 40 MHz:
                long long int timeDifferenceInt = static_cast<long long int>(timeDifference / 25);

                // Correlation plots
                if(abs(timeDifference) < timingCut || !do_timing_cut_) {
                    correlationX->Fill(refCluster->global().x() - cluster->global().x());
                    correlationX2D->Fill(cluster->global().x(), refCluster->global().x());
                    correlationX2Dlocal->Fill(cluster->column(), refCluster->column());
                }
                if(abs(timeDifference) < timingCut || !do_timing_cut_) {
                    correlationY->Fill(refCluster->global().y() - cluster->global().y());
                    correlationY2D->Fill(cluster->global().y(), refCluster->global().y());
                    correlationY2Dlocal->Fill(cluster->row(), refCluster->row());
                }
                //                    correlationTime[m_detector->name()]->Fill(Units::convert(timeDifference, "s"));
                correlationTime->Fill(timeDifference); // time difference in ns
                correlationTimeInt->Fill(static_cast<double>(timeDifferenceInt));
            }
        }
    }

    return StatusCode::Success;
}
