/**
 * @file
 * @brief Implementation of [AnalysisEfficiency] module
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisEfficiency.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisEfficiency::AnalysisEfficiency(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector) {
    m_detector = detector;

    config_.setDefault<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<double>("inpixel_bin_size", Units::get<double>(1.0, "um"));

    m_timeCutFrameEdge = config_.get<double>("time_cut_frameedge");
    m_chi2ndofCut = config_.get<double>("chi2ndof_cut");
    m_inpixelBinSize = config_.get<double>("inpixel_bin_size");
}

void AnalysisEfficiency::initialize() {

    hPixelEfficiency = new TH1D(
        "hPixelEfficiency", "hPixelEfficiency; single pixel efficiency; # entries", 201, 0, 1.005); // get 0.5%-wide bins

    auto pitch_x = static_cast<double>(Units::convert(m_detector->getPitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->getPitch().Y(), "um"));

    auto nbins_x = static_cast<int>(std::ceil(m_detector->getPitch().X() / m_inpixelBinSize));
    auto nbins_y = static_cast<int>(std::ceil(m_detector->getPitch().Y() / m_inpixelBinSize));
    if(nbins_x > 1e4 || nbins_y > 1e4) {
        throw InvalidValueError(config_, "inpixel_bin_size", "Too many bins for in-pixel histograms.");
    }
    std::string title =
        m_detector->getName() + " Pixel efficiency map;in-pixel x_{track} [#mum];in-pixel y_{track} #mum;efficiency";
    hPixelEfficiencyMap_trackPos = new TProfile2D("pixelEfficiencyMap_trackPos",
                                                  title.c_str(),
                                                  nbins_x,
                                                  -pitch_x / 2.,
                                                  pitch_x / 2.,
                                                  nbins_y,
                                                  -pitch_y / 2.,
                                                  pitch_y / 2.,
                                                  0,
                                                  1);
    title = m_detector->getName() + " Chip efficiency map;x [px];y [px];efficiency";
    hChipEfficiencyMap_trackPos = new TProfile2D("chipEfficiencyMap_trackPos",
                                                 title.c_str(),
                                                 m_detector->nPixels().X(),
                                                 -0.5,
                                                 m_detector->nPixels().X() - 0.5,
                                                 m_detector->nPixels().Y(),
                                                 -0.5,
                                                 m_detector->nPixels().Y() - 0.5,
                                                 0,
                                                 1);
    title = m_detector->getName() + " Global efficiency map;x [mm];y [mm];efficiency";
    hGlobalEfficiencyMap_trackPos = new TProfile2D("globalEfficiencyMap_trackPos",
                                                   title.c_str(),
                                                   300,
                                                   -1.5 * m_detector->getSize().X(),
                                                   1.5 * m_detector->getSize().X(),
                                                   300,
                                                   -1.5 * m_detector->getSize().Y(),
                                                   1.5 * m_detector->getSize().Y(),
                                                   0,
                                                   1);
    title = m_detector->getName() + " Chip efficiency map;x [px];y [px];efficiency";
    hChipEfficiencyMap_clustPos = new TProfile2D("chipEfficiencyMap_clustPos",
                                                 title.c_str(),
                                                 m_detector->nPixels().X(),
                                                 -0.5,
                                                 m_detector->nPixels().X() - 0.5,
                                                 m_detector->nPixels().Y(),
                                                 -0.5,
                                                 m_detector->nPixels().Y() - 0.5,
                                                 0,
                                                 1);
    title = m_detector->getName() + " Global efficiency map;x [mm];y [mm];efficiency";
    hGlobalEfficiencyMap_clustPos = new TProfile2D("globalEfficiencyMap_clustPos",
                                                   title.c_str(),
                                                   300,
                                                   -1.5 * m_detector->getSize().X(),
                                                   1.5 * m_detector->getSize().X(),
                                                   300,
                                                   -1.5 * m_detector->getSize().Y(),
                                                   1.5 * m_detector->getSize().Y(),
                                                   0,
                                                   1);
    hDistanceCluster_track = new TH2D("distanceTrack_Hit",
                                      "distance between track and hit; track_x - dut_x [mm]; track_y - dut_y [mm] ",
                                      150,
                                      -1.5 * m_detector->getPitch().x(),
                                      1.5 * m_detector->getPitch().x(),
                                      150,
                                      -1.5 * m_detector->getPitch().y(),
                                      1.5 * m_detector->getPitch().y());
    eTotalEfficiency =
        new TEfficiency("eTotalEfficiency", "totalEfficiency; axis has no meaning; total chip efficiency", 1, 0, 1);
    totalEfficiency = new TNamed("totalEffiency", "totalEffiency");

    hTrackTimeToPrevHit_matched =
        new TH1D("trackTimeToPrevHit_matched", "trackTimeToPrevHit_matched;time to prev hit [us];# events", 1e6, 0, 1e6);
    hTrackTimeToPrevHit_notmatched = new TH1D(
        "trackTimeToPrevHit_notmatched", "trackTimeToPrevHit_notmatched;time to prev hit [us];# events", 1e6, 0, 1e6);

    title = m_detector->getName() + "time difference to previous track (if this has assoc cluster)";
    hTimeDiffPrevTrack_assocCluster = new TH1D("timeDiffPrevTrack_assocCluster", title.c_str(), 11000, -1000, 10000);
    hTimeDiffPrevTrack_assocCluster->GetXaxis()->SetTitle("time diff [#mus]");
    hTimeDiffPrevTrack_assocCluster->GetYaxis()->SetTitle("events");
    title = m_detector->getName() + "time difference to previous track (if this has no assoc cluster)";
    hTimeDiffPrevTrack_noAssocCluster = new TH1D("timeDiffPrevTrack_noAssocCluster", title.c_str(), 11000, -1000, 10000);
    hTimeDiffPrevTrack_noAssocCluster->GetXaxis()->SetTitle("time diff [#mus]");
    hTimeDiffPrevTrack_noAssocCluster->GetYaxis()->SetTitle("events");

    hRowDiffPrevTrack_assocCluster =
        new TH1D("rowDiffPrevTrack_assocCluster",
                 "rowDiffPrevTrack_assocCluster; row difference (matched track to prev track) [px];# events",
                 2 * m_detector->nPixels().Y(),
                 -m_detector->nPixels().Y() - 0.5,
                 m_detector->nPixels().Y() - 0.5);

    hColDiffPrevTrack_assocCluster =
        new TH1D("colDiffPrevTrack_assocCluster",
                 "colDiffPrevTrack_assocCluster;column difference (matched track to prev track) [px];# events",
                 2 * m_detector->nPixels().X(),
                 -m_detector->nPixels().X() - 0.5,
                 m_detector->nPixels().X() - 0.5);

    hRowDiffPrevTrack_noAssocCluster =
        new TH1D("rowDiffPrevTrack_noAssocCluster",
                 "rowDiffPrevTrack_noAssocCluster;row difference (non-matched track - prev track) [px];# events",
                 2 * m_detector->nPixels().Y(),
                 -m_detector->nPixels().Y() - 0.5,
                 m_detector->nPixels().Y() - 0.5);

    hColDiffPrevTrack_noAssocCluster =
        new TH1D("colDiffPrevTrack_noAassocCluster",
                 "colDiffPrevTrack_noAssocCluster;column difference (non-matched track - prev track) [px];# events",
                 2 * m_detector->nPixels().X(),
                 -m_detector->nPixels().X() - 0.5,
                 m_detector->nPixels().X() - 0.5);

    hPosDiffPrevTrack_assocCluster = new TH2D("posDiffPrevTrack_assocCluster",
                                              "posDiffPrevTrack_assocCluster;column difference (matched track - prev track) "
                                              "[px];row difference (matched track - prev track) [px];# events",
                                              2 * m_detector->nPixels().X(),
                                              -m_detector->nPixels().X() - 0.5,
                                              m_detector->nPixels().X() - 0.5,
                                              2 * m_detector->nPixels().Y(),
                                              -m_detector->nPixels().Y() - 0.5,
                                              m_detector->nPixels().Y() - 0.5);

    hPosDiffPrevTrack_noAssocCluster =
        new TH2D("posDiffPrevTrack_noAssocCluster",
                 "posDiffPrevTrack_noAssocCluster;column difference (non-matched track - prev track) [px];row difference "
                 "(non-matched track - prev track) [px];# events",
                 2 * m_detector->nPixels().X(),
                 -m_detector->nPixels().X() - 0.5,
                 m_detector->nPixels().X() - 0.5,
                 2 * m_detector->nPixels().Y(),
                 -m_detector->nPixels().Y() - 0.5,
                 m_detector->nPixels().Y() - 0.5);

    // initialize matrix with hit timestamps to all 0:
    auto nRows = static_cast<size_t>(m_detector->nPixels().Y());
    auto nCols = static_cast<size_t>(m_detector->nPixels().X());
    std::vector<double> v_row(nRows, 0.); // create vector will zeros of length <nRows>
    prev_hit_ts.assign(nCols, v_row);     // use vector v_row to construct matrix
}

StatusCode AnalysisEfficiency::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();

    // Loop over all tracks
    for(auto& track : tracks) {
        n_track++;
        bool has_associated_cluster = false;
        bool is_within_roi = true;
        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > m_chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            n_chi2++;
            continue;
        }

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        LOG(TRACE) << " Checking if track is outside DUT area";
        if(!m_detector->hasIntercept(track.get(), 1)) {
            LOG(DEBUG) << " - track outside DUT area: " << localIntercept;
            n_dut++;
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        LOG(TRACE) << " Checking if track is outside ROI";
        if(!m_detector->isWithinROI(track.get())) {
            LOG(DEBUG) << " - track outside ROI";
            n_roi++;
            is_within_roi = false;
            // here we don't continue because only some particular histograms shall be effected
        }

        // Check that it doesn't go through/near a masked pixel
        LOG(TRACE) << " Checking if track is close to masked pixel";
        if(m_detector->hitMasked(track.get(), 1.)) {
            n_masked++;
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }

        // Get the event:
        auto event = clipboard->getEvent();

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

        // Get the DUT clusters from the clipboard, that are assigned to the track
        auto associated_clusters = track->getAssociatedClusters(m_detector->getName());
        if(associated_clusters.size() > 0) {
            auto cluster = track->getClosestCluster(m_detector->getName());
            has_associated_cluster = true;
            matched_tracks++;
            auto clusterLocal = m_detector->globalToLocal(cluster->global());
            hDistanceCluster_track->Fill(localIntercept.x() - clusterLocal.x(), localIntercept.y() - clusterLocal.y());
            hGlobalEfficiencyMap_clustPos->Fill(cluster->global().x(), cluster->global().y(), has_associated_cluster);
            hChipEfficiencyMap_clustPos->Fill(
                m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal), has_associated_cluster);
        }

        hGlobalEfficiencyMap_trackPos->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
        hChipEfficiencyMap_trackPos->Fill(
            m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);
        // For pixels, only look at the ROI:
        if(is_within_roi) {
            hPixelEfficiencyMap_trackPos->Fill(xmod, ymod, has_associated_cluster);
            eTotalEfficiency->Fill(has_associated_cluster, 0); // use 0th bin for total efficiency
        }

        auto intercept_col = static_cast<size_t>(m_detector->getColumn(localIntercept));
        auto intercept_row = static_cast<size_t>(m_detector->getRow(localIntercept));

        if(has_associated_cluster) {
            hTimeDiffPrevTrack_assocCluster->Fill(
                static_cast<double>(Units::convert(track->timestamp() - last_track_timestamp, "us")));
            hRowDiffPrevTrack_assocCluster->Fill(m_detector->getRow(localIntercept) - last_track_row);
            hColDiffPrevTrack_assocCluster->Fill(m_detector->getColumn(localIntercept) - last_track_col);
            hPosDiffPrevTrack_assocCluster->Fill(m_detector->getColumn(localIntercept) - last_track_col,
                                                 m_detector->getRow(localIntercept) - last_track_row);
            if((prev_hit_ts.at(intercept_col)).at(intercept_row) != 0) {
                hTrackTimeToPrevHit_matched->Fill(static_cast<double>(
                    Units::convert(track->timestamp() - prev_hit_ts.at(intercept_col).at(intercept_row), "us")));
            }
        } else {
            hGlobalEfficiencyMap_clustPos->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
            hChipEfficiencyMap_clustPos->Fill(
                m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);

            hTimeDiffPrevTrack_noAssocCluster->Fill(
                static_cast<double>(Units::convert(track->timestamp() - last_track_timestamp, "us")));
            hRowDiffPrevTrack_noAssocCluster->Fill(m_detector->getRow(localIntercept) - last_track_row);
            hColDiffPrevTrack_noAssocCluster->Fill(m_detector->getColumn(localIntercept) - last_track_col);
            hPosDiffPrevTrack_noAssocCluster->Fill(m_detector->getColumn(localIntercept) - last_track_col,
                                                   m_detector->getRow(localIntercept) - last_track_row);
            if((prev_hit_ts.at(intercept_col)).at(intercept_row) != 0) {
                LOG(DEBUG) << "Found a time difference of "
                           << Units::display(track->timestamp() - prev_hit_ts.at(intercept_col).at(intercept_row), "us");
                hTrackTimeToPrevHit_notmatched->Fill(static_cast<double>(
                    Units::convert(track->timestamp() - prev_hit_ts.at(intercept_col).at(intercept_row), "us")));
            }
        }
        last_track_timestamp = track->timestamp();
        last_track_col = m_detector->getColumn(localIntercept);
        last_track_row = m_detector->getRow(localIntercept);
    } // end loop over tracks

    // Before going to the next event, loop over all pixels (all hits incl. noise)
    // and fill matrix with timestamps of previous pixels.
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    if(pixels.empty()) {
        LOG(DEBUG) << "Detector " << m_detector->getName() << " does not have any pixels on the clipboard";
    }

    for(auto& pixel : pixels) {
        if(pixel->column() > m_detector->nPixels().X() || pixel->row() > m_detector->nPixels().Y()) {
            continue;
        }
        prev_hit_ts.at(static_cast<size_t>(pixel->column())).at(static_cast<size_t>(pixel->row())) = pixel->timestamp();
    }

    return StatusCode::Success;
}

void AnalysisEfficiency::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    // Track selection flow:
    LOG(STATUS) << "Track selection flow:       " << n_track << std::endl
                << "* rejected by chi2          -" << n_chi2 << std::endl
                << "* track outside ROI         -" << n_roi << std::endl
                << "* track outside DUT         -" << n_dut << std::endl
                << "* track close to masked px  -" << n_masked << std::endl
                << "Accepted tracks:            " << total_tracks;

    double totalEff = 100 * static_cast<double>(matched_tracks) / (total_tracks > 0 ? total_tracks : 1);
    LOG(STATUS) << "Total efficiency of detector " << m_detector->getName() << ": " << totalEff << "%, measured with "
                << matched_tracks << "/" << total_tracks << " matched/total tracks";

    totalEfficiency->SetName((to_string(totalEff) + " %").c_str());
    totalEfficiency->Write();

    for(int icol = 1; icol < m_detector->nPixels().X() + 1; icol++) {
        for(int irow = 1; irow < m_detector->nPixels().Y() + 1; irow++) {
            // calculate total efficiency: (just to double check the other calculation)
            double eff = hChipEfficiencyMap_trackPos->GetBinContent(icol, irow);
            if(eff > 0) {
                LOG(TRACE) << "col/row = " << icol << "/" << irow << ", binContent = " << eff;
                hPixelEfficiency->Fill(hChipEfficiencyMap_trackPos->GetBinContent(icol, irow));
            }
        }
    }
}
