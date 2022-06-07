/**
 * @file
 * @brief Implementation of module AnalysisDUT
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisDUT.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisDUT::AnalysisDUT(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    config_.setDefault<double>("spatial_cut_sensoredge", 0.5);
    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<bool>("use_closest_cluster", true);
    config_.setDefault<int>("n_time_bins", 20000);
    config_.setDefault<double>("time_binning", Units::get<double>(0.1, "ns"));
    config_.setDefault<bool>("correlations", false);
    config_.setDefault<int>("n_charge_bins", 1000);
    config_.setDefault<double>("charge_histo_range", 1000.0);
    config_.setDefault<int>("n_raw_bins", 1000);
    config_.setDefault<double>("raw_histo_range", 1000.0);
    config_.setDefault<double>("inpixel_bin_size", Units::get<double>(0.5, "um"));

    time_cut_frameedge_ = config_.get<double>("time_cut_frameedge");
    spatial_cut_sensoredge_ = config_.get<double>("spatial_cut_sensoredge");
    chi2_ndof_cut_ = config_.get<double>("chi2ndof_cut");
    use_closest_cluster_ = config_.get<bool>("use_closest_cluster");
    n_timebins_ = config_.get<int>("n_time_bins");
    time_binning_ = config_.get<double>("time_binning");
    correlations_ = config_.get<bool>("correlations");
    n_chargebins_ = config_.get<int>("n_charge_bins");
    charge_histo_range_ = config_.get<double>("charge_histo_range");
    inpixelBinSize_ = config_.get<double>("inpixel_bin_size");

    // if no separate raw histo bin settings are given, use the ones specified for the charge
    if(config_.has("n_charge_bins") & !config_.has("n_raw_bins")) {
        n_rawbins_ = n_chargebins_;
    } else {
        n_rawbins_ = config_.get<int>("n_raw_bins");
    }

    if(config_.has("charge_histo_range") & !config_.has("raw_histo_range")) {
        raw_histo_range_ = charge_histo_range_;
    } else {
        raw_histo_range_ = config_.get<double>("raw_histo_range");
    }
}

void AnalysisDUT::initialize() {

    createGlobalResidualPlots();
    createLocalResidualPlots();
    if(correlations_) {
        trackCorrelationX_beforeCuts =
            new TH1F("trackCorrelationX_beforeCuts",
                     "Track residual X (tracks before cuts), all clusters;x_{track}-x_{hit} [#mum];# entries",
                     8000,
                     -1000.5,
                     999.5);
        trackCorrelationY_beforeCuts =
            new TH1F("trackCorrelationY_beforeCuts",
                     "Track residual Y (tracks before cuts), all clusters;y_{track}-y_{hit} [#mum];# entries",
                     8000,
                     -1000.5,
                     999.5);
        trackCorrelationTime_beforeCuts =
            new TH1F("trackCorrelationTime_beforeCuts",
                     "Track time residual (tracks before cuts), all clusters;time_{track}-time_{hit} [ns];# entries",
                     n_timebins_,
                     -(n_timebins_ + 1) / 2. * time_binning_,
                     (n_timebins_ - 1) / 2. * time_binning_);

        trackCorrelationX_afterCuts =
            new TH1F("trackCorrelationX_afterCuts",
                     "Track residual X (tracks after cuts), all clusters;x_{track}-x_{hit} [#mum];# entries",
                     8000,
                     -1000.5,
                     999.5);
        trackCorrelationY_afterCuts =
            new TH1F("trackCorrelationY_afterCuts",
                     "Track residual Y (tracks after cuts), all clusters;y_{track}-y_{hit} [#mum];# entries",
                     8000,
                     -1000.5,
                     999.5);
        trackCorrelationTime_afterCuts =
            new TH1F("trackCorrelationTime_afterCuts",
                     "Track time residual (tracks after cuts), all clusters;time_{track}-time_{hit} [ns];# entries",
                     n_timebins_,
                     -(n_timebins_ + 1) / 2. * time_binning_,
                     (n_timebins_ - 1) / 2. * time_binning_);
    }

    auto title = "local residual x of " + m_detector->getName() + " vs col; column; local residual x" +
                 m_detector->getName() + "[#mum]";
    resX_vs_col = new TH2F("residual_local_x_col",
                           title.c_str(),
                           m_detector->nPixels().X(),
                           -0.5,
                           m_detector->nPixels().X() - 0.5,
                           300,
                           -150,
                           150);
    title = "local residual y of " + m_detector->getName() + " vs col; column; local residual x" + m_detector->getName() +
            "[#mum]";
    resY_vs_col = new TH2F("residual_local_y_col",
                           title.c_str(),
                           m_detector->nPixels().X(),
                           -0.5,
                           m_detector->nPixels().X() - 0.5,
                           300,
                           -150,
                           150);

    title =
        "local residual x of " + m_detector->getName() + " vs row; row; local residual x" + m_detector->getName() + "[#mum]";
    resX_vs_row = new TH2F("resisdual_local_x_row",
                           title.c_str(),
                           m_detector->nPixels().Y(),
                           -0.5,
                           m_detector->nPixels().Y() - 0.5,
                           300,
                           -150,
                           150);
    title = "local residual y of " + m_detector->getName() + " vs row; row; local residual x " + m_detector->getName() +
            "[#mum]";
    resY_vs_row = new TH2F("residual_local_y_row",
                           title.c_str(),
                           m_detector->nPixels().Y(),
                           -0.5,
                           m_detector->nPixels().Y() - 0.5,
                           300,
                           -150,
                           150);

    hClusterMapAssoc = new TH2F("clusterMapAssoc",
                                "Map of associated clusters; cluster col; cluster row",
                                m_detector->nPixels().X(),
                                -0.5,
                                m_detector->nPixels().X() - 0.5,
                                m_detector->nPixels().Y(),
                                -0.5,
                                m_detector->nPixels().Y() - 0.5);
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "Size map for associated clusters;cluster column;cluster row;# entries",
                                          m_detector->nPixels().X(),
                                          -0.5,
                                          m_detector->nPixels().X() - 0.5,
                                          m_detector->nPixels().Y(),
                                          -0.5,
                                          m_detector->nPixels().Y() - 0.5,
                                          0,
                                          100);
    hClusterSizeVsColAssoc = new TProfile("clusterSizeVsColAssoc",
                                          "cluster size vs. column for assoc clusters;cluster column;mean cluster size",
                                          m_detector->nPixels().X(),
                                          -0.5,
                                          m_detector->nPixels().X() - 0.5,
                                          0,
                                          100);
    hClusterSizeVsRowAssoc = new TProfile("clusterSizeVsRowAssoc",
                                          "cluster size vs. row for assoc clusters;cluster row;mean cluster size",
                                          m_detector->nPixels().Y(),
                                          -0.5,
                                          m_detector->nPixels().Y() - 0.5,
                                          0,
                                          100);
    hClusterWidthColVsRowAssoc =
        new TProfile("clusterWidthColVsRowAssoc",
                     "cluster column width vs. row for assoc clusters;cluster row;mean cluster column width",
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5,
                     0,
                     100);
    hClusterWidthRowVsRowAssoc =
        new TProfile("clusterWidthRowVsRowAssoc",
                     "cluster row width vs. row for assoc clusters;cluster row;mean cluster row width",
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5,
                     0,
                     100);
    hClusterChargeMapAssoc =
        new TProfile2D("clusterChargeMapAssoc",
                       "Charge map for associated clusters; cluster column; cluster row;cluster charge [a.u.]",
                       m_detector->nPixels().X(),
                       -0.5,
                       m_detector->nPixels().X() - 0.5,
                       m_detector->nPixels().Y(),
                       -0.5,
                       m_detector->nPixels().Y() - 0.5,
                       0.0,
                       charge_histo_range_);

    hClusterChargeVsColAssoc =
        new TProfile("clusterChargeVsColAssoc",
                     "cluster charge vs. column for assoc clusters;cluster column;mean cluster charge [a.u.]",
                     m_detector->nPixels().X(),
                     -0.5,
                     m_detector->nPixels().X() - 0.5,
                     0.0,
                     charge_histo_range_);
    hClusterChargeVsRowAssoc =
        new TProfile("clusterChargeVsRowAssoc",
                     "cluster charge vs. row for assoc clusters;cluster row;mean cluster charge [a.u.]",
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5,
                     0.0,
                     charge_histo_range_);

    hSeedChargeVsColAssoc =
        new TProfile("seedChargeVsColAssoc",
                     "seed pixel charge vs. column for assoc clusters;cluster column;mean seed pixel charge [a.u.]",
                     m_detector->nPixels().X(),
                     -0.5,
                     m_detector->nPixels().X() - 0.5,
                     0.0,
                     charge_histo_range_);
    hSeedChargeVsRowAssoc =
        new TProfile("seedChargeVsRowAssoc",
                     "seed pixel charge vs. row for assoc clusters;cluster row;mean seed pixel charge [a.u.]",
                     m_detector->nPixels().Y(),
                     -0.5,
                     m_detector->nPixels().Y() - 0.5,
                     0.0,
                     charge_histo_range_);
    hClusterChargeVsRowAssoc_2D = new TH2F("hClusterChargeVsRowAssoc_2D",
                                           "cluster charge vs. cluster row; cluster row; cluster charge [a.u.]",
                                           m_detector->nPixels().Y(),
                                           -0.5,
                                           m_detector->nPixels().Y() - 0.5,
                                           n_chargebins_,
                                           0.0,
                                           charge_histo_range_);
    hSeedChargeVsRowAssoc_2D = new TH2F("hSeedChargeVsRowAssoc_2D",
                                        "seed charge vs. cluster row; cluster row; seed pixel charge [a.u.]",
                                        m_detector->nPixels().Y(),
                                        -0.5,
                                        m_detector->nPixels().Y() - 0.5,
                                        n_chargebins_,
                                        0.0,
                                        charge_histo_range_);

    hTrackZPosDUT = new TH1F("globalTrackZPosOnDUT",
                             "Global z-position of track on the DUT; global z of track intersection [mm]; # entries ",
                             400,
                             m_detector->displacement().z() - 10,
                             m_detector->displacement().z() + 10);
    // Per-pixel histograms
    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "Hit map of pixels from associated clusters; hit column; hit row",
                            m_detector->nPixels().X(),
                            -0.5,
                            m_detector->nPixels().X() - 0.5,
                            m_detector->nPixels().Y(),
                            -0.5,
                            m_detector->nPixels().Y() - 0.5);
    hPixelRawValueAssoc = new TH1F("pixelRawValueAssoc",
                                   "Charge distribution of pixels from associated clusters;pixel raw value;# entries",
                                   n_chargebins_,
                                   0.0,
                                   raw_histo_range_);
    hPixelRawValueMapAssoc = new TProfile2D("pixelRawValueMapAssoc",
                                            "Charge map of pixels from associated clusters;pixel raw values;# entries",
                                            m_detector->nPixels().X(),
                                            -0.5,
                                            m_detector->nPixels().X() - 0.5,
                                            m_detector->nPixels().Y(),
                                            -0.5,
                                            m_detector->nPixels().Y() - 0.5,
                                            0.0,
                                            raw_histo_range_);

    associatedTracksVersusTime =
        new TH1F("associatedTracksVersusTime", "Associated tracks over time;time [s];# associated tracks", 300000, 0, 300);

    clusterChargeAssoc = new TH1F("clusterChargeAssociated",
                                  "Charge distribution of associated clusters;cluster charge [a.u.];# entries",
                                  n_chargebins_,
                                  0.0,
                                  charge_histo_range_);
    seedChargeAssoc =
        new TH1F("seedChargeAssociated",
                 "Charge distribution of seed pixels for associated clusters;seed pixel charge [a.u.];# entries",
                 n_chargebins_,
                 0.0,
                 charge_histo_range_);
    clusterSizeAssoc = new TH1F(
        "clusterSizeAssociated", "Size distribution of associated clusters;cluster size; # entries", 30, -0.5, 29.5);
    clusterSizeAssocNorm =
        new TH1F("clusterSizeAssociatedNormalized",
                 "Normalized size distribution of associated clusters;cluster size;# entries (normalized)",
                 30,
                 0,
                 30);
    clusterWidthRowAssoc = new TH1F("clusterWidthRowAssociated",
                                    "Height distribution of associated clusters (rows);cluster size row; # entries",
                                    30,
                                    -0.5,
                                    29.5);
    clusterWidthColAssoc = new TH1F("clusterWidthColAssociated",
                                    "Width distribution of associated clusters (columns);cluster size col; # entries",
                                    30,
                                    -0.5,
                                    29.5);

    // In-pixel studies:
    auto pitch_x = m_detector->getPitch().X() * 1000.; // convert mm -> um
    auto pitch_y = m_detector->getPitch().Y() * 1000.; // convert mm -> um
    std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

    // cut flow histogram
    title = m_detector->getName() + ": number of tracks discarded by different cuts;cut type;tracks";
    hCutHisto = new TH1F("hCutHisto", title.c_str(), 4, 1, 5);
    hCutHisto->GetXaxis()->SetBinLabel(1, "High Chi2");
    hCutHisto->GetXaxis()->SetBinLabel(2, "Outside DUT area");
    hCutHisto->GetXaxis()->SetBinLabel(3, "Close to masked pixel");
    hCutHisto->GetXaxis()->SetBinLabel(4, "Close to frame begin/end");

    title = "Resolution in X;" + mod_axes + "MAD(#Deltax) [#mum]";
    rmsxvsxmym = new TProfile2D("rmsxvsxmym",
                                title.c_str(),
                                static_cast<int>(pitch_x),
                                -pitch_x / 2.,
                                pitch_x / 2.,
                                static_cast<int>(pitch_y),
                                -pitch_y / 2.,
                                pitch_y / 2.);

    title = "Resolution in Y;" + mod_axes + "MAD(#Deltay) [#mum]";
    rmsyvsxmym = new TProfile2D("rmsyvsxmym",
                                title.c_str(),
                                static_cast<int>(pitch_x),
                                -pitch_x / 2.,
                                pitch_x / 2.,
                                static_cast<int>(pitch_y),
                                -pitch_y / 2.,
                                pitch_y / 2.);

    title = "Resolution;" + mod_axes + "MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    rmsxyvsxmym = new TProfile2D("rmsxyvsxmym",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.);

    title = "Mean cluster charge map;" + mod_axes + "<cluster charge> [a.u.]";
    qvsxmym = new TProfile2D("qvsxmym",
                             title.c_str(),
                             static_cast<int>(pitch_x),
                             -pitch_x / 2.,
                             pitch_x / 2.,
                             static_cast<int>(pitch_y),
                             -pitch_y / 2.,
                             pitch_y / 2.,
                             0.0,
                             charge_histo_range_);

    title = "Most probable cluster charge map, Moyal approx.;" + mod_axes + "cluster charge MPV [a.u.]";
    qMoyalvsxmym = new TProfile2D("qMoyalvsxmym",
                                  title.c_str(),
                                  static_cast<int>(pitch_x),
                                  -pitch_x / 2.,
                                  pitch_x / 2.,
                                  static_cast<int>(pitch_y),
                                  -pitch_y / 2.,
                                  pitch_y / 2.,
                                  0.0,
                                  charge_histo_range_);

    title = "Seed pixel charge map;" + mod_axes + "<seed pixel charge> [a.u.]";
    pxqvsxmym = new TProfile2D("pxqvsxmym",
                               title.c_str(),
                               static_cast<int>(pitch_x),
                               -pitch_x / 2.,
                               pitch_x / 2.,
                               static_cast<int>(pitch_y),
                               -pitch_y / 2.,
                               pitch_y / 2.,
                               0.0,
                               charge_histo_range_);

    title = "Mean cluster size map;" + mod_axes + "<pixels/cluster>";
    npxvsxmym = new TProfile2D("npxvsxmym",
                               title.c_str(),
                               static_cast<int>(pitch_x),
                               -pitch_x / 2.,
                               pitch_x / 2.,
                               static_cast<int>(pitch_y),
                               -pitch_y / 2.,
                               pitch_y / 2.,
                               0,
                               4.5);

    title = "1-pixel cluster map;" + mod_axes + "clusters";
    npx1vsxmym = new TH2F("npx1vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "2-pixel cluster map;" + mod_axes + "clusters";
    npx2vsxmym = new TH2F("npx2vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "3-pixel cluster map;" + mod_axes + "clusters";
    npx3vsxmym = new TH2F("npx3vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "4-pixel cluster map;" + mod_axes + "clusters";
    npx4vsxmym = new TH2F("npx4vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "Mean cluster charge map (1-pixel);" + mod_axes + "<cluster charge> [a.u.]";
    qvsxmym_1px = new TProfile2D("qvsxmym_1px",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.,
                                 0.0,
                                 charge_histo_range_);

    title = "Mean cluster charge map (2-pixel);" + mod_axes + "<cluster charge> [a.u.]";
    qvsxmym_2px = new TProfile2D("qvsxmym_2px",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.,
                                 0.0,
                                 charge_histo_range_);

    title = "Mean cluster charge map (3-pixel);" + mod_axes + "<cluster charge> [a.u.]";
    qvsxmym_3px = new TProfile2D("qvsxmym_3px",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.,
                                 0.0,
                                 charge_histo_range_);

    title = "Mean cluster charge map (4-pixel);" + mod_axes + "<cluster charge> [a.u.]";
    qvsxmym_4px = new TProfile2D("qvsxmym_4px",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.,
                                 0.0,
                                 charge_histo_range_);

    residualsTime = new TH1F("residualsTime",
                             "Time residual;time_{track}-time_{hit} [ns];# entries",
                             n_timebins_,
                             -(n_timebins_ + 1) / 2. * time_binning_,
                             (n_timebins_ - 1) / 2. * time_binning_);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime",
                                   "Time residual vs. time;time [s];time_{track}-time_{hit} [ns];# entries",
                                   20000,
                                   0,
                                   200,
                                   n_timebins_,
                                   -(n_timebins_ + 1) / 2. * time_binning_,
                                   (n_timebins_ - 1) / 2. * time_binning_);
    residualsTimeVsTot =
        new TH2F("residualsTimeVsTot",
                 "Pixel charge vs. time residual;time_{track} - time_{hit} [ns];seed pixel raw value;# entries",
                 n_timebins_,
                 -(n_timebins_ + 1) / 2. * time_binning_,
                 (n_timebins_ - 1) / 2. * time_binning_,
                 n_rawbins_,
                 0.0,
                 raw_histo_range_);

    residualsTimeVsCol =
        new TH2F("residualsTimeVsCol",
                 "Seed pixel column vs. time residual;time_{track} - time_{hit} [ns];seed pixel column;# entries",
                 n_timebins_,
                 -(n_timebins_ + 1) / 2. * time_binning_,
                 (n_timebins_ - 1) / 2. * time_binning_,
                 m_detector->nPixels().X(),
                 -0.5,
                 m_detector->nPixels().X() - 0.5);

    residualsTimeVsRow = new TH2F("residualsTimeVsRow",
                                  "Seed pixel row vs. time residual;time_{track} - time_{hit} [ns];seed pixel row;# entries",
                                  n_timebins_,
                                  -(n_timebins_ + 1) / 2. * time_binning_,
                                  (n_timebins_ - 1) / 2. * time_binning_,
                                  m_detector->nPixels().X(),
                                  -0.5,
                                  m_detector->nPixels().X() - 0.5);

    residualsTimeVsSignal =
        new TH2F("residualsTimeVsSignal",
                 "Cluster charge vs. time residual;time_{track}-time_{hit} [mm];cluster charge [a.u.];# entries",
                 n_timebins_,
                 -(n_timebins_ + 1) / 2. * time_binning_,
                 (n_timebins_ - 1) / 2. * time_binning_,
                 n_chargebins_,
                 0.0,
                 charge_histo_range_);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition",
                 "Map of associated track positions (global);global intercept x [mm];global intercept y [mm]",
                 400,
                 -20,
                 20,
                 200,
                 -10,
                 10);
    hAssociatedTracksLocalPosition =
        new TH2F("hAssociatedTracksLocalPosition",
                 "Map of associated track positions (local);local intercept x [px];local intercept y [px]",
                 10 * m_detector->nPixels().X(),
                 -0.5,
                 m_detector->nPixels().X() - 0.5,
                 10 * m_detector->nPixels().Y(),
                 -0.5,
                 m_detector->nPixels().Y() - 0.5);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition",
                 "Map of not associated track positions (global); global intercept x [mm]; global intercept y [mm]",
                 200,
                 -10,
                 10,
                 200,
                 -10,
                 10);

    pxTimeMinusSeedTime =
        new TH1F("pxTimeMinusSeedTime",
                 "pixel - seed pixel timestamp (all pixels w/o seed);ts_{pixel} - ts_{seed} [ns];# entries",
                 n_timebins_,
                 -(n_timebins_ + 1) / 2. * time_binning_,
                 (n_timebins_ - 1) / 2. * time_binning_);
    pxTimeMinusSeedTime_vs_pxCharge = new TH2F(
        "pxTimeMinusSeedTime_vs_pxCharge",
        "pixel - seed pixel timestamp (all pixels w/o seed);ts_{pixel} - ts_{seed} [ns]; pixel charge [a.u.];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        n_chargebins_,
        0.0,
        charge_histo_range_);
    pxTimeMinusSeedTime_vs_pxCharge_2px = new TH2F(
        "pxTimeMinusSeedTime_vs_pxCharge_2px",
        "pixel - seed pixel timestamp (all pixels w/o seed);ts_{pixel} - ts_{seed} [ns]; pixel charge [a.u.];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        n_chargebins_,
        0.0,
        charge_histo_range_);
    pxTimeMinusSeedTime_vs_pxCharge_3px = new TH2F(
        "pxTimeMinusSeedTime_vs_pxCharge_3px",
        "pixel - seed pixel timestamp (all pixels w/o seed);ts_{pixel} - ts_{seed} [ns]; pixel charge [a.u.];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        n_chargebins_,
        0.0,
        charge_histo_range_);
    pxTimeMinusSeedTime_vs_pxCharge_4px = new TH2F(
        "pxTimeMinusSeedTime_vs_pxCharge_4px",
        "pixel - seed pixel timestamp (all pixels w/o seed);ts_{pixel} - ts_{seed} [ns]; pixel charge [a.u.];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        n_chargebins_,
        0.0,
        charge_histo_range_);

    track_trackDistance = new TH2F("track_to_track_distance",
                                   "Local track to track distance;#Delta_x [#mum]; #Delta_y [#mum]",
                                   800,
                                   -1000,
                                   1000,
                                   800,
                                   -1000,
                                   1000);

    auto nbins_x = static_cast<int>(std::ceil(m_detector->getPitch().X() / inpixelBinSize_));
    auto nbins_y = static_cast<int>(std::ceil(m_detector->getPitch().Y() / inpixelBinSize_));
    if(nbins_x > 1e4 || nbins_y > 1e4) {
        throw InvalidValueError(config_, "inpixel_bin_size", "Too many bins for in-pixel histograms.");
    }

    title =
        m_detector->getName() + "in pixel cluster size map;in-pixel x_{track} [#mum];in-pixel y_{track} #mum;cluster size";
    hclusterSize_trackPos_TProfile = new TProfile2D("clusterSize_trackPos_TProfile",
                                                    title.c_str(),
                                                    nbins_x,
                                                    -pitch_x / 2.,
                                                    pitch_x / 2.,
                                                    nbins_y,
                                                    -pitch_y / 2.,
                                                    pitch_y / 2.,
                                                    -500,
                                                    500);
    title = m_detector->getName() +
            "in pixel time resolution map;in-pixel x_{track} [#mum];in-pixel y_{track} #mum;#sigma time difference [ns]";
    htimeRes_trackPos_TProfile = new TH2D("timeRes_trackPos_TProfile",
                                          title.c_str(),
                                          nbins_x,
                                          -pitch_x / 2.,
                                          pitch_x / 2.,
                                          nbins_y,
                                          -pitch_y / 2.,
                                          pitch_y / 2.);
    title = m_detector->getName() + "in pixel time difference map;in-pixel x_{track} [#mum];in-pixel y_{track} #mum;time "
                                    "difference: track - cluster [ns]";
    htimeDelay_trackPos_TProfile =
        new TProfile2D("timeDelay_trackPos_TProfile",
                       title.c_str(),
                       nbins_x,
                       -pitch_x / 2.,
                       pitch_x / 2.,
                       nbins_y,
                       -pitch_y / 2.,
                       pitch_y / 2.,
                       -500,
                       500,
                       "s"); // standard deviation as the error on a bin, convienent for time resolution
}

StatusCode AnalysisDUT::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();

    // Loop over all tracks
    for(auto& track : tracks) {
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        // Fill correlation plots BEFORE applying any cuts:
        if(correlations_) {
            auto clusters = clipboard->getData<Cluster>(m_detector->getName());
            for(auto& cls : clusters) {
                double xdistance_um = (globalIntercept.X() - cls->global().x()) * 1000.;
                double ydistance_um = (globalIntercept.Y() - cls->global().y()) * 1000.;
                trackCorrelationX_beforeCuts->Fill(xdistance_um);
                trackCorrelationY_beforeCuts->Fill(ydistance_um);
                trackCorrelationTime_beforeCuts->Fill(track->timestamp() - cls->timestamp());
            }
        }

        // Flags to select clusters and tracks
        bool has_associated_cluster = false;
        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > chi2_ndof_cut_) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            hCutHisto->Fill(1);
            num_tracks_++;
            continue;
        }

        // Create track-to-track plot
        for(auto& track2 : tracks) {
            if((track == track2) || (track2->getChi2ndof() > chi2_ndof_cut_)) {
                continue;
            }
            auto inter1 = m_detector->globalToLocal(m_detector->getIntercept(track.get()));
            auto inter2 = m_detector->globalToLocal(m_detector->getIntercept(track2.get()));
            track_trackDistance->Fill(1000. * (inter1.x() - inter2.x()), 1000. * (inter1.y() - inter2.y()));
        }

        // Check if it intercepts the DUT
        if(!m_detector->hasIntercept(track.get(), spatial_cut_sensoredge_)) {
            LOG(DEBUG) << " - track outside DUT area";
            hCutHisto->Fill(2);
            num_tracks_++;
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track.get())) {
            continue;
        }

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track.get(), 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            hCutHisto->Fill(3);
            num_tracks_++;
            continue;
        }

        // Fill correlation plots after applying cuts:
        if(correlations_) {
            auto clusters = clipboard->getData<Cluster>(m_detector->getName());
            for(auto& cls : clusters) {
                double xdistance_um = (globalIntercept.X() - cls->global().x()) * 1000.;
                double ydistance_um = (globalIntercept.Y() - cls->global().y()) * 1000.;
                trackCorrelationX_afterCuts->Fill(xdistance_um);
                trackCorrelationY_afterCuts->Fill(ydistance_um);
                trackCorrelationTime_afterCuts->Fill(track->timestamp() - cls->timestamp());
            }
        }

        // Get the event:
        auto event = clipboard->getEvent();

        // Discard tracks which are very close to the frame edges
        if(fabs(track->timestamp() - event->end()) < time_cut_frameedge_) {
            // Late edge - eventEnd points to the end of the frame`
            LOG(DEBUG) << " - track close to end of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->end()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            hCutHisto->Fill(4);
            num_tracks_++;
            continue;
        } else if(fabs(track->timestamp() - event->start()) < time_cut_frameedge_) {
            // Early edge - eventStart points to the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            hCutHisto->Fill(4);
            num_tracks_++;
            continue;
        }

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod_um = inpixel.X() * 1000.; // convert mm -> um
        auto ymod_um = inpixel.Y() * 1000.; // convert mm -> um

        // Loop over all associated DUT clusters:
        for(auto assoc_cluster : track->getAssociatedClusters(m_detector->getName())) {
            LOG(DEBUG) << " - Looking at next associated DUT cluster";

            // if closest cluster should be used continue if current associated cluster is not the closest one
            if(use_closest_cluster_ && track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                continue;
            }
            has_associated_cluster = true;
            htimeDelay_trackPos_TProfile->Fill(xmod_um, ymod_um, (track->timestamp() - assoc_cluster->timestamp()));
            hclusterSize_trackPos_TProfile->Fill(xmod_um, ymod_um, static_cast<double>(assoc_cluster->size()));

            hTrackZPosDUT->Fill(track->getState(m_detector->getName()).z());

            // Fill per-pixel histograms
            for(auto& pixel : assoc_cluster->pixels()) {
                hHitMapAssoc->Fill(pixel->column(), pixel->row());
                hPixelRawValueAssoc->Fill(pixel->raw());
                hPixelRawValueMapAssoc->Fill(pixel->column(), pixel->row(), pixel->raw());
            }

            // Cluster and Charge Plots

            auto cluster_charge = assoc_cluster->charge();
            clusterChargeAssoc->Fill(cluster_charge);
            seedChargeAssoc->Fill(assoc_cluster->getSeedPixel()->charge());
            hClusterChargeMapAssoc->Fill(assoc_cluster->column(), assoc_cluster->row(), cluster_charge);
            hClusterChargeVsColAssoc->Fill(assoc_cluster->column(), cluster_charge);
            hClusterChargeVsRowAssoc->Fill(assoc_cluster->row(), cluster_charge);
            hSeedChargeVsColAssoc->Fill(assoc_cluster->column(), assoc_cluster->getSeedPixel()->charge());
            hSeedChargeVsRowAssoc->Fill(assoc_cluster->row(), assoc_cluster->getSeedPixel()->charge());
            hClusterChargeVsRowAssoc_2D->Fill(assoc_cluster->row(), cluster_charge);
            hSeedChargeVsRowAssoc_2D->Fill(assoc_cluster->row(), assoc_cluster->getSeedPixel()->charge());

            clusterSizeAssoc->Fill(static_cast<double>(assoc_cluster->size()));
            clusterSizeAssocNorm->Fill(static_cast<double>(assoc_cluster->size()));
            clusterWidthRowAssoc->Fill(static_cast<double>(assoc_cluster->rowWidth()));
            clusterWidthColAssoc->Fill(static_cast<double>(assoc_cluster->columnWidth()));

            hClusterMapAssoc->Fill(assoc_cluster->column(), assoc_cluster->row());
            hClusterSizeMapAssoc->Fill(
                assoc_cluster->column(), assoc_cluster->row(), static_cast<double>(assoc_cluster->size()));
            hClusterSizeVsColAssoc->Fill(assoc_cluster->column(), static_cast<double>(assoc_cluster->size()));
            hClusterSizeVsRowAssoc->Fill(assoc_cluster->row(), static_cast<double>(assoc_cluster->size()));
            hClusterWidthColVsRowAssoc->Fill(assoc_cluster->row(), static_cast<double>(assoc_cluster->columnWidth()));
            hClusterWidthRowVsRowAssoc->Fill(assoc_cluster->row(), static_cast<double>(assoc_cluster->rowWidth()));

            associatedTracksVersusTime->Fill(track->timestamp() / 1e9); // convert ns -> s

            // Check distance between track and cluster in local coordinates
            ROOT::Math::XYZPoint intercept = m_detector->getLocalIntercept(track.get());
            double local_x_distance = intercept.X() - assoc_cluster->local().x();
            double local_y_distance = intercept.Y() - assoc_cluster->local().y();
            double local_x_distance_um = local_x_distance * 1000.;
            double local_y_distance_um = local_y_distance * 1000.;

            double local_x_absdistance = fabs(local_x_distance);
            double local_y_absdistance = fabs(local_y_distance);
            double time_distance = track->timestamp() - assoc_cluster->timestamp();
            double local_pos_diff = sqrt(local_x_distance * local_x_absdistance + local_y_absdistance * local_y_absdistance);
            double local_pos_diff_um = local_pos_diff * 1000.;

            resX_vs_col->Fill(assoc_cluster->column(), local_x_distance_um);
            resY_vs_col->Fill(assoc_cluster->column(), local_y_distance_um);
            resX_vs_row->Fill(assoc_cluster->row(), local_x_distance_um);
            resY_vs_row->Fill(assoc_cluster->row(), local_y_distance_um);
            // Cluster charge normalized to path length in sensor:
            double norm = 1; // FIXME fabs(cos( turn*wt )) * fabs(cos( tilt*wt ));
            // FIXME: what does this mean? To my understanding we have the correct charge here already...
            auto normalized_charge = cluster_charge * norm;

            // clusterChargeAssoc->Fill(normalized_charge);

            // Residuals
            residualsX_local->Fill(local_x_distance_um);
            residualsY_local->Fill(local_y_distance_um);
            residualsPos_local->Fill(local_pos_diff_um);
            residualsPosVsresidualsTime_local->Fill(time_distance, local_pos_diff_um);

            // fill resolution depending on cluster size (max 4, everything higher in last plot)
            (assoc_cluster->columnWidth() < residualsXclusterColLocal.size())
                ? residualsXclusterColLocal.at(assoc_cluster->columnWidth() - 1)->Fill(local_x_distance_um)
                : residualsXclusterColLocal.back()->Fill(local_x_distance_um);
            (assoc_cluster->rowWidth() < residualsYclusterRowLocal.size())
                ? residualsYclusterRowLocal.at(assoc_cluster->rowWidth() - 1)->Fill(local_y_distance_um)
                : residualsYclusterRowLocal.back()->Fill(local_y_distance_um);

            // Global residuals

            ROOT::Math::XYZPoint global_lintercept = m_detector->getIntercept(track.get());
            double global_x_distance = global_lintercept.X() - assoc_cluster->global().x();
            double global_y_distance = global_lintercept.Y() - assoc_cluster->global().y();
            double global_x_distance_um = global_x_distance * 1000.;
            double global_y_distance_um = global_y_distance * 1000.;
            double global_x_absdistance = fabs(global_x_distance);
            double global_y_absdistance = fabs(global_y_distance);
            double global_pos_diff =
                sqrt(global_x_distance * global_x_absdistance + global_y_absdistance * global_y_absdistance);
            double global_pos_diff_um = global_pos_diff * 1000.;

            residualsX_global->Fill(global_x_distance_um);
            residualsY_global->Fill(global_y_distance_um);
            residualsPos_global->Fill(global_pos_diff_um);
            residualsPosVsresidualsTime_global->Fill(time_distance, global_pos_diff_um);

            (assoc_cluster->columnWidth() < residualsXclusterColGlobal.size())
                ? residualsXclusterColGlobal.at(assoc_cluster->columnWidth() - 1)->Fill(global_x_distance_um)
                : residualsXclusterColGlobal.back()->Fill(global_x_distance_um);
            (assoc_cluster->rowWidth() < residualsYclusterRowGlobal.size())
                ? residualsYclusterRowGlobal.at(assoc_cluster->rowWidth() - 1)->Fill(global_y_distance_um)
                : residualsYclusterRowGlobal.back()->Fill(global_y_distance_um);

            (assoc_cluster->columnWidth() < residualsYclusterColGlobal.size())
                ? residualsYclusterColGlobal.at(assoc_cluster->columnWidth() - 1)->Fill(global_y_distance_um)
                : residualsYclusterColGlobal.back()->Fill(global_y_distance_um);
            (assoc_cluster->rowWidth() < residualsXclusterRowGlobal.size())
                ? residualsXclusterRowGlobal.at(assoc_cluster->rowWidth() - 1)->Fill(global_x_distance_um)
                : residualsXclusterRowGlobal.back()->Fill(global_x_distance_um);

            // residual MAD x, y, combined (sqrt(x*x + y*y))- this we also need as global and local? tricky
            rmsxvsxmym->Fill(xmod_um, ymod_um, local_x_absdistance);
            rmsyvsxmym->Fill(xmod_um, ymod_um, local_y_absdistance);
            rmsxyvsxmym->Fill(
                xmod_um, ymod_um, fabs(sqrt(local_x_distance * local_x_distance + local_y_distance * local_y_distance)));

            // Time residuals
            residualsTime->Fill(time_distance);
            residualsTimeVsTime->Fill(track->timestamp() / 1e9, time_distance); // convert ns -> s
            residualsTimeVsTot->Fill(time_distance, assoc_cluster->getSeedPixel()->raw());
            residualsTimeVsCol->Fill(time_distance, assoc_cluster->getSeedPixel()->column());
            residualsTimeVsRow->Fill(time_distance, assoc_cluster->getSeedPixel()->row());
            residualsTimeVsSignal->Fill(time_distance, cluster_charge);

            // Fill in-pixel plots: (all as function of track position within pixel cell)
            qvsxmym->Fill(xmod_um, ymod_um, cluster_charge); // cluster charge profile

            // FIXME: Moyal distribution (https://reference.wolfram.com/language/ref/MoyalDistribution.html) of the cluster
            // charge
            qMoyalvsxmym->Fill(xmod_um, ymod_um, exp(-normalized_charge / 3.5)); // norm. cluster charge profile

            // mean charge of cluster seed
            pxqvsxmym->Fill(xmod_um, ymod_um, assoc_cluster->getSeedPixel()->charge());

            if(assoc_cluster->size() > 1) {
                for(auto& px : assoc_cluster->pixels()) {
                    if(px == assoc_cluster->getSeedPixel()) {
                        continue; // don't fill this histogram for seed pixel!
                    }
                    pxTimeMinusSeedTime->Fill(static_cast<double>(
                        Units::convert(px->timestamp() - assoc_cluster->getSeedPixel()->timestamp(), "ns")));
                    pxTimeMinusSeedTime_vs_pxCharge->Fill(
                        static_cast<double>(
                            Units::convert(px->timestamp() - assoc_cluster->getSeedPixel()->timestamp(), "ns")),
                        px->charge());

                    if(assoc_cluster->size() == 2) {
                        pxTimeMinusSeedTime_vs_pxCharge_2px->Fill(
                            static_cast<double>(
                                Units::convert(px->timestamp() - assoc_cluster->getSeedPixel()->timestamp(), "ns")),
                            px->charge());
                    } else if(assoc_cluster->size() == 3) {
                        pxTimeMinusSeedTime_vs_pxCharge_3px->Fill(
                            static_cast<double>(
                                Units::convert(px->timestamp() - assoc_cluster->getSeedPixel()->timestamp(), "ns")),
                            px->charge());
                    } else if(assoc_cluster->size() == 4) {
                        pxTimeMinusSeedTime_vs_pxCharge_4px->Fill(
                            static_cast<double>(
                                Units::convert(px->timestamp() - assoc_cluster->getSeedPixel()->timestamp(), "ns")),
                            px->charge());
                    }
                }
            }

            // mean cluster size
            npxvsxmym->Fill(xmod_um, ymod_um, static_cast<double>(assoc_cluster->size()));
            if(assoc_cluster->size() == 1) {
                npx1vsxmym->Fill(xmod_um, ymod_um);
                qvsxmym_1px->Fill(xmod_um, ymod_um, cluster_charge);
            }
            if(assoc_cluster->size() == 2) {
                npx2vsxmym->Fill(xmod_um, ymod_um);
                qvsxmym_2px->Fill(xmod_um, ymod_um, cluster_charge);
            }
            if(assoc_cluster->size() == 3) {
                npx3vsxmym->Fill(xmod_um, ymod_um);
                qvsxmym_3px->Fill(xmod_um, ymod_um, cluster_charge);
            }
            if(assoc_cluster->size() == 4) {
                npx4vsxmym->Fill(xmod_um, ymod_um);
                qvsxmym_4px->Fill(xmod_um, ymod_um, cluster_charge);
            }

            hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
            hAssociatedTracksLocalPosition->Fill(m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept));
        }
        if(!has_associated_cluster) {
            hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
        }
        num_tracks_++;
    }
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisDUT::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    hCutHisto->Scale(1 / double(num_tracks_));
    clusterSizeAssocNorm->Scale(1 / clusterSizeAssoc->Integral());
    // do the timing profile:
    for(auto x = 0; x < htimeDelay_trackPos_TProfile->GetNbinsX(); ++x) {
        for(auto y = 0; y < htimeDelay_trackPos_TProfile->GetNbinsY(); ++y) {
            htimeRes_trackPos_TProfile->SetBinContent(x, y, htimeDelay_trackPos_TProfile->GetBinError(x, y));
            // LOG(STATUS) << x <<'\t' << y << "\t" << htimeDelay_trackPos_TProfile->GetBinError(x,y);
        }
    }
    htimeDelay_trackPos_TProfile->Write();
    htimeRes_trackPos_TProfile->Write();
}

void AnalysisDUT::createGlobalResidualPlots() {
    TDirectory* directory = getROOTDirectory();
    TDirectory* local_directory = directory->mkdir("global_residuals");
    if(local_directory == nullptr) {
        throw RuntimeError("Cannot create or access global ROOT directory for module " + this->getUniqueName());
    }
    local_directory->cd();

    residualsX_global =
        new TH1F("residualsX", "Residual in global X;x_{track}-x_{hit}  [#mum];# entries", 4000, -500.5, 499.5);
    residualsY_global =
        new TH1F("residualsY", "Residual in global Y;y_{track}-y_{hit}  [#mum];# entries", 4000, -500.5, 499.5);
    residualsPos_global =
        new TH1F("residualsPos",
                 "Absolute distance between track and hit in global coordinates;|pos_{track}-pos_{hit}|  [#mum];# entries",
                 4000,
                 -500.5,
                 499.5);
    residualsPosVsresidualsTime_global = new TH2F(
        "residualsPosVsresidualsTime",
        "Absolute global position residuals vs. time;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [#mum];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        800,
        -0.25,
        199.75);

    for(int i = 1; i <= 5; ++i) {
        auto name = (i <= 4 ? std::to_string(i) : ("atLeast5"));
        residualsXclusterColGlobal.push_back(new TH1F(
            ("residualsX" + name + "pixCol").c_str(),
            ("Residual for " + name + "-pixel clusters along column in global X;x_{track}-x_{hit} [#mum];# entries").c_str(),
            4000,
            -500.5,
            499.5));

        residualsXclusterRowGlobal.push_back(new TH1F(
            ("residualsX" + name + "pixRow").c_str(),
            ("Residual for " + name + "-pixel clusters along row in global X;x_{track}-x_{hit} [#mum];# entries").c_str(),
            4000,
            -500.5,
            499.5));

        residualsYclusterColGlobal.push_back(new TH1F(
            ("residualsY" + name + "pixCol").c_str(),
            ("Residual for " + name + "-pixel clusters along column in global Y;y_{track}-y_{hit} [#mum];# entries").c_str(),
            4000,
            -500.5,
            499.5));

        residualsYclusterRowGlobal.push_back(new TH1F(
            ("residualsY" + name + "pixRow").c_str(),
            ("Residual for " + name + "-pixel clusters along row in global Y;y_{track}-y_{hit} [#mum];# entries").c_str(),
            4000,
            -500.5,
            499.5));
    }

    directory->cd();
}

void AnalysisDUT::createLocalResidualPlots() {
    TDirectory* directory = getROOTDirectory();
    TDirectory* local_directory = directory->mkdir("local_residuals");
    if(local_directory == nullptr) {
        throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
    }
    local_directory->cd();

    residualsX_local =
        new TH1F("residualsX", "Residual in local X;x_{track}-x_{hit}  [#mum];# entries", 4000, -500.5, 499.5);
    residualsY_local =
        new TH1F("residualsY", "Residual in local Y;y_{track}-y_{hit}  [#mum];# entries", 4000, -500.5, 499.5);
    residualsPos_local =
        new TH1F("residualsPos",
                 "Absolute distance between track and hit in local coordinates;|pos_{track}-pos_{hit}|  [#mum];# entries",
                 4000,
                 -500.5,
                 499.5);
    residualsPosVsresidualsTime_local = new TH2F(
        "residualsPosVsresidualsTime",
        "Absolute local position residuals vs. time;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [#mum];# entries",
        n_timebins_,
        -(n_timebins_ + 1) / 2. * time_binning_,
        (n_timebins_ - 1) / 2. * time_binning_,
        800,
        -0.25,
        199.75);

    for(int i = 1; i <= 5; ++i) {
        auto name = (i <= 4 ? std::to_string(i) : ("atLeast5"));
        residualsXclusterColLocal.push_back(
            new TH1F(("residualsX" + name + "pix").c_str(),
                     ("Residual for " + name + "-pixel clusters in local X;x_{track}-x_{hit} [#mum];# entries").c_str(),
                     4000,
                     -500.5,
                     499.5));
        residualsYclusterRowLocal.push_back(
            new TH1F(("residualsY" + name + "pix").c_str(),
                     ("Residual for " + name + "-pixel clusters in local Y;y_{track}-y_{hit} [#mum];# entries").c_str(),
                     4000,
                     -500.5,
                     499.5));
    }

    directory->cd();
}
