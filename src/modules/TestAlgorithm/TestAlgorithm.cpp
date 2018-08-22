#include "TestAlgorithm.h"

using namespace corryvreckan;
using namespace std;

TestAlgorithm::TestAlgorithm(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    makeCorrelations = m_config.get<bool>("makeCorrelations", false);
    timingCut = m_config.get<double>("timingCut", Units::convert(100, "ns"));
    LOG(DEBUG) << "Setting makeCorrelations to: " << makeCorrelations;

    m_eventLength = m_config.get<double>("eventLength", 1);

}

void TestAlgorithm::initialise() {

    // Make histograms for each Timepix3
    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Booking histograms for detector " << detector->name();

        // get the reference detector:
        Detector* reference = get_detector(m_config.get<std::string>("reference"));

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

        // Correlation plots
        name = "correlationX_" + detector->name();
        correlationX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationY_" + detector->name();
        correlationY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 1000, -10., 10.);
        name = "correlationTime_" + detector->name();
	// time correlation plot range should cover length of events. nanosecond binning.
        correlationTime[detector->name()] = new TH1F(name.c_str(), name.c_str(), (int)(2.*m_eventLength), -1*m_eventLength, m_eventLength);
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

        // Timing plots
        name = "eventTimes_" + detector->name();
        eventTimes[detector->name()] = new TH1F(name.c_str(), name.c_str(), 3000000, 0, 300);
    }
}

StatusCode TestAlgorithm::run(Clipboard* clipboard) {

    // Loop over all Timepix3 and make plots
    for(auto& detector : get_detectors()) {
        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }

        // Loop over all pixels and make hitmaps
        for(auto& pixel : (*pixels)) {
            // Hitmap
            hitmap[detector->name()]->Fill(pixel->column(), pixel->row());
            // Timing plots
            eventTimes[detector->name()]->Fill(Units::convert(pixel->timestamp(), "s"));
        }

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
            continue;
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
                        correlationX[detector->name()]->Fill(refCluster->globalX() - cluster->globalX());
                        correlationX2D[detector->name()]->Fill(cluster->globalX(), refCluster->globalX());
                        correlationX2Dlocal[detector->name()]->Fill(cluster->column(), refCluster->column());
                    }
                    if(abs(timeDifference) < timingCut) {
                        correlationY[detector->name()]->Fill(refCluster->globalY() - cluster->globalY());
                        correlationY2D[detector->name()]->Fill(cluster->globalY(), refCluster->globalY());
                        correlationY2Dlocal[detector->name()]->Fill(cluster->row(), refCluster->row());
                    }
                    correlationTime[detector->name()]->Fill(Units::convert(timeDifference, "s"));
                    correlationTimeInt[detector->name()]->Fill(timeDifferenceInt);
                }
            }
        }
    }

    return Success;
}

void TestAlgorithm::finalise() {}
