/**
 * @file
 * @brief Implementation of module Correlations
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Correlations.h"
#include "tools/cuts.h"

using namespace corryvreckan;
using namespace std;

Correlations::Correlations(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    // Backwards compatibility: also allow timing_cut to be used for time_cut_abs
    config_.setAlias("time_cut_abs", "timing_cut", true);
    config_.setAlias("do_time_cut", "do_timing_cut", true);

    config_.setDefault<bool>("do_time_cut", false);
    config_.setDefault<bool>("correlation_vs_time", false);
    config_.setDefault<double>("time_binning", Units::get<double>(1, "ns"));

    if(config_.count({"time_cut_rel", "time_cut_abs"}) == 0) {
        config_.setDefault("time_cut_rel", 3.0);
    }

    // timing cut, relative (x * time_resolution) or absolute:
    time_cut_ = corryvreckan::calculate_cut<double>("time_cut", config_, m_detector);
    do_time_cut_ = config_.get<bool>("do_time_cut");

    corr_vs_time_ = config_.get<bool>("correlation_vs_time");
    time_binning_ = config_.get<double>("time_binning");
}

void Correlations::initialize() {

    LOG_ONCE(WARNING) << "Correlations module is enabled and will significantly increase the runtime";
    LOG(DEBUG) << "Booking histograms for detector " << m_detector->getName();

    // get the reference detector:
    std::shared_ptr<Detector> reference = get_reference();

    // Simple hit map
    std::string title = m_detector->getName() + ": hitmap;x [px];y [px];events";
    hitmap = new TH2F("hitmap",
                      title.c_str(),
                      m_detector->nPixels().X(),
                      -0.5,
                      m_detector->nPixels().X() - 0.5,
                      m_detector->nPixels().Y(),
                      -0.5,
                      m_detector->nPixels().Y() - 0.5);
    title = m_detector->getName() + ": hitmap of clusters;x [px];y [px];events";
    hitmap_clusters = new TH2F("hitmap_clusters",
                               title.c_str(),
                               m_detector->nPixels().X(),
                               -0.5,
                               m_detector->nPixels().X() - 0.5,
                               m_detector->nPixels().Y(),
                               -0.5,
                               m_detector->nPixels().Y() - 0.5);

    // Correlation plots
    title = m_detector->getName() + ": correlation X;x_{ref}-x [mm];events";
    correlationX = new TH1F("correlationX", title.c_str(), 1000, -10.01, 9.99);
    title = m_detector->getName() + ": correlation Y;y_{ref}-y [mm];events";
    correlationY = new TH1F("correlationY", title.c_str(), 1000, -10.01, 9.99);
    title = m_detector->getName() + ": correlation XY;y_{ref}-x [mm];events";
    correlationXY = new TH1F("correlationXY", title.c_str(), 1000, -10.01, 9.99);
    title = m_detector->getName() + ": correlation YX;x_{ref}-y [mm];events";
    correlationYX = new TH1F("correlationYX", title.c_str(), 1000, -10.01, 9.99);

    // time correlation plot range should cover length of events. nanosecond binning.
    title = m_detector->getName() + "Reference cluster time stamp - cluster time stamp;t_{ref}-t [ns];events";
    correlationTime = new TH1F("correlationTime",
                               title.c_str(),
                               static_cast<int>(2. * time_cut_ / time_binning_),
                               -1 * time_cut_ - time_binning_ / 2.,
                               time_cut_ - time_binning_ / 2.);

    if(corr_vs_time_) {
        title = m_detector->getName() + " Correlation X versus time;t [s];x_{ref}-x [mm];events";
        std::string name = "correlationXVsTime";
        correlationXVsTime = new TH2F(name.c_str(), title.c_str(), 600, -2.5, 3e3 - 2.5, 200, -10.05, 9.95);

        title = m_detector->getName() + " Correlation Y versus time;t [s];y_{ref}-y [mm];events";
        name = "correlationYVsTime";
        correlationYVsTime = new TH2F(name.c_str(), title.c_str(), 600, -2.5, 3e3 - 2.5, 200, -10.05, 9.95);

        title = m_detector->getName() + "Reference pixel time stamp - pixel timestamp over time;t [s];t_{ref}-t [ns];events";
        correlationTimeOverTime_px = new TH2F("correlationTimeOverTime_px",
                                              title.c_str(),
                                              3e3,
                                              -0.5,
                                              3e3 - 0.5,
                                              static_cast<int>(2. * time_cut_ / time_binning_),
                                              -1 * time_cut_ - time_binning_ / 2.,
                                              time_cut_ - time_binning_ / 2.);

        title = m_detector->getName() + " Cross-Correlation XY versus time;t [s];x_{ref}-y [mm];events";
        name = "correlationXYVsTime";
        correlationXYVsTime = new TH2F(name.c_str(), title.c_str(), 600, -2.5, 3e3 - 2.5, 200, -10.05, 9.95);

        title = m_detector->getName() + " Cross-Correlation YX versus time;t [s];y_{ref}-x [mm];events";
        name = "correlationYXVsTime";
        correlationYXVsTime = new TH2F(name.c_str(), title.c_str(), 600, -2.5, 3e3 - 2.5, 200, -10.05, 9.95);

        title = m_detector->getName() +
                "Reference cluster time stamp - cluster time stamp over time;t [s];t_{ref}-t [ns];events";
        correlationTimeOverTime = new TH2F("correlationTimeOverTime",
                                           title.c_str(),
                                           3e3,
                                           -0.5,
                                           3e3 - 0.5,
                                           static_cast<int>(2. * time_cut_ / time_binning_),
                                           -1 * time_cut_ - time_binning_ / 2.,
                                           time_cut_ - time_binning_ / 2.);
        title = m_detector->getName() + "Reference cluster time stamp - cluster time stamp over seed pixel raw value;seed "
                                        "pixel raw value [lsb];t_{ref}-t [ns];events";
        correlationTimeOverSeedPixelRawValue = new TH2F("correlationTimeOverSeedPixelRawValue",
                                                        title.c_str(),
                                                        32,
                                                        -0.5,
                                                        31.5,
                                                        static_cast<int>(2. * time_cut_ / time_binning_),
                                                        -1 * time_cut_ - time_binning_ / 2.,
                                                        time_cut_ - time_binning_ / 2.);

        title = m_detector->getName() + "Reference pixel time stamp - pixel time stamp over pixel raw value;"
                                        "pixel raw value [lsb];t_{ref}-t [ns];events";
        correlationTimeOverPixelRawValue_px = new TH2F("correlationTimeOverSeedPixelRawValue_px",
                                                       title.c_str(),
                                                       32,
                                                       -0.5,
                                                       31.5,
                                                       static_cast<int>(2. * time_cut_ / time_binning_),
                                                       -1 * time_cut_ - time_binning_ / 2.,
                                                       time_cut_ - time_binning_ / 2.);
    }

    title = m_detector->getName() + "Reference pixel time stamp - pixel time stamp;t_{ref}-t [ns];events";
    correlationTime_px = new TH1F("correlationTime_px",
                                  title.c_str(),
                                  static_cast<int>(2. * time_cut_ / time_binning_),
                                  -1 * time_cut_ - time_binning_ / 2.,
                                  time_cut_ - time_binning_ / 2.);
    title = m_detector->getName() + "Reference cluster time stamp - cluster time stamp;t_{ref}-t [1/40MHz];events";
    correlationTimeInt = new TH1F("correlationTimeInt", title.c_str(), 8000, -40005, 39995);

    // 2D correlation plots (pixel-by-pixel, local coordinates):
    title = m_detector->getName() + ": 2D correlation X (local);x [px];x_{ref} [px];events";
    correlationX2Dlocal = new TH2F("correlationX_2Dlocal",
                                   title.c_str(),
                                   m_detector->nPixels().X(),
                                   -0.5,
                                   m_detector->nPixels().X() - 0.5,
                                   reference->nPixels().X(),
                                   -0.5,
                                   reference->nPixels().X() - 0.5);
    title = m_detector->getName() + ": 2D correlation Y (local);y [px];y_{ref} [px];events";
    correlationY2Dlocal = new TH2F("correlationY_2Dlocal",
                                   title.c_str(),
                                   m_detector->nPixels().Y(),
                                   -0.5,
                                   m_detector->nPixels().Y() - 0.5,
                                   reference->nPixels().Y(),
                                   -0.5,
                                   reference->nPixels().Y() - 0.5);
    title = m_detector->getName() + ": correlation col to col;col [px];col_{ref} [px];events";
    correlationColCol_px = new TH2F("correlationColCol_px",
                                    title.c_str(),
                                    m_detector->nPixels().X(),
                                    -0.5,
                                    m_detector->nPixels().X() - 0.5,
                                    reference->nPixels().X(),
                                    -0.5,
                                    reference->nPixels().X() - 0.5);
    title = m_detector->getName() + ": correlation col to row;col [px];row_{ref} [px];events";
    correlationColRow_px = new TH2F("correlationColRow_px",
                                    title.c_str(),
                                    m_detector->nPixels().X(),
                                    -0.5,
                                    m_detector->nPixels().X() - 0.5,
                                    reference->nPixels().Y(),
                                    -0.5,
                                    reference->nPixels().Y() - 0.5);
    title = m_detector->getName() + ": correlation row to col;row [px];col_{ref} [px];events";
    correlationRowCol_px = new TH2F("correlationRowCol_px",
                                    title.c_str(),
                                    m_detector->nPixels().Y(),
                                    -0.5,
                                    m_detector->nPixels().Y() - 0.5,
                                    reference->nPixels().X(),
                                    -0.5,
                                    reference->nPixels().X() - 0.5);
    title = m_detector->getName() + ": correlation row to row;row [px];row_{ref} [px];events";
    correlationRowRow_px = new TH2F("correlationRowRow_px",
                                    title.c_str(),
                                    m_detector->nPixels().Y(),
                                    -0.5,
                                    m_detector->nPixels().Y() - 0.5,
                                    reference->nPixels().Y(),
                                    -0.5,
                                    reference->nPixels().Y() - 0.5);

    title = m_detector->getName() + ": 2D correlation X (global);x [mm];x_{ref} [mm];events";
    correlationX2D = new TH2F("correlationX_2D", title.c_str(), 100, -10.1, 9.9, 100, -10.1, 9.9);
    title = m_detector->getName() + ": 2D correlation Y (global);y [mm];y_{ref} [mm];events";
    correlationY2D = new TH2F("correlationY_2D", title.c_str(), 100, -10.1, 9.9, 100, -10.1, 9.9);

    title = m_detector->getName() + ": 2D cross-correlation X/Y (global);x [mm];y_{ref} [mm];events";
    correlationXY2D = new TH2F("correlationXY_2D", title.c_str(), 100, -10.1, 9.9, 100, -10.1, 9.9);
    title = m_detector->getName() + ": 2D cross-correlation Y/X (global);y [mm];x_{ref} [mm];events";
    correlationYX2D = new TH2F("correlationYX_2D", title.c_str(), 100, -10.1, 9.9, 100, -10.1, 9.9);

    // Timing plots
    title = m_detector->getName() + ": event time;t [s];events";
    eventTimes = new TH1F("eventTimes", title.c_str(), 3000000, -1e-5, 300 - 1e-5);
}

StatusCode Correlations::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the pixels
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    for(auto& pixel : pixels) {
        // Hitmap
        hitmap->Fill(pixel->column(), pixel->row());
        // Timing plots
        eventTimes->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "s")));
    }

    // Get the clusters
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());
    for(auto& cluster : clusters) {
        hitmap_clusters->Fill(cluster->column(), cluster->row());
    }

    // Get pixels/clusters from reference detector
    auto reference = get_reference();
    auto referencePixels = clipboard->getData<Pixel>(reference->getName());
    auto referenceClusters = clipboard->getData<Cluster>(reference->getName());
    for(auto& pixel : pixels) {
        // Loop over reference plane pixels:
        for(auto& refPixel : referencePixels) {
            correlationColCol_px->Fill(pixel->column(), refPixel->column());
            correlationColRow_px->Fill(pixel->column(), refPixel->row());
            correlationRowCol_px->Fill(pixel->row(), refPixel->column());
            correlationRowRow_px->Fill(pixel->row(), refPixel->row());

            double timeDiff = refPixel->timestamp() - pixel->timestamp();
            correlationTime_px->Fill(static_cast<double>(Units::convert(timeDiff, "ns")));
            if(corr_vs_time_) {
                correlationTimeOverTime_px->Fill(static_cast<double>(Units::convert(pixel->timestamp(), "s")), timeDiff);
                correlationTimeOverPixelRawValue_px->Fill(pixel->raw(), timeDiff);
            }
        }
    }

    for(auto& cluster : clusters) {

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(cluster.get())) {
            LOG(DEBUG) << " - cluster outside ROI";
            continue;
        }

        // Loop over reference plane clusters to make correlation plots
        for(auto& refCluster : referenceClusters) {

            double timeDifference = refCluster->timestamp() - cluster->timestamp();
            // in 40 MHz:
            long long int timeDifferenceInt = static_cast<long long int>(timeDifference / 25);

            // Correlation plots
            if(abs(timeDifference) < time_cut_ || !do_time_cut_) {
                correlationX->Fill(refCluster->global().x() - cluster->global().x());
                correlationX2D->Fill(cluster->global().x(), refCluster->global().x());
                correlationX2Dlocal->Fill(cluster->column(), refCluster->column());

                correlationY->Fill(refCluster->global().y() - cluster->global().y());
                correlationY2D->Fill(cluster->global().y(), refCluster->global().y());
                correlationY2Dlocal->Fill(cluster->row(), refCluster->row());

                correlationXY->Fill(refCluster->global().y() - cluster->global().x());
                correlationXY2D->Fill(refCluster->global().y(), cluster->global().x());
                correlationYX->Fill(refCluster->global().x() - cluster->global().y());
                correlationYX2D->Fill(refCluster->global().x(), cluster->global().y());
            }

            correlationTime->Fill(timeDifference); // time difference in ns
            LOG(DEBUG) << "Time difference: " << Units::display(timeDifference, {"ns", "us"})
                       << ", Time ref. cluster: " << Units::display(refCluster->timestamp(), {"ns", "us"})
                       << ", Time cluster: " << Units::display(cluster->timestamp(), {"ns", "us"});

            if(corr_vs_time_) {
                if(abs(timeDifference) < time_cut_ || !do_time_cut_) {
                    correlationXVsTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                             refCluster->global().x() - cluster->global().x());
                    correlationYVsTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                             refCluster->global().y() - cluster->global().y());
                    correlationXYVsTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                              refCluster->global().x() - cluster->global().y());
                    correlationYXVsTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                              refCluster->global().y() - cluster->global().x());
                }
                // Time difference in ns
                correlationTimeOverTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                              timeDifference);
                correlationTimeOverSeedPixelRawValue->Fill(cluster->getSeedPixel()->raw(), timeDifference);
            }
            correlationTimeInt->Fill(static_cast<double>(timeDifferenceInt));
        }
    }

    return StatusCode::Success;
}
