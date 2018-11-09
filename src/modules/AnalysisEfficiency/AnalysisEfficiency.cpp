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

    m_timeCutFrameEdge = m_config.get<double>("timeCutFrameEdge", static_cast<double>(Units::convert(20, "ns")));
    m_chi2ndofCut = m_config.get<double>("chi2ndofCut", 3.);
}

void AnalysisEfficiency::initialise() {

    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));

    // Efficiency maps
    hPixelEfficiencyMap = new TProfile2D("hPixelEfficiencyMap",
                                         "hPixelEfficiencyMap",
                                         static_cast<int>(pitch_x),
                                         0,
                                         pitch_x,
                                         static_cast<int>(pitch_y),
                                         0,
                                         pitch_y,
                                         0,
                                         1);
    hChipEfficiencyMap = new TProfile2D("hChipEfficiencyMap",
                                        "hChipEfficiencyMap",
                                        m_detector->nPixelsX(),
                                        0,
                                        m_detector->nPixelsX(),
                                        m_detector->nPixelsY(),
                                        0,
                                        m_detector->nPixelsY(),
                                        0,
                                        1);
    hGlobalEfficiencyMap = new TProfile2D("hGlobalEfficiencyMap",
                                          "hGlobalEfficiencyMap",
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
        return Success;
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

        if(!m_detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
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

        // Discard tracks which are very close to the frame edges
        if(fabs(track->timestamp() - clipboard->get_persistent("eventEnd")) < m_timeCutFrameEdge) {
            // Late edge - eventEnd points to the end of the frame`
            LOG(DEBUG) << " - track close to end of readout frame: "
                       << Units::display(fabs(track->timestamp() - clipboard->get_persistent("eventEnd")), {"us", "ns"})
                       << " at " << Units::display(track->timestamp(), {"us"});
            continue;
        } else if(fabs(track->timestamp() - clipboard->get_persistent("eventStart")) < m_timeCutFrameEdge) {
            // Early edge - eventStart points to the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - clipboard->get_persistent("eventStart")), {"us", "ns"})
                       << " at " << Units::display(track->timestamp(), {"us"});
            continue;
        }

        // Count this as reference track:
        total_tracks++;

        // Calculate in-pixel position of track in microns
        auto xmod = static_cast<double>(Units::convert(m_detector->inPixelX(localIntercept), "um"));
        auto ymod = static_cast<double>(Units::convert(m_detector->inPixelY(localIntercept), "um"));

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
                    break;
                }
            }
        }

        hGlobalEfficiencyMap->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
        hChipEfficiencyMap->Fill(
            m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);

        // For pixels, only look at the ROI:
        if(is_within_roi) {
            hPixelEfficiencyMap->Fill(xmod, ymod, has_associated_cluster);
        }
    }

    return Success;
}

void AnalysisEfficiency::finalise() {
    LOG(STATUS) << "Total efficiency of detector " << m_detector->name() << ": "
                << (100 * matched_tracks / (total_tracks > 0 ? total_tracks : 1)) << "%, measured with " << total_tracks
                << " tracks";
}
