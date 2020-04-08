/**
 * @file
 * @brief Implementation of module DUTAssociation
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    // Backwards compatibilty: also allow timing_cut to be used for time_cut_abs and spatial_cut for spatial_cut_abs
    m_config.setAlias("time_cut_abs", "timing_cut", true);
    m_config.setAlias("spatial_cut_abs", "spatial_cut", true);

    // timing cut, relative (x * time_resolution) or absolute:
    if(m_config.count({"time_cut_rel", "time_cut_abs"}) > 1) {
        throw InvalidCombinationError(
            m_config, {"time_cut_rel", "time_cut_abs"}, "Absolute and relative time cuts are mutually exclusive.");
    } else if(m_config.has("time_cut_abs")) {
        timeCut = m_config.get<double>("time_cut_abs");
    } else {
        timeCut = m_config.get<double>("time_cut_rel", 3.0) * m_detector->getTimeResolution();
    }

    // spatial cut, relative (x * spatial_resolution) or absolute:
    if(m_config.count({"spatial_cut_rel", "spatial_cut_abs"}) > 1) {
        throw InvalidCombinationError(
            m_config, {"spatial_cut_rel", "spatial_cut_abs"}, "Absolute and relative spatial cuts are mutually exclusive.");
    } else if(m_config.has("spatial_cut_abs")) {
        spatialCut = m_config.get<XYVector>("spatial_cut_abs");
    } else {
        spatialCut = m_config.get<double>("spatial_cut_rel", 3.0) * m_detector->getSpatialResolution();
    }
    useClusterCentre = m_config.get<bool>("use_cluster_centre", false);

    LOG(DEBUG) << "time_cut = " << Units::display(timeCut, {"ms", "us", "ns"});
    LOG(DEBUG) << "spatial_cut = " << Units::display(spatialCut, {"um", "mm"});
    LOG(DEBUG) << "use_cluster_centre = " << useClusterCentre;
}

void DUTAssociation::initialise() {
    // Cut flow histogram
    std::string title = m_detector->getName() + ": number of tracks discarded by different cuts;cut type;clusters";
    hCutHisto = new TH1F("hCutHisto", title.c_str(), 2, 1, 3);
    hCutHisto->GetXaxis()->SetBinLabel(1, "Spatial");
    hCutHisto->GetXaxis()->SetBinLabel(2, "Timing");

    hDistX = new TH1D("hDistXClusterClosestPx",
                      "Distance cluster center to pixel closest to track; x_{cluster} - x_{closest pixel} [um]; # events",
                      2000,
                      -1000,
                      1000);
    hDistY = new TH1D("hDistYClusterClosestPx",
                      "Distance cluster center to pixel closest to track; y_{cluster} - y_{closest pixel} [um]; # events",
                      2000,
                      -1000,
                      1000);
    hDistX_1px =
        new TH1D("hDistXClusterClosestPx_1px",
                 "Distance 1px-cluster center to pixel closest to track; x_{cluster} - x_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);
    hDistY_1px =
        new TH1D("hDistYClusterClosestPx_1px",
                 "Distance 1px-cluster center to pixel closest to track; y_{cluster} - y_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);
    hDistX_2px =
        new TH1D("hDistXClusterClosestPx_2px",
                 "Distance 2px-cluster center to pixel closest to track; x_{cluster} - x_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);
    hDistY_2px =
        new TH1D("hDistYClusterClosestPx_2px",
                 "Distance 2px-cluster center to pixel closest to track; y_{cluster} - y_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);
    hDistX_3px =
        new TH1D("hDistXClusterClosestPx_3px",
                 "Distance 3px-cluster center to pixel closest to track; x_{cluster} - x_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);
    hDistY_3px =
        new TH1D("hDistYClusterClosestPx_3px",
                 "Distance 3px-cluster center to pixel closest to track; y_{cluster} - y_{closest pixel} [um]; # events",
                 2000,
                 -1000,
                 1000);

    // Nr of associated clusters per track
    title = m_detector->getName() + ": number of associated clusters per track;associated clusters;events";
    hNoAssocCls = new TH1F("no_assoc_cls", title.c_str(), 10, 0, 10);
    LOG(DEBUG) << "DUT association time cut = " << Units::display(timeCut, {"ms", "ns"});
}

StatusCode DUTAssociation::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Get the DUT clusters from the clipboard
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        LOG(TRACE) << "Proccessing track with model " << track->getType() << ", chi2 of " << track->chi2();
        int assoc_cls_per_track = 0;
        auto min_distance = std::numeric_limits<double>::max();

        if(clusters == nullptr) {
            hNoAssocCls->Fill(0);
            LOG(DEBUG) << "No DUT clusters on the clipboard";
            continue;
        }

        // Loop over all DUT clusters
        for(auto& cluster : (*clusters)) {
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->global().z());
            auto interceptLocal = m_detector->globalToLocal(intercept);

            // distance of track to cluster centre
            double xdistance_centre = std::abs(interceptLocal.X() - cluster->local().x());
            double ydistance_centre = std::abs(interceptLocal.Y() - cluster->local().y());

            // distance of track to nearest pixel: initialise to maximal possible value
            auto xdistance_nearest = std::numeric_limits<double>::max();
            auto ydistance_nearest = std::numeric_limits<double>::max();

            for(auto& pixel : cluster->pixels()) {
                // convert pixel address to local coordinates:
                auto pixelPositionLocal =
                    m_detector->getLocalPosition(static_cast<double>(pixel->column()), static_cast<double>(pixel->row()));

                xdistance_nearest = std::min(xdistance_nearest, std::abs(interceptLocal.X() - pixelPositionLocal.x()));
                ydistance_nearest = std::min(ydistance_nearest, std::abs(interceptLocal.Y() - pixelPositionLocal.y()));
            }

            hDistX->Fill(xdistance_centre - xdistance_nearest);
            hDistY->Fill(ydistance_centre - ydistance_nearest);
            if(cluster->columnWidth() == 1) {
                hDistX_1px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 1) {
                hDistY_1px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }
            if(cluster->columnWidth() == 2) {
                hDistX_2px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 2) {
                hDistY_2px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }
            if(cluster->columnWidth() == 3) {
                hDistX_3px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 3) {
                hDistY_3px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }

            // Check if the cluster is close in space (either use cluster centre of closest pixel to track)
            auto xdistance = (useClusterCentre ? xdistance_centre : xdistance_nearest);
            auto ydistance = (useClusterCentre ? ydistance_centre : ydistance_nearest);
            auto distance = sqrt(xdistance * xdistance + ydistance * ydistance);
            if(std::abs(xdistance) > spatialCut.x() || std::abs(ydistance) > spatialCut.y()) {
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << Units::display(std::abs(xdistance), {"um", "mm"})
                           << "," << Units::display(std::abs(ydistance), {"um", "mm"}) << ")"
                           << " with local track intersection at " << Units::display(interceptLocal, {"um", "mm"});
                hCutHisto->Fill(1);
                num_cluster++;
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timeCut) {
                LOG(DEBUG) << "Discarding DUT cluster with time difference "
                           << Units::display(std::abs(cluster->timestamp() - track->timestamp()), {"ms", "s"});
                hCutHisto->Fill(2);
                num_cluster++;
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                       << Units::display(abs(ydistance), {"um", "mm"}) << ")";
            track->addAssociatedCluster(cluster);
            assoc_cls_per_track++;
            assoc_cluster_counter++;
            num_cluster++;

            // check if cluster is closest to track
            if(distance < min_distance) {
                min_distance = distance;
                track->setClosestCluster(cluster);
            }
        }
        hNoAssocCls->Fill(assoc_cls_per_track);
        if(assoc_cls_per_track > 0) {
            track_w_assoc_cls++;
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void DUTAssociation::finalise() {
    hCutHisto->Scale(1 / double(num_cluster));
    LOG(STATUS) << "In total, " << assoc_cluster_counter << " clusters are associated to " << track_w_assoc_cls
                << " tracks.";
    LOG(INFO) << "Number of tracks with at least one associated cluster: " << track_w_assoc_cls;
    return;
}
