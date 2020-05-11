/**
 * @file
 * @brief Implementation of [AnalysisEfficiency] module
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
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

AnalysisTimingATLASpix::AnalysisTimingATLASpix(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector) {

    // Backwards compatibilty: also allow timing_cut to be used for time_cut_abs
    config_.setAlias("time_cut_abs", "timing_cut", true);

    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<double>("time_cut_frameedge", static_cast<double>(Units::convert(20, "ns")));
    config_.setDefault<int>("high_tot_cut", 40);
    config_.setDefault<int>("low_tot_cut", 10);
    config_.setDefault<double>("timing_tail_cut", static_cast<double>(Units::convert(20, "ns")));
    config_.setDefault<bool>("calc_corrections", false);
    config_.setDefault<int>("tot_bin_example", 3);
    config_.setDefault<XYVector>(
        "inpixel_bin_size",
        {static_cast<double>(Units::convert(1.0, "um")), static_cast<double>(Units::convert(1.0, "um"))});

    using namespace ROOT::Math;
    m_detector = detector;
    if(config.count({"time_cut_rel", "time_cut_abs"}) > 1) {
        throw InvalidCombinationError(
            config_, {"time_cut_rel", "time_cut_abs"}, "Absolute and relative time cuts are mutually exclusive.");
    } else if(config_.has("time_cut_abs")) {
        m_timeCut = config_.get<double>("time_cut_abs");
    } else {
        m_timeCut = config_.get<double>("time_cut_rel", 3.0) * m_detector->getTimeResolution();
    }

    m_chi2ndofCut = config_.get<double>("chi2ndof_cut");
    m_timeCutFrameEdge = config_.get<double>("time_cut_frameedge");

    if(config_.has("cluster_charge_cut")) {
        m_clusterChargeCut = config_.get<double>("cluster_charge_cut");
    }
    if(config_.has("cluster_size_cut")) {
        m_clusterSizeCut = config_.get<size_t>("cluster_size_cut");
    }

    m_highTotCut = config_.get<int>("high_tot_cut");
    m_lowTotCut = config_.get<int>("low_tot_cut");
    m_timingTailCut = config_.get<double>("timing_tail_cut");

    if(config_.has("correction_file_row")) {
        m_correctionFile_row = config_.get<std::string>("correction_file_row");
        m_correctionGraph_row = config_.get<std::string>("correction_graph_row");
        m_pointwise_correction_row = true;
    } else {
        m_pointwise_correction_row = false;
    }
    if(config_.has("correction_file_timewalk")) {
        m_correctionFile_timewalk = config_.get<std::string>("correction_file_timewalk");
        m_correctionGraph_timewalk = config_.get<std::string>("correction_graph_timewalk");
        m_pointwise_correction_timewalk = true;
    } else {
        m_pointwise_correction_timewalk = false;
    }

    m_calcCorrections = config_.get<bool>("calc_corrections");
    m_totBinExample = config_.get<int>("tot_bin_example");
    m_inpixelBinSize = config_.get<XYVector>("inpixel_bin_size");

    total_tracks_uncut = 0;
    tracks_afterChi2Cut = 0;
    tracks_hasIntercept = 0;
    tracks_isWithinROI = 0;
    tracks_afterMasking = 0;
    total_tracks = 0;
    matched_tracks = 0;
    tracks_afterClusterChargeCut = 0;
    tracks_afterClusterSizeCut = 0;
}

void AnalysisTimingATLASpix::initialize() {

    auto pitch_x = static_cast<double>(Units::convert(m_detector->getPitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->getPitch().Y(), "um"));

    std::string name = "hTrackCorrelationTime";
    hTrackCorrelationTime =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
    hTrackCorrelationTime->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
    hTrackCorrelationTime->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTimeAssoc";
    hTrackCorrelationTimeAssoc =
        new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
    hTrackCorrelationTimeAssoc->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
    hTrackCorrelationTimeAssoc->GetYaxis()->SetTitle("# events");

    name = "hTrackCorrelationTimeAssocVsTime";
    hTrackCorrelationTimeAssocVsTime = new TH2F(name.c_str(), name.c_str(), 3e3, 0, 3e3, 1e3, -5, 5);
    hTrackCorrelationTimeAssocVsTime->GetYaxis()->SetTitle("ts_{track} - ts_{cluster} [us]");
    hTrackCorrelationTimeAssocVsTime->GetXaxis()->SetTitle("time [s]");
    hTrackCorrelationTimeAssocVsTime->GetZaxis()->SetTitle("# events");

    if(m_pointwise_correction_row) {
        name = "hTrackCorrelationTime_rowCorr";
        std::string title = "hTrackCorrelationTime_rowCorr: row-by-row correction";
        hTrackCorrelationTime_rowCorr =
            new TH1F(name.c_str(), title.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
        hTrackCorrelationTime_rowCorr->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
        hTrackCorrelationTime_rowCorr->GetYaxis()->SetTitle("# events");
    }

    if(m_pointwise_correction_timewalk) {
        name = "hTrackCorrelationTime_rowAndTWCorr";
        hTrackCorrelationTime_rowAndTWCorr =
            new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
        hTrackCorrelationTime_rowAndTWCorr->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
        hTrackCorrelationTime_rowAndTWCorr->GetYaxis()->SetTitle("# events");

        name = "hTrackCorrelationTime_rowAndTWCorr_l25";
        hTrackCorrelationTime_rowAndTWCorr_l25 =
            new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
        hTrackCorrelationTime_rowAndTWCorr_l25->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns] (if seed tot < 25lsb)");
        hTrackCorrelationTime_rowAndTWCorr_l25->GetYaxis()->SetTitle("# events");

        name = "hTrackCorrelationTime_rowAndTWCorr_l40";
        hTrackCorrelationTime_rowAndTWCorr_l40 =
            new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
        hTrackCorrelationTime_rowAndTWCorr_l40->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns] (if seed tot < 40lsb)");
        hTrackCorrelationTime_rowAndTWCorr_l40->GetYaxis()->SetTitle("# events");

        name = "hTrackCorrelationTime_rowAndTWCorr_g40";
        hTrackCorrelationTime_rowAndTWCorr_g40 =
            new TH1F(name.c_str(), name.c_str(), static_cast<int>(2. * m_timeCut), -1 * m_timeCut, m_timeCut);
        hTrackCorrelationTime_rowAndTWCorr_g40->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns] (if seed tot > 40lsb)");
        hTrackCorrelationTime_rowAndTWCorr_g40->GetYaxis()->SetTitle("# events");

        name = "hTrackCorrelationTime_totBin_" + std::to_string(m_totBinExample);
        hTrackCorrelationTime_example = new TH1D(name.c_str(), name.c_str(), 20000, -5000, 5000);
        hTrackCorrelationTime_example->GetXaxis()->SetTitle(
            "track time stamp - pixel time stamp [ns] (all pixels from cluster)");
        hTrackCorrelationTime_example->GetYaxis()->SetTitle("# events");
    }

    // 2D histograms:
    // column dependence
    name = "hTrackCorrelationTimeVsCol";
    hTrackCorrelationTimeVsCol = new TH2F(
        name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().X(), -0.5, m_detector->nPixels().X() - 0.5);
    hTrackCorrelationTimeVsCol->GetYaxis()->SetTitle("pixel column");
    hTrackCorrelationTimeVsCol->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns]");
    // row dependence
    name = "hTrackCorrelationTimeVsRow";
    hTrackCorrelationTimeVsRow = new TH2F(
        name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), -0.5, m_detector->nPixels().Y() - 0.5);
    hTrackCorrelationTimeVsRow->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns]");
    name = "hTrackCorrelationTimeVsRow_1px";
    hTrackCorrelationTimeVsRow_1px = new TH2F(
        name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), -0.5, m_detector->nPixels().Y() - 0.5);
    hTrackCorrelationTimeVsRow_1px->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow_1px->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns] (single-pixel clusters)");
    name = "hTrackCorrelationTimeVsRow_npx";
    hTrackCorrelationTimeVsRow_npx = new TH2F(
        name.c_str(), name.c_str(), 20000, -5000, 5000, m_detector->nPixels().Y(), -0.5, m_detector->nPixels().Y() - 0.5);
    hTrackCorrelationTimeVsRow_npx->GetYaxis()->SetTitle("pixel row");
    hTrackCorrelationTimeVsRow_npx->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns] (multi-pixel clusters)");

    // control plot: row dependence after row correction
    if(m_pointwise_correction_row) {
        name = "hTrackCorrelationTimeVsRow_rowCorr";
        hTrackCorrelationTimeVsRow_rowCorr = new TH2F(name.c_str(),
                                                      name.c_str(),
                                                      20000,
                                                      -5000,
                                                      5000,
                                                      m_detector->nPixels().Y(),
                                                      -0.5,
                                                      m_detector->nPixels().Y() - 0.5);
        hTrackCorrelationTimeVsRow_rowCorr->GetYaxis()->SetTitle("pixel row");
        hTrackCorrelationTimeVsRow_rowCorr->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns]");
    }

    // control plot: time walk dependence, not row corrected
    name = "hTrackCorrelationTimeVsTot";
    hTrackCorrelationTimeVsTot = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
    hTrackCorrelationTimeVsTot->GetYaxis()->SetTitle("seed pixel tot [lsb]");
    hTrackCorrelationTimeVsTot->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");

    name = "hTrackCorrelationTimeVsTot_1px";
    hTrackCorrelationTimeVsTot_1px = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
    hTrackCorrelationTimeVsTot_1px->GetYaxis()->SetTitle("seed pixel tot [lsb] (if clustersize = 1)");
    hTrackCorrelationTimeVsTot_1px->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");

    name = "hTrackCorrelationTimeVsTot_npx";
    hTrackCorrelationTimeVsTot_npx = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
    hTrackCorrelationTimeVsTot_npx->GetYaxis()->SetTitle("seed pixel tot [lsb] (if clustersize > 1)");
    hTrackCorrelationTimeVsTot_npx->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");

    name = "hTrackCorrelationTimeVsTot_px";
    hTrackCorrelationTimeVsTot_px = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
    hTrackCorrelationTimeVsTot_px->GetYaxis()->SetTitle("pixel tot [lsb]");
    hTrackCorrelationTimeVsTot_px->GetXaxis()->SetTitle("ts_{track} - ts_{pixel} (all pixels from cluster) [ns]");

    name = "hClusterTimeMinusPixelTime";
    hClusterTimeMinusPixelTime = new TH1F(name.c_str(), name.c_str(), 2000, -1000, 1000);
    hClusterTimeMinusPixelTime->GetXaxis()->SetTitle(
        "ts_{cluster} - ts_{pixel} [ns] (all pixels from cluster (if clusterSize>1))");

    // timewalk after row correction
    if(m_pointwise_correction_row) {
        name = "hTrackCorrelationTimeVsTot_rowCorr";
        hTrackCorrelationTimeVsTot_rowCorr = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
        hTrackCorrelationTimeVsTot_rowCorr->GetYaxis()->SetTitle("seed pixel tot [lsb]");
        hTrackCorrelationTimeVsTot_rowCorr->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns]");

        name = "hTrackCorrelationTimeVsTot_rowCorr_1px";
        hTrackCorrelationTimeVsTot_rowCorr_1px = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
        hTrackCorrelationTimeVsTot_rowCorr_1px->GetYaxis()->SetTitle("seed pixel tot [lsb] (single-pixel clusters)");
        hTrackCorrelationTimeVsTot_rowCorr_1px->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");

        name = "hTrackCorrelationTimeVsTot_rowCorr_npx";
        hTrackCorrelationTimeVsTot_rowCorr_npx = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
        hTrackCorrelationTimeVsTot_rowCorr_npx->GetYaxis()->SetTitle("seed pixel tot [lsb] (multi-pixel clusters)");
        hTrackCorrelationTimeVsTot_rowCorr_npx->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
    }

    // final plots with both row and timewalk correction:
    if(m_pointwise_correction_timewalk) {
        name = "hTrackCorrelationTimeVsRow_rowAndTWCorr";
        hTrackCorrelationTimeVsRow_rowAndTWCorr = new TH2F(name.c_str(),
                                                           name.c_str(),
                                                           20000,
                                                           -5000,
                                                           5000,
                                                           m_detector->nPixels().Y(),
                                                           -0.5,
                                                           m_detector->nPixels().Y() - 0.5);
        hTrackCorrelationTimeVsRow_rowAndTWCorr->GetYaxis()->SetTitle("row");
        hTrackCorrelationTimeVsRow_rowAndTWCorr->GetXaxis()->SetTitle("ts_{track} - ts_{seed pixel} [ns]");

        name = "hTrackCorrelationTimeVsTot_rowAndTWCorr";
        hTrackCorrelationTimeVsTot_rowAndTWCorr = new TH2F(name.c_str(), name.c_str(), 20000, -5000, 5000, 64, -0.5, 63.5);
        hTrackCorrelationTimeVsTot_rowAndTWCorr->GetYaxis()->SetTitle("seed pixel tot [lsb]");
        hTrackCorrelationTimeVsTot_rowAndTWCorr->GetXaxis()->SetTitle("ts_{track} - ts_{cluster} [ns]");
    }

    // in-pixel time resolution plots:
    auto nbins_x = static_cast<int>(std::ceil(m_detector->getPitch().X() / m_inpixelBinSize.X()));
    auto nbins_y = static_cast<int>(std::ceil(m_detector->getPitch().Y() / m_inpixelBinSize.Y()));
    if(nbins_x > 1e4 || nbins_y > 1e4) {
        throw InvalidValueError(config_, "inpixel_bin_size", "Too many bins for in-pixel histograms.");
    }

    std::string title =
        "in-pixel time resolution map;in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];ts_{track} - ts_{cluster} [ns]";
    hPixelTrackCorrelationTimeMap = new TProfile2D("pixelTrackCorrelationTimeMap",
                                                   title.c_str(),
                                                   nbins_x,
                                                   -pitch_x / 2.,
                                                   pitch_x / 2.,
                                                   nbins_y,
                                                   -pitch_y / 2.,
                                                   pitch_y / 2.,
                                                   -1 * m_timeCut,
                                                   m_timeCut);

    name = "hClusterSizeVsTot_Assoc";
    hClusterSizeVsTot_Assoc = new TH2F(name.c_str(), name.c_str(), 20, 0, 20, 64, -0.5, 63.5);
    hClusterSizeVsTot_Assoc->GetYaxis()->SetTitle("pixel ToT [lsb] (all pixels from cluster)");
    hClusterSizeVsTot_Assoc->GetXaxis()->SetTitle("clusterSize");

    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "hitMapAssoc; x_{track} [px]; y_{track} [px];# entries",
                            m_detector->nPixels().X(),
                            -0.5,
                            m_detector->nPixels().X() - 0.5,
                            m_detector->nPixels().Y(),
                            -0.5,
                            m_detector->nPixels().Y() - 0.5);
    hHitMapAssoc_highToT = new TH2F("hitMapAssoc_highToT",
                                    "hitMapAssoc_highToT; x_{track} [px]; y_{track} [px];# entries",
                                    m_detector->nPixels().X(),
                                    -0.5,
                                    m_detector->nPixels().X() - 0.5,
                                    m_detector->nPixels().Y(),
                                    -0.5,
                                    m_detector->nPixels().Y() - 0.5);
    hHitMapAssoc_inPixel = new TH2F("hitMapAssoc_inPixel",
                                    "hitMapAssoc_inPixel; in-pixel x_{track} [#mum]; in-pixel y_{track} [#mum]",
                                    static_cast<int>(pitch_x),
                                    -pitch_x / 2.,
                                    pitch_x / 2.,
                                    static_cast<int>(pitch_y),
                                    -pitch_y / 2.,
                                    pitch_y / 2.);
    if(config_.has("high_tot_cut")) {
        hHitMapAssoc_inPixel_highToT =
            new TH2F("hitMapAssoc_inPixel_highToT",
                     "hitMapAssoc_inPixel_highToT;in-pixel x_{track} [#mum];in-pixel y_{track} [#mum]",
                     static_cast<int>(pitch_x),
                     -pitch_x / 2.,
                     pitch_x / 2.,
                     static_cast<int>(pitch_y),
                     -pitch_y / 2.,
                     pitch_y / 2.);
    }
    hClusterMapAssoc = new TH2F("hClusterMapAssoc",
                                "hClusterMapAssoc; x_{cluster} [px]; y_{cluster} [px];# entries",
                                m_detector->nPixels().X(),
                                -0.5,
                                m_detector->nPixels().X() - 0.5,
                                m_detector->nPixels().Y(),
                                -0.5,
                                m_detector->nPixels().Y() - 0.5);

    hTotVsRow = new TH2F("hTotVsRow",
                         "hTotVsRow;seed-pixel row;seed-pixel ToT [lsb]",
                         m_detector->nPixels().Y(),
                         -0.5,
                         m_detector->nPixels().Y() - 0.5,
                         64,
                         -0.5,
                         63.5);

    hTotVsTime = new TH2F("hTotVsTime", "hTotVsTime", 64, -0.5, 63.5, 1e6, 0, 100);
    hTotVsTime->GetXaxis()->SetTitle("pixel ToT [lsb]");
    hTotVsTime->GetYaxis()->SetTitle("time [s]");
    if(config_.has("high_tot_cut")) {
        hTotVsTime_highToT = new TH2F("hTotVsTime_highToT", "hTotVsTime_highToT", 64, -0.5, 63.5, 1e6, 0, 100);
        hTotVsTime_highToT->GetXaxis()->SetTitle("pixel ToT [lsb] if > high_tot_cut");
        hTotVsTime_highToT->GetYaxis()->SetTitle("time [s]");
    }

    // control plots for "left/right tail" and "main peak" of the track time correlation
    if(config_.has("timing_tail_cut") && m_pointwise_correction_timewalk) {
        hInPixelMap_leftTail = new TH2F("hPixelMap_leftTail",
                                        "in-pixel track position (left tail of time residual);in-pixel x_{track} "
                                        "[#mum];in-pixel y_{track} [#mum];# entries",
                                        nbins_x,
                                        -pitch_x / 2.,
                                        pitch_x / 2.,
                                        nbins_y,
                                        -pitch_y / 2.,
                                        pitch_y / 2.);
        hInPixelMap_rightTail = new TH2F("hPixelMap_rightTail",
                                         "in-pixel track position (right tail of time residual);in-pixel x_{track} "
                                         "[#mum];in-pixel y_{track} [#mum];# entries",
                                         nbins_x,
                                         -pitch_x / 2.,
                                         pitch_x / 2.,
                                         nbins_y,
                                         -pitch_y / 2.,
                                         pitch_y / 2.);
        hInPixelMap_mainPeak = new TH2F("hPixelMap_mainPeak",
                                        "in-pixel track position (main peak of time residual);in-pixel x_{track} "
                                        "[#mum];in-pixel y_{track} [#mum];# entries",
                                        nbins_x,
                                        -pitch_x / 2.,
                                        pitch_x / 2.,
                                        nbins_y,
                                        -pitch_y / 2.,
                                        pitch_y / 2.);
        hClusterMap_leftTail =
            new TH2F("hClusterMap_leftTail",
                     "hClusterMap (left tail of time residual); x_{cluster} [px]; x_{cluster} [px];# entries",
                     m_detector->nPixels().X(),
                     -0.5,
                     m_detector->nPixels().X() - 0.5,
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5);
        hClusterMap_rightTail =
            new TH2F("hClusterMap_rightTail",
                     "hClusterMap (right tail of time residual); x_{cluster} [px]; x_{cluster} [px];# entries",
                     m_detector->nPixels().X(),
                     -0.5,
                     m_detector->nPixels().X() - 0.5,
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5);
        hClusterMap_mainPeak =
            new TH2F("hClusterMap_mainPeak",
                     "hClusterMap (main peak of time residual); x_{cluster} [px]; x_{cluster} [px];# entries",
                     m_detector->nPixels().X(),
                     -0.5,
                     m_detector->nPixels().X() - 0.5,
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5);
        hClusterSize_leftTail = new TH1F(
            "clusterSize_leftTail", "clusterSize (left tail of time residual);cluster size;# entries", 100, -0.5, 99.5);
        hClusterSize_rightTail = new TH1F(
            "clusterSize_rightTail", "clusterSize (right tail of time residual);cluster size;# entries", 100, -0.5, 99.5);
        hClusterSize_mainPeak = new TH1F(
            "clusterSize_mainPeak", "clusterSize (main peak of time residual);cluster size;# entries", 100, -0.5, 99.5);
        hTot_leftTail =
            new TH1F("hTot_leftTail", "ToT (left tail of time residual);seed pixel ToT [lsb];# events", 2 * 64, -64.5, 63.5);
        hTot_rightTail = new TH1F(
            "hTot_rightTail", "ToT (right tail of time residual);seed pixel ToT [lsb];# events", 2 * 64, -64.5, 63.5);
        hTot_mainPeak = new TH1F("hTot_mainPeak",
                                 "ToT (main peak of time residual, 1px clusters);seed pixel ToT [lsb];# events",
                                 2 * 64,
                                 -64.5,
                                 63.5);
        hTot_leftTail_1px = new TH1F("hTot_leftTail_1px",
                                     "ToT (left tail of time residual, 1px clusters);seed pixel ToT [lsb];# events",
                                     2 * 64,
                                     -64.5,
                                     63.5);
        hTot_rightTail_1px = new TH1F("hTot_rightTail_1px",
                                      "ToT (right tail of time residual, 1px clusters);seed pixel ToT [lsb];# events",
                                      2 * 64,
                                      -64.5,
                                      63.5);
        hTot_mainPeak_1px = new TH1F("hTot_mainPeak_1px",
                                     "ToT (main peak of time residual, 1px clusters);seed pixel ToT [lsb];# events",
                                     2 * 64,
                                     -64.5,
                                     63.5);
        hPixelTimestamp_leftTail = new TH1F("pixelTimestamp_leftTail",
                                            "pixelTimestamp (left tail of time residual);pixel timestamp [ms];# events",
                                            3e6,
                                            0,
                                            3e3);
        hPixelTimestamp_rightTail = new TH1F("pixelTimestamp_rightTail",
                                             "pixelTimestamp (left tail of time residual);pixel timestamp [ms];# events",
                                             3e6,
                                             0,
                                             3e3);
        hPixelTimestamp_mainPeak = new TH1F("pixelTimestamp_mainPeak",
                                            "pixelTimestamp (left tail of time residual);pixel timestamp [ms];# events",
                                            3e6,
                                            0,
                                            3e3);
    }

    // /////////////////////////////////////////// //
    // TGraphErrors for Timewalk & Row Correction: //
    // /////////////////////////////////////////// //

    if(m_calcCorrections) {
        gTimeCorrelationVsRow = new TGraphErrors();
        gTimeCorrelationVsRow->SetName("gTimeCorrelationVsRow");
        gTimeCorrelationVsRow->SetTitle("gTimeCorrelationVsRow");
        gTimeCorrelationVsRow->GetXaxis()->SetTitle("row");
        gTimeCorrelationVsRow->GetYaxis()->SetTitle("time correlation peak [ns]");
    }

    if(m_pointwise_correction_row) {
        int nBinsToT = hTrackCorrelationTimeVsTot_rowCorr->GetNbinsY();
        gTimeCorrelationVsTot_rowCorr = new TGraphErrors(nBinsToT);
        gTimeCorrelationVsTot_rowCorr->SetName("gTimeCorrelationVsTot_rowCorr");
        gTimeCorrelationVsTot_rowCorr->SetTitle("gTimeCorrelationVsTot_rowCorr");
        gTimeCorrelationVsTot_rowCorr->GetXaxis()->SetTitle("pixel ToT [lsb]");
        gTimeCorrelationVsTot_rowCorr->GetYaxis()->SetTitle("time correlation peak [ns]");

        nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_1px->GetNbinsY();
        gTimeCorrelationVsTot_rowCorr_1px = new TGraphErrors(nBinsToT);
        gTimeCorrelationVsTot_rowCorr_1px->SetName("gTimeCorrelationVsTot_rowCorr_1px");
        gTimeCorrelationVsTot_rowCorr_1px->SetTitle("gTimeCorrelationVsTot_rowCorr_1px");
        gTimeCorrelationVsTot_rowCorr_1px->GetXaxis()->SetTitle("pixel ToT [lsb] (single-pixel clusters)");
        gTimeCorrelationVsTot_rowCorr_1px->GetYaxis()->SetTitle("time correlation peak [ns]");

        nBinsToT = hTrackCorrelationTimeVsTot_rowCorr_npx->GetNbinsY();
        gTimeCorrelationVsTot_rowCorr_npx = new TGraphErrors(nBinsToT);
        gTimeCorrelationVsTot_rowCorr_npx->SetName("gTimeCorrelationVsTot_rowCorr_npx");
        gTimeCorrelationVsTot_rowCorr_npx->SetTitle("gTimeCorrelationVsTot_rowCorr_npx");
        gTimeCorrelationVsTot_rowCorr_npx->GetXaxis()->SetTitle("pixel ToT [lsb] (multi-pixel clusters");
        gTimeCorrelationVsTot_rowCorr_npx->GetYaxis()->SetTitle("time correlation peak [ns]");
    }

    LOG(INFO) << "Calculate corrections: = " << m_calcCorrections;

    if(m_pointwise_correction_row) {
        // Import TGraphErrors for row corection:
        TFile file(m_correctionFile_row.c_str());
        if(!file.IsOpen()) {
            throw InvalidValueError(config_,
                                    "correction_file_row",
                                    "ROOT file doesn't exist. If no row correction shall be applied, remove this parameter "
                                    "from the configuration file.");
        }

        gRowCorr = static_cast<TGraphErrors*>(file.Get(m_correctionGraph_row.c_str()));
        // Check if graph exists in ROOT file:
        if(!gRowCorr) {
            throw InvalidValueError(
                config_, "correction_graph_row", "Graph doesn't exist in ROOT file. Use full/path/to/graph.");
        }
    } else {
        LOG(STATUS) << "----> NO POINTWISE ROW CORRECTION!!!";
    }
    if(m_pointwise_correction_timewalk) {
        // Import TGraphErrors for timewalk corection:
        TFile file(m_correctionFile_timewalk.c_str());
        if(!file.IsOpen()) {
            throw InvalidValueError(config_,
                                    "correction_file_timewalk",
                                    "ROOT file doesn't exist. If no row correction shall be applied, remove this parameter "
                                    "from the configuration file.");
        }

        gTimeWalkCorr = static_cast<TGraphErrors*>(file.Get(m_correctionGraph_timewalk.c_str()));
        // Check if graph exists in ROOT file:
        if(!gTimeWalkCorr) {
            throw InvalidValueError(
                config_, "correction_graph_timewalk", "Graph doesn't exist in ROOT file. Use full/path/to/graph.");
        }
    } else {
        LOG(STATUS) << "----> NO POINTWISE TIMEWALK CORRECTION!!!";
    }
}

StatusCode AnalysisTimingATLASpix::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    for(auto& track : tracks) {
        bool has_associated_cluster = false;
        bool is_within_roi = true;
        LOG(DEBUG) << "Looking at next track";
        total_tracks_uncut++;

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > m_chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            continue;
        }
        tracks_afterChi2Cut++;

        // Check if it intercepts the DUT
        if(!m_detector->hasIntercept(track.get(), 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
            continue;
        }
        tracks_hasIntercept++;

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track.get())) {
            LOG(DEBUG) << " - track outside ROI";
            is_within_roi = false;
        }
        tracks_isWithinROI++;

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track.get(), 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }
        tracks_afterMasking++;

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
            // Early edge - eventStart points to the start of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            continue;
        }

        // Count this as reference track:
        total_tracks++;

        // Get the DUT clusters from the clipboard
        auto clusters = clipboard->getData<Cluster>(m_detector->getName());
        // Loop over all DUT clusters to find matches:
        for(auto& cluster : clusters) {
            LOG(DEBUG) << " - Looking at next DUT cluster";

            hTrackCorrelationTime->Fill(track->timestamp() - cluster->timestamp());

            auto associated_clusters = track->getAssociatedClusters(m_detector->getName());
            if(std::find(associated_clusters.begin(), associated_clusters.end(), cluster.get()) !=
               associated_clusters.end()) {
                LOG(DEBUG) << "Found associated cluster " << (*cluster);
                has_associated_cluster = true;
                matched_tracks++;

                if(config_.has("cluster_charge_cut") && cluster->charge() > m_clusterChargeCut) {
                    LOG(DEBUG) << " - track discarded due to clusterChargeCut";
                    continue;
                }
                tracks_afterClusterChargeCut++;

                if(config_.has("cluster_size_cut") && cluster->size() > m_clusterSizeCut) {
                    LOG(DEBUG) << " - track discarded due to clusterSizeCut";
                    continue;
                }
                tracks_afterClusterSizeCut++;

                double timeDiff = track->timestamp() - cluster->timestamp();
                hTrackCorrelationTimeAssoc->Fill(timeDiff);
                hTrackCorrelationTimeAssocVsTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                                       static_cast<double>(Units::convert(timeDiff, "us")));

                hTrackCorrelationTimeVsTot->Fill(timeDiff, cluster->getSeedPixel()->raw());
                hTrackCorrelationTimeVsCol->Fill(timeDiff, cluster->getSeedPixel()->column());
                hTrackCorrelationTimeVsRow->Fill(timeDiff, cluster->getSeedPixel()->row());

                hTotVsRow->Fill(cluster->getSeedPixel()->row(), cluster->getSeedPixel()->raw());

                // single-pixel and multi-pixel clusters:
                if(cluster->size() == 1) {
                    hTrackCorrelationTimeVsTot_1px->Fill(timeDiff, cluster->getSeedPixel()->raw());
                    hTrackCorrelationTimeVsRow_1px->Fill(timeDiff, cluster->getSeedPixel()->row());
                } else {
                    hTrackCorrelationTimeVsTot_npx->Fill(timeDiff, cluster->getSeedPixel()->raw());
                    hTrackCorrelationTimeVsRow_npx->Fill(timeDiff, cluster->getSeedPixel()->row());
                }

                // Calculate in-pixel position of track in microns
                auto globalIntercept = m_detector->getIntercept(track.get());
                auto localIntercept = m_detector->globalToLocal(globalIntercept);
                auto inpixel = m_detector->inPixel(localIntercept);
                auto xmod = static_cast<double>(Units::convert(inpixel.X(), "um"));
                auto ymod = static_cast<double>(Units::convert(inpixel.Y(), "um"));
                hHitMapAssoc_inPixel->Fill(xmod, ymod);
                if(config_.has("high_tot_cut") && cluster->charge() > m_highTotCut && cluster->size() == 1) {
                    hHitMapAssoc_inPixel_highToT->Fill(xmod, ymod);
                }
                hPixelTrackCorrelationTimeMap->Fill(xmod, ymod, timeDiff);

                // 2D histograms: --> fill for all pixels from cluster
                for(auto& pixel : cluster->pixels()) {

                    hTrackCorrelationTimeVsTot_px->Fill(track->timestamp() - pixel->timestamp(), pixel->raw());

                    // to check that cluster timestamp = earliest pixel timestamp
                    if(cluster->size() > 1) {
                        hClusterTimeMinusPixelTime->Fill(cluster->timestamp() - pixel->timestamp());
                    }

                    hClusterSizeVsTot_Assoc->Fill(static_cast<double>(cluster->size()), pixel->raw());
                    hHitMapAssoc->Fill(pixel->column(), pixel->row());
                    hTotVsTime->Fill(pixel->raw(), static_cast<double>(Units::convert(pixel->timestamp(), "s")));
                    if(config_.has("high_tot_cut") && pixel->raw() > m_highTotCut) {
                        hHitMapAssoc_highToT->Fill(pixel->column(), pixel->row());
                        hTotVsTime_highToT->Fill(pixel->raw(), static_cast<double>(Units::convert(pixel->timestamp(), "s")));
                    }
                }
                hClusterMapAssoc->Fill(cluster->column(), cluster->row());
                if(config_.has("timing_tail_cut") && m_pointwise_correction_timewalk) {
                    if(track->timestamp() - cluster->timestamp() < -m_timingTailCut) {
                        hInPixelMap_leftTail->Fill(xmod, ymod);
                    } else if(track->timestamp() - cluster->timestamp() > m_timingTailCut) {
                        hInPixelMap_rightTail->Fill(xmod, ymod);
                    } else {
                        hInPixelMap_mainPeak->Fill(xmod, ymod);
                    }
                }

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
                                                             cluster->getSeedPixel()->raw());
                    if(cluster->size() == 1) {
                        hTrackCorrelationTimeVsTot_rowCorr_1px->Fill(
                            track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->raw());
                    }
                    if(cluster->size() > 1) {
                        hTrackCorrelationTimeVsTot_rowCorr_npx->Fill(
                            track->timestamp() - cluster->getSeedPixel()->timestamp(), cluster->getSeedPixel()->raw());
                    }
                    //} for(auto& pixels : ...)
                }
                // point-wise timewalk correction on top:
                if(m_pointwise_correction_timewalk) {
                    correctClusterTimestamp(cluster, 1); // mode=1 --> timewalk correction
                    hTrackCorrelationTime_rowAndTWCorr->Fill(track->timestamp() - cluster->timestamp());
                    if(cluster->getSeedPixel()->raw() < 25) {
                        hTrackCorrelationTime_rowAndTWCorr_l25->Fill(track->timestamp() - cluster->timestamp());
                    }
                    if(cluster->getSeedPixel()->raw() < 40) {
                        hTrackCorrelationTime_rowAndTWCorr_l40->Fill(track->timestamp() - cluster->timestamp());
                    }
                    if(cluster->getSeedPixel()->raw() > 40) {
                        hTrackCorrelationTime_rowAndTWCorr_g40->Fill(track->timestamp() - cluster->timestamp());
                    }

                    hTrackCorrelationTimeVsRow_rowAndTWCorr->Fill(track->timestamp() - cluster->getSeedPixel()->timestamp(),
                                                                  cluster->getSeedPixel()->row());
                    hTrackCorrelationTimeVsTot_rowAndTWCorr->Fill(track->timestamp() - cluster->getSeedPixel()->timestamp(),
                                                                  cluster->getSeedPixel()->raw());

                    // control plots to investigate "left/right tail" in time correlation:
                    if(config_.has("timing_tail_cut")) {
                        if(track->timestamp() - cluster->timestamp() < -m_timingTailCut) {
                            hClusterMap_leftTail->Fill(cluster->column(), cluster->row());
                            hTot_leftTail->Fill(cluster->getSeedPixel()->raw());
                            hPixelTimestamp_leftTail->Fill(cluster->getSeedPixel()->timestamp());
                            hClusterSize_leftTail->Fill(static_cast<double>(cluster->size()));
                            if(cluster->size() == 1) {
                                hTot_leftTail_1px->Fill(cluster->getSeedPixel()->raw());
                            }
                        } else if(track->timestamp() - cluster->timestamp() > m_timingTailCut) {
                            hClusterMap_rightTail->Fill(cluster->column(), cluster->row());
                            hTot_rightTail->Fill(cluster->getSeedPixel()->raw());
                            hPixelTimestamp_rightTail->Fill(cluster->getSeedPixel()->timestamp());
                            hClusterSize_rightTail->Fill(static_cast<double>(cluster->size()));
                            if(cluster->size() == 1) {
                                hTot_rightTail_1px->Fill(cluster->getSeedPixel()->raw());
                            }
                        } else {
                            hClusterMap_mainPeak->Fill(cluster->column(), cluster->row());
                            hTot_mainPeak->Fill(cluster->getSeedPixel()->raw());
                            hPixelTimestamp_mainPeak->Fill(cluster->getSeedPixel()->timestamp());
                            hClusterSize_mainPeak->Fill(static_cast<double>(cluster->size()));
                            if(cluster->size() == 1) {
                                hTot_mainPeak_1px->Fill(cluster->getSeedPixel()->raw());
                            }
                        }
                    }
                }
            }

        } // for loop over all clusters

        LOG(DEBUG) << "is_within_roi = " << is_within_roi;
        LOG(DEBUG) << "has_associated_cluster = " << has_associated_cluster;

    } // for loop over all tracks

    return StatusCode::Success;
}

void AnalysisTimingATLASpix::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    LOG(STATUS) << "Timing analysis finished for detector " << m_detector->getName() << ": ";

    if(m_calcCorrections) {

        /// ROW CORRECTION ///
        std::string fitOption = "q"; // set to "" for terminal output
        int binMax = 0;
        double timePeak = 0.;
        double timePeakErr = 0.;
        int nRows = hTrackCorrelationTimeVsRow->GetNbinsY();

        for(int iBin = 0; iBin < nRows; iBin++) {
            TH1D* hTemp = hTrackCorrelationTimeVsRow->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

            // if(hTemp->GetEntries() < 500) { // too few entries to fit
            if(hTemp->GetEntries() < 250) { // too few entries to fit
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
        if(m_pointwise_correction_row) {
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
                    timePeak = hTemp->GetMean();
                    timePeakErr = hTemp->GetStdDev();
                    delete hTemp;
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
                TH1D* hTemp =
                    hTrackCorrelationTimeVsTot_rowCorr_1px->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

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
                TH1D* hTemp =
                    hTrackCorrelationTimeVsTot_rowCorr_npx->ProjectionX("timeCorrelationInOneTotBin", iBin, iBin + 1);

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

            // Example Slice to investigate quality of Gaussian fit:
            hTrackCorrelationTime_example = hTrackCorrelationTimeVsTot_rowCorr->ProjectionX(
                ("hTrackCorrelationTime_totBin_" + std::to_string(m_totBinExample)).c_str(),
                m_totBinExample,
                m_totBinExample + 1);

            binMax = hTrackCorrelationTime_example->GetMaximumBin();
            timePeak = hTrackCorrelationTime_example->GetXaxis()->GetBinCenter(binMax);

            TF1* fPeak = new TF1("fPeak", "gaus");
            fPeak->SetParameters(1, 100, 45);
            double timeInt = 50;
            fitOption = "q"; // set to "q" = quiet for suppressed terminial output
            hTrackCorrelationTime_example->Fit("fPeak", fitOption.c_str(), "", timePeak - timeInt, timePeak + timeInt);
            delete fPeak;
        }

        gTimeCorrelationVsRow->Write();
        if(m_pointwise_correction_row) {
            gTimeCorrelationVsTot_rowCorr->Write();
            gTimeCorrelationVsTot_rowCorr_1px->Write();
            gTimeCorrelationVsTot_rowCorr_npx->Write();
        }
    } // if(m_calcCorrections)

    LOG(INFO) << "matched/total tracks: " << matched_tracks << "/" << total_tracks;
    LOG(INFO) << "total tracks (uncut):\t" << total_tracks_uncut;
    LOG(INFO) << "after chi2 cut:\t" << tracks_afterChi2Cut;
    LOG(INFO) << "with intercept:\t" << tracks_hasIntercept;
    LOG(INFO) << "withing ROI:\t\t" << tracks_isWithinROI;
    LOG(INFO) << "frameEdge cut:\t\t" << matched_tracks;
    LOG(INFO) << "after clusterTotCut:\t" << tracks_afterClusterChargeCut;
    LOG(INFO) << "after clusterSizeCut:\t" << tracks_afterClusterSizeCut;
}

void AnalysisTimingATLASpix::correctClusterTimestamp(std::shared_ptr<Cluster> cluster, int mode) {

    /*
     * MODE:
     *  0 --> row correction
     *  1 --> timewalk correction
     */

    // Get the pixels on this cluster
    auto pixels = cluster->pixels();
    auto first_pixel = pixels.front();
    double correction = 0;

    if(mode == 0) {
        correction = gRowCorr->Eval(first_pixel->row());
    } else if(mode == 1) {
        correction = gTimeWalkCorr->Eval(first_pixel->raw());
    } else {
        LOG(ERROR) << "Mode " << mode << " does not exist!\n"
                   << "Choose\n\t0 --> row correction \n\t1-->timewalk correction";
        return;
    }

    // Initial guess for cluster timestamp:
    double timestamp = first_pixel->timestamp() + correction;

    // Loop over all pixels:
    for(auto& pixel : pixels) {
        // FIXME ugly hack
        auto px = const_cast<Pixel*>(pixel);

        if(mode == 0) {
            correction = gRowCorr->Eval(pixel->row());
        } else if(mode == 1) {
            correction = gTimeWalkCorr->Eval(pixel->raw());
        } else {
            return;
        }

        // Override pixel timestamps:
        px->setTimestamp(pixel->timestamp() + correction);

        // timestamp = earliest pixel:
        if(pixel->timestamp() < timestamp) {
            timestamp = pixel->timestamp();
        }
    }

    cluster->setTimestamp(timestamp);
}
