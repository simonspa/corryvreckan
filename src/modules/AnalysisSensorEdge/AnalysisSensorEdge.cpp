/**
 * @file
 * @brief Implementation of module AnalysisSensorEdge
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisSensorEdge.h"

using namespace corryvreckan;

AnalysisSensorEdge::AnalysisSensorEdge(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("inpixel_bin_size", Units::get<double>(1.0, "um"));
    m_inpixelBinSize = config_.get<double>("inpixel_bin_size");
}

void AnalysisSensorEdge::initialize() {

    auto px = static_cast<double>(Units::convert(m_detector->getPitch().X(), "um"));
    auto py = static_cast<double>(Units::convert(m_detector->getPitch().Y(), "um"));

    auto nx = static_cast<int>(std::ceil(m_detector->getPitch().X() / m_inpixelBinSize));
    auto ny = static_cast<int>(std::ceil(m_detector->getPitch().Y() / m_inpixelBinSize));
    if(nx > 1e4 || ny > 1e4) {
        throw InvalidValueError(config_, "inpixel_bin_size", "Too many bins for in-pixel histograms.");
    }

    std::string title = m_detector->getName() +
                        " Pixel efficiency map (first column);in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    efficiencyFirstCol =
        new TProfile2D("efficiencyFirstCol", title.c_str(), nx, -px / 2., px / 2., ny, -py / 2., py / 2., 0, 1);

    title = m_detector->getName() +
            " Pixel efficiency map (last column);in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    efficiencyLastCol =
        new TProfile2D("efficiencyLastCol", title.c_str(), nx, -px / 2., px / 2., ny, -py / 2., py / 2., 0, 1);

    title = m_detector->getName() +
            " Pixel efficiency map (first row);in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    efficiencyFirstRow =
        new TProfile2D("efficiencyFirstRow", title.c_str(), nx, -px / 2., px / 2., ny, -py / 2., py / 2., 0, 1);

    title = m_detector->getName() +
            " Pixel efficiency map (last row);in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    efficiencyLastRow =
        new TProfile2D("efficiencyLastRow", title.c_str(), nx, -px / 2., px / 2., ny, -py / 2., py / 2., 0, 1);

    title =
        m_detector->getName() +
        " Pixel efficiency map (all edges, edge to the left);in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    efficiencyEdges = new TProfile2D("efficiencyEdges", title.c_str(), nx, -px / 2., px / 2., ny, -py / 2., py / 2., 0, 1);

    title = "Position of tracks used;x [px];y [px];tracks";
    trackPositionsUsed = new TH2D("trackPositionsUsed",
                                  title.c_str(),
                                  m_detector->nPixels().X(),
                                  -0.5,
                                  m_detector->nPixels().X() - 0.5,
                                  m_detector->nPixels().Y(),
                                  -0.5,
                                  m_detector->nPixels().Y() - 0.5);

    title = "Position of tracks unused;x [px];y [px];tracks";
    trackPositionsUnused = new TH2D("trackPositionsUnused",
                                    title.c_str(),
                                    m_detector->nPixels().X(),
                                    -0.5,
                                    m_detector->nPixels().X() - 0.5,
                                    m_detector->nPixels().Y(),
                                    -0.5,
                                    m_detector->nPixels().Y() - 0.5);
}

StatusCode AnalysisSensorEdge::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();

    // Loop over all tracks
    for(auto& track : tracks) {
        LOG(DEBUG) << "Looking at next track";

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        LOG(TRACE) << " Checking if track is outside DUT area";
        if(!m_detector->hasIntercept(track.get())) {
            LOG(DEBUG) << " - track outside DUT area: " << localIntercept;
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        LOG(TRACE) << " Checking if track is outside ROI";
        if(!m_detector->isWithinROI(track.get())) {
            LOG(DEBUG) << " - track outside ROI";
            continue;
        }

        // Check that it doesn't go through/near a masked pixel
        LOG(TRACE) << " Checking if track is close to masked pixel";
        if(m_detector->hitMasked(track.get(), 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod = static_cast<double>(Units::convert(inpixel.X(), "um"));
        auto ymod = static_cast<double>(Units::convert(inpixel.Y(), "um"));

        // Get the DUT clusters from the clipboard, that are assigned to the track
        auto associated_clusters = track->getAssociatedClusters(m_detector->getName());
        bool has_associated_cluster = (associated_clusters.size() > 0);

        // Get the pixel index we're talking about:
        int column = static_cast<int>(std::round(m_detector->getColumn(localIntercept)));
        int row = static_cast<int>(std::round(m_detector->getRow(localIntercept)));

        LOG(DEBUG) << "Track impact at pixel (" << column << ", " << row << ")" << std::endl
                   << " - (" << m_detector->getColumn(localIntercept) << ", " << m_detector->getRow(localIntercept) << ")";

        bool edge = false;
        // Fill left and right edge histograms:
        if(column == 0) {
            efficiencyFirstCol->Fill(xmod, ymod, has_associated_cluster);
            // All-edge plot uses FirstColumn as reference orientation
            efficiencyEdges->Fill(xmod, ymod, has_associated_cluster);
            edge = true;
        } else if(column == (m_detector->nPixels().x() - 1)) {
            efficiencyLastCol->Fill(xmod, ymod, has_associated_cluster);
            // All-edge plot needs flipping of the x-coordinate to get edge to the left:
            efficiencyEdges->Fill(m_detector->getPitch().x() - xmod, ymod, has_associated_cluster);
            edge = true;
        }

        // Fill top and bottom edge histograms:
        if(row == 0) {
            efficiencyFirstRow->Fill(xmod, ymod, has_associated_cluster);
            // All-edge plot needs rotation by 90deg to get edge to the left:
            efficiencyEdges->Fill(ymod, xmod, has_associated_cluster);
            edge = true;
        } else if(row == (m_detector->nPixels().y() - 1)) {
            efficiencyLastRow->Fill(xmod, ymod, has_associated_cluster);
            // All-edge plot needs rotation by 90deg and flipping of y-axis to get edge to the left:
            efficiencyEdges->Fill(m_detector->getPitch().y() - ymod, xmod, has_associated_cluster);
            edge = true;
        }

        // Mark track impact position for reference
        if(edge) {
            LOG(INFO) << "Impact on edge at pixel (" << column << ", " << row << ")";
            trackPositionsUsed->Fill(m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept));
        } else {
            trackPositionsUnused->Fill(m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept));
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
