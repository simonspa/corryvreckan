/**
 * @file
 * @brief Implementation of [AnalysisEfficiency] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisEfficiency.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisEfficiency::AnalysisEfficiency(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector) {
    m_detector = detector;

    m_timeCutFrameEdge = m_config.get<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    m_pixelTolerance = m_config.get<double>("pixel_tolerance", 1.);
    m_chi2ndofCut = m_config.get<double>("chi2ndof_cut", 3.);
    m_inpixelBinSize = m_config.get<double>("inpixel_bin_size", Units::get<double>(1.0, "um"));
}

void AnalysisEfficiency::initialise() {

    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));

    auto nbins_x = static_cast<int>(std::ceil(pitch_x / Units::convert(m_inpixelBinSize, "um")));
    auto nbins_y = static_cast<int>(std::ceil(pitch_y / Units::convert(m_inpixelBinSize, "um")));
    if(nbins_x > 1e4 || nbins_y > 1e4) {
        throw ModuleError("Parameter \"inpixel_bin_size\" is too small. Too many bins for ROOT. Please increase "
                          "\"inpixel_bin_size\" in your configuration file.");
    }
    std::string title = m_detector->name() + " Pixel efficiency map;x_{track} mod " + std::to_string(pitch_x) +
                        "#mum;y_{track} mod " + std::to_string(pitch_y) + "#mum;efficiency";
    hPixelEfficiencyMap_trackPos =
        new TProfile2D("pixelEfficiencyMap_trackPos", title.c_str(), nbins_x, 0, pitch_x, nbins_y, 0, pitch_y, 0, 1);
    title = m_detector->name() + " Chip efficiency map;x [px];y [px];efficiency";
    hChipEfficiencyMap_trackPos = new TProfile2D("chipEfficiencyMap_trackPos",
                                                 title.c_str(),
                                                 m_detector->nPixels().X(),
                                                 0,
                                                 m_detector->nPixels().X(),
                                                 m_detector->nPixels().Y(),
                                                 0,
                                                 m_detector->nPixels().Y(),
                                                 0,
                                                 1);
    title = m_detector->name() + " Global efficiency map;x [mm];y [mm];efficiency";
    hGlobalEfficiencyMap_trackPos = new TProfile2D("globalEfficiencyMap_trackPos",
                                                   title.c_str(),
                                                   300,
                                                   -1.5 * m_detector->size().X(),
                                                   1.5 * m_detector->size().X(),
                                                   300,
                                                   -1.5 * m_detector->size().Y(),
                                                   1.5 * m_detector->size().Y(),
                                                   0,
                                                   1);
    title = m_detector->name() + " Chip efficiency map;x [px];y [px];efficiency";
    hChipEfficiencyMap_clustPos = new TProfile2D("chipEfficiencyMap_clustPos",
                                                 title.c_str(),
                                                 m_detector->nPixels().X(),
                                                 0,
                                                 m_detector->nPixels().X(),
                                                 m_detector->nPixels().Y(),
                                                 0,
                                                 m_detector->nPixels().Y(),
                                                 0,
                                                 1);
    title = m_detector->name() + " Global efficiency map;x [mm];y [mm];efficiency";
    hGlobalEfficiencyMap_clustPos = new TProfile2D("globalEfficiencyMap_clustPos",
                                                   title.c_str(),
                                                   300,
                                                   -1.5 * m_detector->size().X(),
                                                   1.5 * m_detector->size().X(),
                                                   300,
                                                   -1.5 * m_detector->size().Y(),
                                                   1.5 * m_detector->size().Y(),
                                                   0,
                                                   1);
}

StatusCode AnalysisEfficiency::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the telescope tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        bool has_associated_cluster = false;
        bool is_within_roi = true;
        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->chi2ndof() > m_chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            continue;
        }

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track);
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        if(!m_detector->hasIntercept(track, m_pixelTolerance)) {
            LOG(DEBUG) << " - track outside DUT area: " << localIntercept;
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track)) {
            LOG(DEBUG) << " - track outside ROI";
            is_within_roi = false;
        }

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track, 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }

        // Get the event:
        auto event = clipboard->get_event();

        // Discard tracks which are very close to the frame edges
        if(fabs(track->timestamp() - event->end()) < m_timeCutFrameEdge) {
            // Late edge - eventEnd points to the end of the frame`
            LOG(DEBUG) << " - track close to end of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->end()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            continue;
        } else if(fabs(track->timestamp() - event->start()) < m_timeCutFrameEdge) {
            // Early edge - eventStart points to the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            continue;
        }

        // Count this as reference track:
        total_tracks++;

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod = static_cast<double>(Units::convert(inpixel.X(), "um"));
        auto ymod = static_cast<double>(Units::convert(inpixel.Y(), "um"));

        // Get the DUT clusters from the clipboard
        Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(m_detector->name(), "clusters"));
        if(clusters == nullptr) {
            LOG(DEBUG) << " - no DUT clusters";
        } else {

            // Loop over all DUT clusters to find matches:
            for(auto* cluster : (*clusters)) {
                LOG(DEBUG) << " - Looking at next DUT cluster";

                auto associated_clusters = track->associatedClusters();
                if(std::find(associated_clusters.begin(), associated_clusters.end(), cluster) != associated_clusters.end()) {
                    LOG(DEBUG) << "Found associated cluster " << (*cluster);
                    has_associated_cluster = true;
                    matched_tracks++;
                    auto clusterLocal = m_detector->globalToLocal(cluster->global());
                    hGlobalEfficiencyMap_clustPos->Fill(
                        cluster->global().x(), cluster->global().y(), has_associated_cluster);
                    hChipEfficiencyMap_clustPos->Fill(
                        m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal), has_associated_cluster);
                    break;
                }
            }
        }
        hGlobalEfficiencyMap_trackPos->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
        hChipEfficiencyMap_trackPos->Fill(
            m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);
        // For pixels, only look at the ROI:
        if(is_within_roi) {
            hPixelEfficiencyMap_trackPos->Fill(xmod, ymod, has_associated_cluster);
        }
        if(has_associated_cluster == false) {
            hGlobalEfficiencyMap_clustPos->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
            hChipEfficiencyMap_clustPos->Fill(
                m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);
        }
    }

    return StatusCode::Success;
}

void AnalysisEfficiency::finalise() {
    LOG(INFO) << "No. matched tracks=" << matched_tracks;
    LOG(INFO) << "Total no. tracks=" << total_tracks;
    LOG(STATUS) << "Total efficiency of detector " << m_detector->name() << ": "
                << (100 * matched_tracks / (total_tracks > 0 ? total_tracks : 1)) << "%, measured with " << total_tracks
                << " tracks";
}
