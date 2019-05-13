/**
 * @file
 * @brief Implementation of [AnalysisEfficiency] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisTimingATLASpix.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

#include "TF1.h"
#include "TFile.h"

using namespace corryvreckan;

AnalysisTimingATLASpix::AnalysisTimingATLASpix(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector) {

    using namespace ROOT::Math;
    m_detector = detector;
    m_timingCut = m_config.get<double>("timing_cut", static_cast<double>(Units::convert(1000, "ns")));
    m_chi2ndofCut = m_config.get<double>("chi2_ndof_cut", 3.);
    m_timeCutFrameEdge = m_config.get<double>("time_cut_frame_edge", static_cast<double>(Units::convert(20, "ns")));
    m_clusterTotCut = m_config.get<double>("cluster_tot_cut", static_cast<double>(Units::convert(500, "ns")));
    m_clusterSizeCut = m_config.get<size_t>("cluster_size_cut", static_cast<size_t>(100));
    m_highTotCut = m_config.get<int>("high_tot_cut", 40);
    m_leftTailCut = m_config.get<double>("left_tail_cut", static_cast<double>(Units::convert(-10, "ns")));

    if(m_config.has("correction_file_row")) {
        m_correctionFile_row = m_config.get<std::string>("correction_file_row");
        m_correctionGraph_row = m_config.get<std::string>("correction_graph_row");
        m_pointwise_correction_row = true;
    } else {
        m_pointwise_correction_row = false;
    }
    if(m_config.has("correction_file_timewalk")) {
        m_correctionFile_timewalk = m_config.get<std::string>("correction_file_timewalk");
        m_correctionGraph_timewalk = m_config.get<std::string>("correction_graph_timewalk");
        m_pointwise_correction_timewalk = true;
    } else {
        m_pointwise_correction_timewalk = false;
    }

    m_calcCorrections = m_config.get<bool>("calc_corrections", false);
    m_totBinExample = m_config.get<int>("tot_bin_example", 3);

    total_tracks_uncut = 0;
    tracks_afterChi2Cut = 0;
    tracks_hasIntercept = 0;
    tracks_isWithinROI = 0;
    tracks_afterMasking = 0;
    total_tracks = 0;
    matched_tracks = 0;
    tracks_afterClusterTotCut = 0;
    tracks_afterClusterSizeCut = 0;
}

void AnalysisTimingATLASpix::initialise() {

    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));

    std::string name = "hTrackCorrelationTime";
    hTrackCorrelationTime =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime->GetXaxis()->SetTitle("Track time stamp - cluster time stamp [ns]");
    hTrackCorrelationTime->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTimeAssoc";
    hTrackCorrelationTimeAssoc =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTimeAssoc->GetXaxis()->SetTitle("track time stamp - cluster time stamp [ns]");
    hTrackCorrelationTimeAssoc->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_rowCorr";
    std::string title = "hTrackCorrelationTime_rowCorr: row-by-row correction";
    hTrackCorrelationTime_rowCorr =
        new TH1F(name.c_str(), title.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime_rowCorr->GetXaxis()->SetTitle("track time stamp - cluster time stamp [ns]");
    hTrackCorrelationTime_rowCorr->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_rowAndTimeWalkCorr";
    hTrackCorrelationTime_rowAndTimeWalkCorr =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime_rowAndTimeWalkCorr->GetXaxis()->SetTitle("track time stamp - cluster time stamp [ns]");
    hTrackCorrelationTime_rowAndTimeWalkCorr->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_rowAndTimeWalkCorr_l25";
    hTrackCorrelationTime_rowAndTimeWalkCorr_l25 =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime_rowAndTimeWalkCorr_l25->GetXaxis()->SetTitle(
        "track time stamp - cluster time stamp [ns] (if seed tot < 25lsb)");
    hTrackCorrelationTime_rowAndTimeWalkCorr_l25->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_rowAndTimeWalkCorr_l40";
    hTrackCorrelationTime_rowAndTimeWalkCorr_l40 =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime_rowAndTimeWalkCorr_l40->GetXaxis()->SetTitle(
        "track time stamp - cluster time stamp [ns] (if seed tot < 40lsb)");
    hTrackCorrelationTime_rowAndTimeWalkCorr_l40->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_rowAndTimeWalkCorr_g40";
    hTrackCorrelationTime_rowAndTimeWalkCorr_g40 =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timingCut), -1 * m_timingCut, m_timingCut);
    hTrackCorrelationTime_rowAndTimeWalkCorr_g40->GetXaxis()->SetTitle(
        "track time stamp - cluster time stamp [ns] (if seed tot > 40lsb)");
    hTrackCorrelationTime_rowAndTimeWalkCorr_g40->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTime_totBin_" + std::to_string(m_totBinExample);
    hTrackCorrelationTime_example = new TH1D(name.c_str(), name.c_str(), 20000, -5000, 5000);
    hTrackCorrelationTime_example->GetXaxis()->SetTitle(
        "track time stamp - pixel time stamp [ns] (all pixels from cluster)");
    hTrackCorrelationTime_example->GetYaxis()->SetTitle("# events");

    // 2D histograms:
    // column dependence
    name = "hTrackCorrelationTimeVsCol";
    hTrackCorrelationTimeVsCol =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().X(), 0, m_detector->nPixels().X());
    hTrackCorrelationTimeVsCol->GetYaxis()->SetTitle("pixel column");
    hTrackCorrelationTimeVsCol->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");
    // row dependence
    name = "hTrackCorrelationTimeVsRow";
    hTrackCorrelationTimeVsRow =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hTrackCorrelationTimeVsRow->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");
    name = "hTrackCorrelationTimeVsRow_1px";
    hTrackCorrelationTimeVsRow_1px =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hTrackCorrelationTimeVsRow_1px->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow_1px->GetXaxis()->SetTitle(
        "track time stamp - seed pixel time stamp [ns] (single-pixel clusters)");
    name = "hTrackCorrelationTimeVsRow_npx";
    hTrackCorrelationTimeVsRow_npx =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hTrackCorrelationTimeVsRow_npx->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow_npx->GetXaxis()->SetTitle(
        "track time stamp - seed pixel time stamp [ns] (multi-pixel clusters)");

    // control plot: row dependence after row correction
    name = "hTrackCorrelationTimeVsRow_rowCorr";
    hTrackCorrelationTimeVsRow_rowCorr =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hTrackCorrelationTimeVsRow_rowCorr->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow_rowCorr->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    // control plot: time walk dependence, not row corrected
    name = "hTrackCorrelationTimeVsTot";
    hTrackCorrelationTimeVsTot = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot->GetYaxis()->SetTitle("pixel ToT [ns]");
    hTrackCorrelationTimeVsTot->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hTrackCorrelationTimeVsTot_1px";
    hTrackCorrelationTimeVsTot_1px = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_1px->GetYaxis()->SetTitle("seed pixel ToT [ns] (if clustersize = 1)");
    hTrackCorrelationTimeVsTot_1px->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hTrackCorrelationTimeVsTot_npx";
    hTrackCorrelationTimeVsTot_npx = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_npx->GetYaxis()->SetTitle("seed pixel ToT [ns] (if clustersize > 1)");
    hTrackCorrelationTimeVsTot_npx->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hClusterTimeMinusPixelTime";
    hClusterTimeMinusPixelTime = new TH1F(name.c_str(), name.c_str(), 2000, -1000, 1000);
    hClusterTimeMinusPixelTime->GetXaxis()->SetTitle(
        "cluster timestamp - pixel timestamp [ns] (all pixels from cluster (if clusterSize>1))");

    // timewalk after row correction
    name = "hTrackCorrelationTimeVsTot_rowCorr";
    hTrackCorrelationTimeVsTot_rowCorr = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_rowCorr->GetYaxis()->SetTitle("pixel ToT [ns]");
    hTrackCorrelationTimeVsTot_rowCorr->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hTrackCorrelationTimeVsTot_rowCorr_1px";
    hTrackCorrelationTimeVsTot_rowCorr_1px = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_rowCorr_1px->GetYaxis()->SetTitle("pixel ToT [ns] (single-pixel clusters)");
    hTrackCorrelationTimeVsTot_rowCorr_1px->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hTrackCorrelationTimeVsTot_rowCorr_npx";
    hTrackCorrelationTimeVsTot_rowCorr_npx = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_rowCorr_npx->GetYaxis()->SetTitle("pixel ToT [ns] (multi-pixel clusters)");
    hTrackCorrelationTimeVsTot_rowCorr_npx->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    // final plots with both row and timewalk correction:
    name = "hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr";
    hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr =
        new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr->GetYaxis()->SetTitle("row");
    hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr";
    hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 512, 0, 512);
    hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr->GetYaxis()->SetTitle("pixel ToT [ns]");
    hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr->GetXaxis()->SetTitle("track time stamp - seed pixel time stamp [ns]");

    name = "hClusterSizeVsTot_Assoc";
    hClusterSizeVsTot_Assoc = new TH2F(name.c_str(), name.c_str(), 20, 0, 20, 512, 0, 512);
    hClusterSizeVsTot_Assoc->GetYaxis()->SetTitle("pixel ToT [ns] (all pixels from cluster)");
    hClusterSizeVsTot_Assoc->GetXaxis()->SetTitle("clusterSize");

    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "hitMapAssoc; x_{track} [px]; x_{track} [px]; # entries",
                            m_detector->nPixels().X(),
                            0,
                            m_detector->nPixels().X(),
                            m_detector->nPixels().Y(),
                            0,
                            m_detector->nPixels().Y());
    hHitMapAssoc_highTot = new TH2F("hitMapAssoc_highTot",
                                    "hitMapAssoc_highTot; x_{track} [px]; x_{track} [px]; # entries",
                                    m_detector->nPixels().X(),
                                    0,
                                    m_detector->nPixels().X(),
                                    m_detector->nPixels().Y(),
                                    0,
                                    m_detector->nPixels().Y());
    hHitMapAssoc_inPixel = new TH2F("hitMapAssoc_inPixel",
                                    "hitMapAssoc_inPixel; x_{track} mod 130 #mum; y_{track} mod 130 #mum",
                                    static_cast<int>(pitch_x),
                                    0,
                                    pitch_x,
                                    static_cast<int>(pitch_y),
                                    0,
                                    pitch_y);
    hHitMapAssoc_inPixel_highTot = new TH2F("hitMapAssoc_inPixel_highTot",
                                            "hitMapAssoc_inPixel_highTot;  x_{track} mod 130 #mum; y_{track} mod 130 #mum",
                                            static_cast<int>(pitch_x),
                                            0,
                                            pitch_x,
                                            static_cast<int>(pitch_y),
                                            0,
                                            pitch_y);
    hClusterMapAssoc = new TH2F("hClusterMapAssoc",
                                "hClusterMapAssoc; x_{cluster} [px]; x_{cluster} [px]; # entries",
                                m_detector->nPixels().X(),
                                0,
                                m_detector->nPixels().X(),
                                m_detector->nPixels().Y(),
                                0,
                                m_detector->nPixels().Y());

    hTotVsTime_low = new TH2F("hTotVsTime_low", "hTotVsTime_low", 64, 0, 64, 1e6, 0, 100);
    hTotVsTime_low->GetXaxis()->SetTitle("pixel ToT [lsb]");
    hTotVsTime_low->GetYaxis()->SetTitle("time [s]");
    hTotVsTime_high = new TH2F("hTotVsTime_high", "hTotVsTime_high", 64, 0, 64, 1e6, 0, 100);
    hTotVsTime_high->GetXaxis()->SetTitle("pixel ToT [lsb] if > high_tot_cut");
    hTotVsTime_high->GetYaxis()->SetTitle("time [s]");

    // control plots for "left tail":
    hClusterMap_leftTail = new TH2F("hClusterMap_leftTail",
                                    "hClusterMap_leftTail; x_{cluster} [px]; x_{cluster} [px]; # entries",
                                    m_detector->nPixels().X(),
                                    0,
                                    m_detector->nPixels().X(),
                                    m_detector->nPixels().Y(),
                                    0,
                                    m_detector->nPixels().Y());
    hTot_leftTail = new TH1F("hTot_leftTail", "hTot_leftTail; pixel ToT [lsb]; # events", 2 * 64, -64, 64);
    hPixelTimestamp_leftTail =
        new TH1F("pixelTS1_leftTail", "pixelTimestamp_leftTail; pixel timestamp [ns]; # entries", 2050, 0, 2050);
    hClusterSize_leftTail = new TH1F("clusterSize_leftTail", "clusterSize_leftTail; cluster size; # entries", 100, 0, 100);
    // right tail = main distribution
    hClusterMap_mainpeak = new TH2F("hClusterMap_mainpeak",
                                    "hClusterMap_mainpeak; x_{cluster} [px]; x_{cluster} [px]; # entries",
                                    m_detector->nPixels().X(),
                                    0,
                                    m_detector->nPixels().X(),
                                    m_detector->nPixels().Y(),
                                    0,
                                    m_detector->nPixels().Y());
    hTot_mainpeak = new TH1F("hTot_mainpeak", "hTot_mainpeak", 2 * 64, -64, 64);
    hPixelTimestamp_mainpeak =
        new TH1F("pixelTimestamp_mainpeak", "pixelTimestamp_mainpeak; pixel timestamp [ns]; # entries", 2050, 0, 2050);
    hClusterSize_mainpeak = new TH1F("clusterSize_mainpeak", "clusterSize_mainpeak; cluster size; # entries", 100, 0, 100);

    // /////////////////////////////////////////// //
    // TGraphErrors for Timewalk & Row Correction: //
    // /////////////////////////////////////////// //

    gTimeCorrelationVsRow = new TGraphErrors();
    gTimeCorrelationVsRow->SetName("gTimeCorrelationVsRow");
    gTimeCorrelationVsRow->SetTitle("gTimeCorrelationVsRow");
    gTimeCorrelationVsRow->GetXaxis()->SetTitle("row");
    gTimeCorrelationVsRow->GetYaxis()->SetTitle("time correlation peak [ns]");

    // !!!!also fix these:!!!!
    int nBinsToT = hTrackCorrelationTimeVsTot_rowCorr->GetNbinsY();
    gTimeCorrelationVsTot_rowCorr = new TGraphErrors(nBinsToT);
    gTimeCorrelationVsTot_rowCorr->SetName("gTimeCorrelationVsTot");
    gTimeCorrelationVsTot_rowCorr->SetTitle("gTimeCorrelationVsTot");
    gTimeCorrelationVsTot_rowCorr->GetXaxis()->SetTitle("pixel ToT [ns]");
    gTimeCorrelationVsTot_rowCorr->GetYaxis()->SetTitle("time correlation peak [ns]");

    nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_1px->GetNbinsY();
    gTimeCorrelationVsTot_rowCorr_1px = new TGraphErrors(nBinsToT);
    gTimeCorrelationVsTot_rowCorr_1px->SetName("gTimeCorrelationVsTot_1px");
    gTimeCorrelationVsTot_rowCorr_1px->SetTitle("gTimeCorrelationVsTot_1px");
    gTimeCorrelationVsTot_rowCorr_1px->GetXaxis()->SetTitle("pixel ToT [ns] (single-pixel clusters)");
    gTimeCorrelationVsTot_rowCorr_1px->GetYaxis()->SetTitle("time correlation peak [ns]");

    nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_npx->GetNbinsY();
    gTimeCorrelationVsTot_rowCorr_npx = new TGraphErrors(nBinsToT);
    gTimeCorrelationVsTot_rowCorr_npx->SetName("gTimeCorrelationVsTot_npx");
    gTimeCorrelationVsTot_rowCorr_npx->SetTitle("gTimeCorrelationVsTot_npx");
    gTimeCorrelationVsTot_rowCorr_npx->GetXaxis()->SetTitle("pixel ToT [ns] (multi-pixel clusters");
    gTimeCorrelationVsTot_rowCorr_npx->GetYaxis()->SetTitle("time correlation peak [ns]");

    LOG(INFO) << "calcCorrections = " << m_calcCorrections;

    if(m_pointwise_correction_row) {
        // Import TGraphErrors for row corection:
        std::string fileName = m_correctionFile_row;
        TFile* file = TFile::Open(fileName.c_str());
        if(!file->IsOpen()) {
            LOG(ERROR) << "Cannot open " << fileName << "!\n";
        }
        gRowCorr = static_cast<TGraphErrors*>(file->Get(m_correctionGraph_row.c_str()));
        file->Close();
        delete file;
    } else {
        LOG(STATUS) << "----> NO POINTWISE ROW CORRECTION!!!";
    }
    if(m_pointwise_correction_timewalk) {
        // Import TGraphErrors for timewalk corection:
        std::string fileName = m_correctionFile_timewalk;
        TFile* file = TFile::Open(fileName.c_str());
        if(!file->IsOpen()) {
            LOG(ERROR) << "Cannot open " << fileName << "!\n";
        }
        gTimeWalkCorr = static_cast<TGraphErrors*>(file->Get(m_correctionGraph_timewalk.c_str()));
        file->Close();
        delete file;
    } else {
        LOG(STATUS) << "----> NO POINTWISE TIMEWALK CORRECTION!!!";
    }
}

StatusCode AnalysisTimingATLASpix::run(std::shared_ptr<Clipboard> clipboard) {

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
        total_tracks_uncut++;

        // Cut on the chi2/ndof
        if(track->chi2ndof() > m_chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            continue;
        }
        tracks_afterChi2Cut++;

        // Check if it intercepts the DUT
        if(!m_detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
            continue;
        }
        tracks_hasIntercept++;

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track)) {
            LOG(DEBUG) << " - track outside ROI";
            is_within_roi = false;
        }
        tracks_isWithinROI++;

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track, 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }
        tracks_afterMasking++;

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
            // Early edge - eventStart points to the start of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            continue;
        }

        // Count this as reference track:
        total_tracks++;

        // Get the DUT clusters from the clipboard
        Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(m_detector->name(), "clusters"));
        if(clusters == nullptr) {
            LOG(DEBUG) << " - no DUT clusters";
        } else {

            // Loop over all DUT clusters to find matches:
            for(auto* cluster : (*clusters)) {
                LOG(DEBUG) << " - Looking at next DUT cluster";

                hTrackCorrelationTime->Fill(track->timestamp() - cluster->timestamp());

                auto associated_clusters = track->associatedClusters();
                if(std::find(associated_clusters.begin(), associated_clusters.end(), cluster) != associated_clusters.end()) {
                    LOG(DEBUG) << "Found associated cluster " << (*cluster);
                    has_associated_cluster = true;
                    matched_tracks++;

                    //                    // UGLY HACK:
                    //                    if(cluster->row() > 390) {
                    //                        continue;
                    //                    }

                    // clusterTotCut:
                    if(cluster->tot() > m_clusterTotCut) {
                        LOG(DEBUG) << " - track discarded due to clusterTotCut";
                        continue;
                    }
                    tracks_afterClusterTotCut++;

                    if(cluster->size() > m_clusterSizeCut) {
                        LOG(DEBUG) << " - track discarded due to clusterSizeCut";
                        continue;
                    }
                    tracks_afterClusterSizeCut++;

                    double timeDiff = track->timestamp() - cluster->timestamp();
                    hTrackCorrelationTimeAssoc->Fill(timeDiff);

                    hTrackCorrelationTimeVsTot->Fill(timeDiff, cluster->getSeedPixel()->tot());
                    if(cluster->size() == 1) {
                        hTrackCorrelationTimeVsTot_1px->Fill(timeDiff, cluster->getSeedPixel()->tot());
                    } else {
                        hTrackCorrelationTimeVsTot_npx->Fill(timeDiff, cluster->getSeedPixel()->tot());
                    }
                    hTrackCorrelationTimeVsCol->Fill(timeDiff, cluster->getSeedPixel()->column());
                    hTrackCorrelationTimeVsRow->Fill(timeDiff, cluster->getSeedPixel()->row());

                    // Calculate in-pixel position of track in microns
                    auto globalIntercept = m_detector->getIntercept(track);
                    auto localIntercept = m_detector->globalToLocal(globalIntercept);
                    auto inpixel = m_detector->inPixel(localIntercept);
                    auto xmod = static_cast<double>(Units::convert(inpixel.X(), "um"));
                    auto ymod = static_cast<double>(Units::convert(inpixel.Y(), "um"));
                    hHitMapAssoc_inPixel->Fill(xmod, ymod);
                    if(cluster->tot() > m_highTotCut && cluster->size() == 1) {
                        hHitMapAssoc_inPixel_highTot->Fill(xmod, ymod);
                    }

                    // 2D histograms: --> fill for all pixels from cluster
                    for(auto& pixel : (*cluster->pixels())) {

                        // to check that cluster timestamp = earliest pixel timestamp
                        if(cluster->size() > 1) {
                            hClusterTimeMinusPixelTime->Fill(cluster->timestamp() - pixel->timestamp());
                        }

                        hClusterSizeVsTot_Assoc->Fill(static_cast<double>(cluster->size()), pixel->tot());
                        hHitMapAssoc->Fill(pixel->column(), pixel->row());
                        hTotVsTime_low->Fill(pixel->tot(), static_cast<double>(Units::convert(pixel->timestamp(), "s")));
                        if(pixel->tot() > m_highTotCut) {
                            hHitMapAssoc_highTot->Fill(pixel->column(), pixel->row());
                            hTotVsTime_high->Fill(pixel->tot(),
                                                  static_cast<double>(Units::convert(pixel->timestamp(), "s")));
                        }
                    }
                    hClusterMapAssoc->Fill(cluster->column(), cluster->row());

                    // !!! Have to do this in the end because it changes the cluster time and position!!!
                    // row-by-row correction using points from TGraphError directly instead of fit.

                    // point-wise correction:
                    if(m_pointwise_correction_row) {
                        correctClusterTimestamp(cluster, 0); // mode=0 --> row correction
                        hTrackCorrelationTime_rowCorr->Fill(track->timestamp() - cluster->timestamp());
                        // for(auto& pixel : (*cluster->pixels())) {
                        hTrackCorrelationTimeVsRow_rowCorr->Fill(track->timestamp() - cluster->getSeedPixel()->timestamp(),
                                                                 cluster->getSeedPixel()->row());
                        hTrackCorrelationTimeVsTot_rowCorr->Fill(track->timestamp() - cluster->getSeedPixel()->timestamp(),
                                                                 cluster->getSeedPixel()->tot());
                        if(cluster->size() == 1) {
                            hTrackCorrelationTimeVsTot_rowCorr_1px->Fill(
                                track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->tot());
                        }
                        if(cluster->size() > 1) {
                            hTrackCorrelationTimeVsTot_rowCorr_npx->Fill(
                                track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->tot());
                        }
                        //} for(auto& pixels : ...)
                    }
                    // point-wise timewalk correction on top:
                    if(m_pointwise_correction_timewalk) {
                        correctClusterTimestamp(cluster, 1); // mode=1 --> timewalk correction
                        hTrackCorrelationTime_rowAndTimeWalkCorr->Fill(track->timestamp() - cluster->timestamp());
                        if(cluster->getSeedPixel()->tot() < 25) {
                            hTrackCorrelationTime_rowAndTimeWalkCorr_l25->Fill(track->timestamp() - cluster->timestamp());
                        }
                        if(cluster->getSeedPixel()->tot() < 40) {
                            hTrackCorrelationTime_rowAndTimeWalkCorr_l40->Fill(track->timestamp() - cluster->timestamp());
                        }
                        if(cluster->getSeedPixel()->tot() > 40) {
                            hTrackCorrelationTime_rowAndTimeWalkCorr_g40->Fill(track->timestamp() - cluster->timestamp());
                        }

                        hTrackCorrelationTimeVsRow_rowAndTimeWalkCorr->Fill(
                            track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->row());
                        hTrackCorrelationTimeVsTot_rowAndTimeWalkCorr->Fill(
                            track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->tot());

                        // control plots to investigate "left tail" in time correlation:
                        if(track->timestamp() - cluster->timestamp() < m_leftTailCut) {
                            hClusterMap_leftTail->Fill(cluster->column(), cluster->row());
                            hTot_leftTail->Fill(cluster->getSeedPixel()->tot());
                            hPixelTimestamp_leftTail->Fill(cluster->getSeedPixel()->timestamp());
                            hClusterSize_leftTail->Fill(static_cast<double>(cluster->size()));
                        }
                        if(track->timestamp() - cluster->timestamp() > m_leftTailCut) {
                            hClusterMap_mainpeak->Fill(cluster->column(), cluster->row());
                            hTot_mainpeak->Fill(cluster->getSeedPixel()->tot());
                            hPixelTimestamp_mainpeak->Fill(cluster->getSeedPixel()->timestamp());
                            hClusterSize_mainpeak->Fill(static_cast<double>(cluster->size()));
                        }
                    }
                }

            } // for loop over all clusters
        }     // else (clusters != nullptr)

        LOG(DEBUG) << "is_within_roi = " << is_within_roi;
        LOG(DEBUG) << "has_associated_cluster = " << has_associated_cluster;

    } // for loop over all tracks

    return StatusCode::Success;
}

void AnalysisTimingATLASpix::finalise() {
    LOG(STATUS) << "Timing analysis finished for detector " << m_detector->name() << ": ";

    if(m_calcCorrections) {

        /// ROW CORRECTION ///
        std::string fitOption = "q"; // set to "" for terminal output
        int binMax = 0;
        double timePeak = 0.;
        double timePeakErr = 0.;
        int nRows = hTrackCorrelationTimeVsRow->GetNbinsY();

        for(int iBin = 0; iBin < nRows; iBin++) {
            TH1D* hTemp = hTrackCorrelationTimeVsRow->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

            if(hTemp->GetEntries() < 500) { // too few entries to fit
                                            // if(hTemp->GetEntries() < 100) { // too few entries to fit
                delete hTemp;
                timePeak = 0;
                timePeakErr = 0;
                continue;
            } else {
                binMax = hTemp->GetMaximumBin();
                timePeak = hTemp->GetXaxis()->GetBinCenter(binMax);

                // fitting a Gaus for a good estimate of the peak positon:
                // NOTE: initial values for Gaussian are hard-coded at the moment!
                TF1* fPeak = new TF1("fPeak", "gaus");
                fPeak->SetParameters(1, 100, 45);
                double timeInt = 50;
                hTemp->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
                fPeak = hTemp->GetFunction("fPeak");
                timePeak = fPeak->GetParameter(1);
                timePeakErr = fPeak->GetParError(1);
                delete fPeak;
                delete hTemp;
            }
            // TGraphErrors should only have as many bins as it has sensible entries
            // (If it has multiple x=0 entries, the Spline interpolation will fail.
            int nBins = gTimeCorrelationVsRow->GetN();
            LOG(STATUS) << "nBins = " << nBins << ", x = " << iBin << ", y = " << timePeak;
            gTimeCorrelationVsRow->SetPoint(nBins, iBin, timePeak);
            gTimeCorrelationVsRow->SetPointError(nBins, 0., timePeakErr);

        } // for(iBin)

        /// TIME WALK CORRECTION on top of ROW CORRECTION: ///
        fitOption = "q"; // set to "" if you want terminal output
        binMax = 0;
        timePeak = 0.;
        timePeakErr = 0.;
        int nBinsToT = hTrackCorrelationTimeVsTot_rowCorr->GetNbinsY();
        LOG(DEBUG) << "nBinsToT = " << nBinsToT;

        for(int iBin = 0; iBin < nBinsToT; iBin++) {
            TH1D* hTemp = hTrackCorrelationTimeVsTot_rowCorr->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

            // if(hTemp->GetEntries() < 500) { // too few entries to fit
            if(hTemp->GetEntries() < 1000) { // too few entries to fit
                delete hTemp;
                timePeak = 0;
                timePeakErr = 0;
                continue;
            } else {
                binMax = hTemp->GetMaximumBin();
                timePeak = hTemp->GetXaxis()->GetBinCenter(binMax);

                // fitting a Gaus for a good estimate of the peak positon:
                // initial parameters are hardcoded at the moment!
                TF1* fPeak = new TF1("fPeak", "gaus");
                fPeak->SetParameters(1, 100, 45);
                double timeInt = 50;
                hTemp->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
                fPeak = hTemp->GetFunction("fPeak");
                timePeak = fPeak->GetParameter(1);
                timePeakErr = fPeak->GetParError(1);

                delete fPeak;
                delete hTemp;
            }
            gTimeCorrelationVsTot_rowCorr->SetPoint(iBin, iBin, timePeak);
            gTimeCorrelationVsTot_rowCorr->SetPointError(iBin, 0, timePeakErr);

        } // for(iBin)

        // SAME FOR SINGLE-PIXEL CLUSTERS:
        nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_1px->GetNbinsY();
        for(int iBin = 0; iBin < nBinsToT; iBin++) {
            TH1D* hTemp = hTrackCorrelationTimeVsTot_rowCorr_1px->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

            // if(hTemp->GetEntries() < 500) { // too few entries to fit
            if(hTemp->GetEntries() < 1000) { // too few entries to fit
                delete hTemp;
                timePeak = 0;
                timePeakErr = 0;
                continue;
            } else {
                binMax = hTemp->GetMaximumBin();
                timePeak = hTemp->GetXaxis()->GetBinCenter(binMax);

                // fitting a Gaus for a good estimate of the peak positon:
                // initial parameters are hardcoded at the moment!
                TF1* fPeak = new TF1("fPeak", "gaus");
                fPeak->SetParameters(1, 100, 45);
                double timeInt = 50;
                hTemp->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
                fPeak = hTemp->GetFunction("fPeak");
                timePeak = fPeak->GetParameter(1);
                timePeakErr = fPeak->GetParError(1);

                delete fPeak;
                delete hTemp;
            }
            gTimeCorrelationVsTot_rowCorr_1px->SetPoint(iBin, iBin, timePeak);
            gTimeCorrelationVsTot_rowCorr_1px->SetPointError(iBin, 0, timePeakErr);
        } // for(iBin)

        // SAME FOR MULTI-PIXEL CLUSTERS:
        nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_npx->GetNbinsY();
        for(int iBin = 0; iBin < nBinsToT; iBin++) {
            TH1D* hTemp = hTrackCorrelationTimeVsTot_rowCorr_npx->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

            // if(hTemp->GetEntries() < 500) { // too few entries to fit
            if(hTemp->GetEntries() < 1000) { // too few entries to fit
                delete hTemp;
                timePeak = 0;
                timePeakErr = 0;
                continue;
            } else {
                binMax = hTemp->GetMaximumBin();
                timePeak = hTemp->GetXaxis()->GetBinCenter(binMax);

                // fitting a Gaus for a good estimate of the peak positon:
                // initial parameters are hardcoded at the moment!
                TF1* fPeak = new TF1("fPeak", "gaus");
                fPeak->SetParameters(1, 100, 45);
                double timeInt = 50;
                hTemp->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
                fPeak = hTemp->GetFunction("fPeak");
                timePeak = fPeak->GetParameter(1);
                timePeakErr = fPeak->GetParError(1);

                delete fPeak;
                delete hTemp;
            }
            gTimeCorrelationVsTot_rowCorr_npx->SetPoint(iBin, iBin, timePeak);
            gTimeCorrelationVsTot_rowCorr_npx->SetPointError(iBin, 0, timePeakErr);
        } // for(iBin)

        /// END TIME WALK CORRECTION ///

    } // if(m_calcCorrections)

    // Example Slice to investigate quality of Gaussian fit:
    hTrackCorrelationTime_example = hTrackCorrelationTimeVsTot_rowCorr->ProjectionX(
        ("hTrackCorrelationTime_totBin_" + std::to_string(m_totBinExample)).c_str(), m_totBinExample, m_totBinExample + 1);

    int binMax = hTrackCorrelationTime_example->GetMaximumBin();
    double timePeak = hTrackCorrelationTime_example->GetXaxis()->GetBinCenter(binMax);

    TF1* fPeak = new TF1("fPeak", "gaus");
    fPeak->SetParameters(1, 100, 45);
    double timeInt = 50;
    std::string fitOption = "q"; // set to "q" = quiet for suppressed terminial output
    hTrackCorrelationTime_example->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
    delete fPeak;

    // hTrackCorrelationTime_example->Write();
    gTimeCorrelationVsRow->Write();
    gTimeCorrelationVsTot_rowCorr->Write();
    gTimeCorrelationVsTot_rowCorr_1px->Write();
    gTimeCorrelationVsTot_rowCorr_npx->Write();

    LOG(INFO) << "matched/total tracks: " << matched_tracks << "/" << total_tracks;
    LOG(INFO) << "total tracks (uncut):\t" << total_tracks_uncut;
    LOG(INFO) << "after chi2 cut:\t" << tracks_afterChi2Cut;
    LOG(INFO) << "with intercept:\t" << tracks_hasIntercept;
    LOG(INFO) << "withing ROI:\t\t" << tracks_isWithinROI;
    LOG(INFO) << "frameEdge cut:\t\t" << matched_tracks;
    LOG(INFO) << "after clusterTotCut:\t" << tracks_afterClusterTotCut;
    LOG(INFO) << "after clusterSizeCut:\t" << tracks_afterClusterSizeCut;
}

void AnalysisTimingATLASpix::correctClusterTimestamp(Cluster* cluster, int mode) {

    /* copied over from
     * Clustering4D::calculateClusterCentre()
     * and modified
     *
     * MODE:
     *  0 --> row correction
     *  1 --> timewalk correction
     */

    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0), correction(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();

    if(mode == 0) {
        correction = gRowCorr->Eval((*pixels)[0]->row());
    } else if(mode == 1) {
        correction = gTimeWalkCorr->Eval((*pixels)[0]->tot());
    } else {
        LOG(ERROR) << "Mode " << mode << " does not exist!\n"
                   << "Choose\n\t0 --> row correction \n\t1-->timewalk correction";
        return;
    }

    // Initial guess for cluster timestamp:
    double timestamp = (*pixels)[0]->timestamp() + correction;

    // Loop over all pixels:
    for(auto& pixel : (*pixels)) {
        double pixelToT = pixel->adc();
        if(pixelToT == 0) {
            LOG(DEBUG) << "Pixel with ToT 0!";
            pixelToT = 1;
        }

        tot += pixelToT;
        row += (pixel->row() * pixelToT);
        column += (pixel->column() * pixelToT);

        if(mode == 0) {
            correction = gRowCorr->Eval(pixel->row());
        } else if(mode == 1) {
            correction = gTimeWalkCorr->Eval(pixel->tot());
        } else {
            return;
        }

        // Override pixel timestamps:
        pixel->setTimestamp(pixel->timestamp() + correction);

        // timestamp = earliest pixel:
        if(pixel->timestamp() < timestamp) {
            timestamp = pixel->timestamp();
        }
    }
    // Row and column positions are tot-weighted
    row /= (tot > 0 ? tot : 1);
    column /= (tot > 0 ? tot : 1);

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(m_detector->pitch().X() * (column - m_detector->nPixels().X() / 2),
                                                        m_detector->pitch().Y() * (row - m_detector->nPixels().Y() / 2),
                                                        0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);

    // Set uncertainty on position from intrinstic detector resolution:
    cluster->setError(m_detector->resolution());

    cluster->setTimestamp(timestamp);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}
