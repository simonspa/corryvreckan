#include "TestAlgorithm.h"

using namespace corryvreckan;
using namespace std;

TestAlgorithm::TestAlgorithm(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    makeCorrelations = m_config.get<bool>("makeCorrelations", false);
    timingCut = m_config.get<double>("timingCut", static_cast<double>(Units::convert(100, "ns")));
    m_eventLength = m_config.get<double>("eventLength", 1);
}

void TestAlgorithm::initialise() {
    LOG(DEBUG) << "Booking histograms for detector " << m_detector->name();

    // get the reference detector:
    std::shared_ptr<Detector> reference = get_reference();

    // Simple hit map
    hitmap = new TH2F("hitmap",
                      "hitmap",
                      m_detector->nPixelsX(),
                      0,
                      m_detector->nPixelsX(),
                      m_detector->nPixelsY(),
                      0,
                      m_detector->nPixelsY());

    // Correlation plots
    correlationX = new TH1F("correlationX", "correlationX", 1000, -10., 10.);
    correlationY = new TH1F("correlationY", "correlationY", 1000, -10., 10.);

    // time correlation plot range should cover length of events. nanosecond binning.
    correlationTime = new TH1F(
        "correlationTime", "correlationTime", static_cast<int>(2. * m_eventLength), -1 * m_eventLength, m_eventLength);
    correlationTime->GetXaxis()->SetTitle("Reference cluster time stamp - cluster time stamp [ns]");
    correlationTimeInt = new TH1F("correlationTimeInt", "correlationTimeInt", 8000, -40000, 40000);
    correlationTimeInt->GetXaxis()->SetTitle("Reference cluster time stamp - cluster time stamp [1/40 MHz]");

    // 2D correlation plots (pixel-by-pixel, local coordinates):
    correlationX2Dlocal = new TH2F("correlationX_2Dlocal",
                                   "correlationX_2Dlocal",
                                   m_detector->nPixelsX(),
                                   0,
                                   m_detector->nPixelsX(),
                                   reference->nPixelsX(),
                                   0,
                                   reference->nPixelsX());
    correlationY2Dlocal = new TH2F("correlationY_2Dlocal_",
                                   "correlationY_2Dlocal_",
                                   m_detector->nPixelsY(),
                                   0,
                                   m_detector->nPixelsY(),
                                   reference->nPixelsY(),
                                   0,
                                   reference->nPixelsY());

    correlationX2D = new TH2F("correlationX_2D", "correlationX_2D", 100, -10., 10., 100, -10., 10.);
    correlationY2D = new TH2F("correlationY_2D", "correlationY_2D", 100, -10., 10., 100, -10., 10.);

    // Timing plots
    eventTimes = new TH1F("eventTimes", "eventTimes", 3000000, 0, 300);
}

StatusCode TestAlgorithm::run(Clipboard* clipboard) {

    // Get the pixels
    Pixels* pixels = reinterpret_cast<Pixels*>(clipboard->get(m_detector->name(), "pixels"));
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return Success;
    }

    // Loop over all pixels and make hitmaps
    for(auto& pixel : (*pixels)) {
        // Hitmap
        hitmap->Fill(pixel->column(), pixel->row());
        // Timing plots
        eventTimes->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "s")));
    }

    // Get the clusters
    Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(m_detector->name(), "clusters"));
    if(clusters == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any clusters on the clipboard";
        return Success;
    }

    // Get clusters from reference detector
    auto reference = get_reference();
    Clusters* referenceClusters = reinterpret_cast<Clusters*>(clipboard->get(reference->name(), "clusters"));
    if(referenceClusters == nullptr) {
        LOG(DEBUG) << "Reference detector " << reference->name() << " does not have any clusters on the clipboard";
        return Success;
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
                if(abs(timeDifference) < timingCut) {
                    correlationX->Fill(refCluster->globalX() - cluster->globalX());
                    correlationX2D->Fill(cluster->globalX(), refCluster->globalX());
                    correlationX2Dlocal->Fill(cluster->column(), refCluster->column());
                }
                if(abs(timeDifference) < timingCut) {
                    correlationY->Fill(refCluster->globalY() - cluster->globalY());
                    correlationY2D->Fill(cluster->globalY(), refCluster->globalY());
                    correlationY2Dlocal->Fill(cluster->row(), refCluster->row());
                }
                //                    correlationTime[m_detector->name()]->Fill(Units::convert(timeDifference, "s"));
                correlationTime->Fill(timeDifference); // time difference in ns
                correlationTimeInt->Fill(static_cast<double>(timeDifferenceInt));
            }
        }
    }

    return Success;
}
