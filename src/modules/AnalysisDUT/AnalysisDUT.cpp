/**
 * @file
 * @brief Implementation of module AnalysisDUT
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
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
    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<bool>("use_closest_cluster", true);
    config_.setDefault<bool>("n_time_bins", 20000);
    config_.setDefault<bool>("time_binning", Units::get<double>(0.1, "ns"));

    m_timeCutFrameEdge = config_.get<double>("time_cut_frameedge");
    chi2ndofCut = config_.get<double>("chi2ndof_cut");
    useClosestCluster = config_.get<bool>("use_closest_cluster");
    nTimeBins = config_.get<double>("n_time_bins");
    timeBinning = config_.get<double>("time_binning");
}

void AnalysisDUT::initialize() {

    hClusterMapAssoc = new TH2F("clusterMapAssoc",
                                "Map of associated clusters; cluster col; cluster row",
                                m_detector->nPixels().X(),
                                -0.5,
                                m_detector->nPixels().X() - 0.5,
                                m_detector->nPixels().Y(),
                                -0.5,
                                m_detector->nPixels().Y() - 0.5);
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "Size map for associated clusters; cluster size; #entries",
                                          m_detector->nPixels().X(),
                                          -0.5,
                                          m_detector->nPixels().X() - 0.5,
                                          m_detector->nPixels().Y(),
                                          -0.5,
                                          m_detector->nPixels().Y() - 0.5,
                                          0,
                                          100);
    hClusterChargeMapAssoc = new TProfile2D("clusterChargeMapAssoc",
                                            "Charge map for associated clusters; cluster charge [e]; #entries",
                                            m_detector->nPixels().X(),
                                            -0.5,
                                            m_detector->nPixels().X() - 0.5,
                                            m_detector->nPixels().Y(),
                                            -0.5,
                                            m_detector->nPixels().Y() - 0.5,
                                            0,
                                            500);

    hTrackZPosDUT = new TH1F("globalTrackZPosOnDUT",
                             "Global z-position of track on the DUT; global z of track intersection [mm]; #entries ",
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
                                   "Charge distribution of pixels from associated clusters;pixel raw value;#entries",
                                   1024,
                                   -0.5,
                                   1023.5);
    hPixelRawValueMapAssoc = new TProfile2D("pixelRawValueMapAssoc",
                                            "Charge map of pixels from associated clusters;pixel raw values;# entries",
                                            m_detector->nPixels().X(),
                                            -0.5,
                                            m_detector->nPixels().X() - 0.5,
                                            m_detector->nPixels().Y(),
                                            -0.5,
                                            m_detector->nPixels().Y() - 0.5,
                                            0,
                                            255);

    associatedTracksVersusTime =
        new TH1F("associatedTracksVersusTime", "Associated tracks over time;time [s];# associated tracks", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "Residual in X;x_{track}-x_{hit}  [mm];# entries", 800, -0.1, 0.1);
    residualsY = new TH1F("residualsY", "Residual in Y;y_{track}-y_{hit}  [mm];# entries", 800, -0.1, 0.1);
    residualsPos = new TH1F(
        "residualsPos", "Absolute distance between track and hit;|pos_{track}-pos_{hit}|  [mm];# entries", 800, -0.1, 0.1);
    residualsPosVsresidualsTime =
        new TH2F("residualsPosVsresidualsTime",
                 "Time vs. absolute position residuals;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [mm];# entries",
                 nTimeBins,
                 -nTimeBins / 2. * timeBinning - timeBinning / 2.,
                 nTimeBins / 2. * timeBinning - timeBinning / 2.,
                 800,
                 0.,
                 0.2);

    residualsX1pix =
        new TH1F("residualsX1pix", "Residual for 1-pixel clusters in X;x_{track}-x_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsY1pix =
        new TH1F("residualsY1pix", "Residual for 1-pixel clusters in Y;y_{track}-y_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsX2pix =
        new TH1F("residualsX2pix", "Residual for 2-pixel clusters in X;x_{track}-x_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsY2pix =
        new TH1F("residualsY2pix", "Residual for 2-pixel clusters in Y;y_{track}-y_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsX3pix =
        new TH1F("residualsX3pix", "Residual for 3-pixel clusters in X;x_{track}-x_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsY3pix =
        new TH1F("residualsY3pix", "Residual for 3-pixel clusters in Y;y_{track}-y_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsX4pix =
        new TH1F("residualsX4pix", "Residual for 4-pixel clusters in X;x_{track}-x_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsY4pix =
        new TH1F("residualsY4pix", "Residual for 4-pixel clusters in Y;y_{track}-y_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsXatLeast5pix = new TH1F(
        "residualsXatLeast5pix", "Residual for >= 5-pixel clusters in X;x_{track}-x_{hit} [mm];# entries", 800, -0.1, 0.1);
    residualsYatLeast5pix = new TH1F(
        "residualsYatLeast5pix", "Residual for >= 5-pixel clusters in Y;y_{track}-y_{hit} [mm];# entries", 800, -0.1, 0.1);

    clusterChargeAssoc = new TH1F("clusterChargeAssociated",
                                  "Charge distribution of associated clusters;cluster charge [e];# entries",
                                  10000,
                                  0,
                                  10000);
    clusterSizeAssoc = new TH1F(
        "clusterSizeAssociated", "Size distribution of associated clusters;cluster size; # entries", 30, -0.5, 29.5);
    clusterSizeAssocNorm = new TH1F("clusterSizeAssociatedNormalized",
                                    "Normalized size distribution of associated clusters;cluster size normalized;#entries",
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
    auto pitch_x = static_cast<double>(Units::convert(m_detector->getPitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->getPitch().Y(), "um"));
    std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

    // cut flow histogram
    std::string title = m_detector->getName() + ": number of tracks discarded by different cuts;cut type;tracks";
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

    title = "Mean cluster charge map;" + mod_axes + "<cluster charge> [ke]";
    qvsxmym = new TProfile2D("qvsxmym",
                             title.c_str(),
                             static_cast<int>(pitch_x),
                             -pitch_x / 2.,
                             pitch_x / 2.,
                             static_cast<int>(pitch_y),
                             -pitch_y / 2.,
                             pitch_y / 2.,
                             0,
                             250);

    title = "Most probable cluster charge map, Moyal approx.;" + mod_axes + "cluster charge MPV [ke]";
    qMoyalvsxmym = new TProfile2D("qMoyalvsxmym",
                                  title.c_str(),
                                  static_cast<int>(pitch_x),
                                  -pitch_x / 2.,
                                  pitch_x / 2.,
                                  static_cast<int>(pitch_y),
                                  -pitch_y / 2.,
                                  pitch_y / 2.,
                                  0,
                                  250);

    title = "Seed pixel charge map;" + mod_axes + "<seed pixel charge> [ke]";
    pxqvsxmym = new TProfile2D("pxqvsxmym",
                               title.c_str(),
                               static_cast<int>(pitch_x),
                               -pitch_x / 2.,
                               pitch_x / 2.,
                               static_cast<int>(pitch_y),
                               -pitch_y / 2.,
                               pitch_y / 2.,
                               0,
                               250);

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

    residualsTime = new TH1F("residualsTime",
                             "Time residual;time_{track}-time_{hit} [ns];#entries",
                             nTimeBins,
                             -nTimeBins / 2. * timeBinning - timeBinning / 2.,
                             nTimeBins / 2. * timeBinning - timeBinning / 2.);

    hTrackCorrelationX =
        new TH1F("hTrackCorrelationX", "Track residual X, all clusters;x_{track}-x_{hit} [mm];# entries", 4000, -10., 10.);
    hTrackCorrelationY =
        new TH1F("hTrackCorrelationY", "Track residual Y, all clusters;y_{track}-y_{hit} [mm];# entries", 4000, -10., 10.);
    hTrackCorrelationPos = new TH1F("hTrackCorrelationPos",
                                    "Track residual (absolute), all clusters;|pos_{track}-pos_{hit}| [mm];# entries",
                                    2100,
                                    -1.,
                                    10.);
    hTrackCorrelationTime = new TH1F("hTrackCorrelationTime",
                                     "Track time residual, all clusters;time_{track}-time_{hit} [ns];# entries",
                                     20000,
                                     -5000,
                                     5000);
    hTrackCorrelationPosVsCorrelationTime =
        new TH2F("hTrackCorrelationPosVsCorrelationTime",
                 "Track time vs. distance residual;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [mm];# entries",
                 20000,
                 -5000,
                 5000,
                 2100,
                 -1.,
                 10.);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime",
                                   "Time residual vs. time;time [ns];time_{track}-time_{hit} [mm];# entries",
                                   20000,
                                   0,
                                   200,
                                   nTimeBins,
                                   -nTimeBins / 2. * timeBinning - timeBinning / 2.,
                                   nTimeBins / 2. * timeBinning - timeBinning / 2.);
    residualsTimeVsTot =
        new TH2F("residualsTimeVsTot",
                 "Time residual vs. pixel charge;time_{track} - time_{hit} [ns];seed pixel ToT [lsb];# entries",
                 nTimeBins,
                 -nTimeBins / 2. * timeBinning - timeBinning / 2.,
                 nTimeBins / 2. * timeBinning - timeBinning / 2.,
                 1000,
                 -0.5,
                 999.5);

    residualsTimeVsSignal =
        new TH2F("residualsTimeVsSignal",
                 "Time residual vs. cluster charge;cluster charge [e];time_{track}-time_{hit} [mm];# entries",
                 20000,
                 0,
                 100000,
                 nTimeBins,
                 -nTimeBins / 2. * timeBinning - timeBinning / 2.,
                 nTimeBins / 2. * timeBinning - timeBinning / 2.);

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
}

StatusCode AnalysisDUT::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();

    // Loop over all tracks
    for(auto& track : tracks) {
        // Flags to select clusters and tracks
        bool has_associated_cluster = false;
        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            hCutHisto->Fill(1);
            num_tracks++;
            continue;
        }

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        if(!m_detector->hasIntercept(track.get(), 0.5)) {
            LOG(DEBUG) << " - track outside DUT area";
            hCutHisto->Fill(2);
            num_tracks++;
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
            num_tracks++;
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
            hCutHisto->Fill(4);
            num_tracks++;
            continue;
        } else if(fabs(track->timestamp() - event->start()) < m_timeCutFrameEdge) {
            // Early edge - eventStart points to the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            hCutHisto->Fill(4);
            num_tracks++;
            continue;
        }

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod = static_cast<double>(Units::convert(inpixel.X(), "um"));
        auto ymod = static_cast<double>(Units::convert(inpixel.Y(), "um"));

        // Loop over all associated DUT clusters:
        for(auto assoc_cluster : track->getAssociatedClusters(m_detector->getName())) {
            LOG(DEBUG) << " - Looking at next associated DUT cluster";

            // if closest cluster should be used continue if current associated cluster is not the closest one
            if(useClosestCluster && track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                continue;
            }
            has_associated_cluster = true;

            hTrackZPosDUT->Fill(track->getState(m_detector->getName()).z());
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->getIntercept(assoc_cluster->global().z());
            double xdistance = intercept.X() - assoc_cluster->global().x();
            double ydistance = intercept.Y() - assoc_cluster->global().y();
            double xabsdistance = fabs(xdistance);
            double yabsdistance = fabs(ydistance);
            double tdistance = track->timestamp() - assoc_cluster->timestamp();
            double posDiff =
                sqrt((intercept.X() - assoc_cluster->global().x()) * (intercept.X() - assoc_cluster->global().x()) +
                     (intercept.Y() - assoc_cluster->global().y()) * (intercept.Y() - assoc_cluster->global().y()));

            // Correlation plots
            hTrackCorrelationX->Fill(xdistance);
            hTrackCorrelationY->Fill(ydistance);
            hTrackCorrelationTime->Fill(tdistance);
            hTrackCorrelationPos->Fill(posDiff);
            hTrackCorrelationPosVsCorrelationTime->Fill(track->timestamp() - assoc_cluster->timestamp(), posDiff);

            hClusterMapAssoc->Fill(assoc_cluster->column(), assoc_cluster->row());
            hClusterSizeMapAssoc->Fill(
                assoc_cluster->column(), assoc_cluster->row(), static_cast<double>(assoc_cluster->size()));

            // Cluster charge normalized to path length in sensor:
            double norm = 1; // FIXME fabs(cos( turn*wt )) * fabs(cos( tilt*wt ));
            // FIXME: what does this mean? To my understanding we have the correct charge here already...
            auto cluster_charge = assoc_cluster->charge();
            auto normalized_charge = cluster_charge * norm;

            // clusterChargeAssoc->Fill(normalized_charge);
            clusterChargeAssoc->Fill(cluster_charge);
            hClusterChargeMapAssoc->Fill(assoc_cluster->column(), assoc_cluster->row(), cluster_charge);

            // Fill per-pixel histograms
            for(auto& pixel : assoc_cluster->pixels()) {
                hHitMapAssoc->Fill(pixel->column(), pixel->row());
                hPixelRawValueAssoc->Fill(pixel->raw());
                hPixelRawValueMapAssoc->Fill(pixel->column(), pixel->row(), pixel->raw());
            }

            associatedTracksVersusTime->Fill(static_cast<double>(Units::convert(track->timestamp(), "s")));

            // Residuals
            residualsX->Fill(xdistance);
            residualsY->Fill(ydistance);
            residualsPos->Fill(posDiff);
            residualsPosVsresidualsTime->Fill(tdistance, posDiff);

            if(assoc_cluster->columnWidth() == 1) {
                residualsX1pix->Fill(xdistance);
            }
            if(assoc_cluster->rowWidth() == 1) {
                residualsY1pix->Fill(ydistance);
            }

            if(assoc_cluster->columnWidth() == 2) {
                residualsX2pix->Fill(xdistance);
            }
            if(assoc_cluster->rowWidth() == 2) {
                residualsY2pix->Fill(ydistance);
            }

            if(assoc_cluster->columnWidth() == 3) {
                residualsX3pix->Fill(xdistance);
            }
            if(assoc_cluster->rowWidth() == 3) {
                residualsY3pix->Fill(ydistance);
            }

            if(assoc_cluster->columnWidth() == 4) {
                residualsX4pix->Fill(xdistance);
            }
            if(assoc_cluster->rowWidth() == 4) {
                residualsY4pix->Fill(ydistance);
            }

            if(assoc_cluster->columnWidth() >= 5) {
                residualsXatLeast5pix->Fill(xdistance);
            }
            if(assoc_cluster->rowWidth() >= 5) {
                residualsYatLeast5pix->Fill(ydistance);
            }

            // Time residuals
            residualsTime->Fill(tdistance);
            residualsTimeVsTime->Fill(tdistance, track->timestamp());
            residualsTimeVsTot->Fill(tdistance, assoc_cluster->getSeedPixel()->raw());
            residualsTimeVsSignal->Fill(tdistance, cluster_charge);

            clusterSizeAssoc->Fill(static_cast<double>(assoc_cluster->size()));
            clusterSizeAssocNorm->Fill(static_cast<double>(assoc_cluster->size()));
            clusterWidthRowAssoc->Fill(static_cast<double>(assoc_cluster->rowWidth()));
            clusterWidthColAssoc->Fill(static_cast<double>(assoc_cluster->columnWidth()));

            // Fill in-pixel plots: (all as function of track position within pixel cell)
            qvsxmym->Fill(xmod, ymod, cluster_charge);                     // cluster charge profile
            qMoyalvsxmym->Fill(xmod, ymod, exp(-normalized_charge / 3.5)); // norm. cluster charge profile

            // mean charge of cluster seed
            pxqvsxmym->Fill(xmod, ymod, assoc_cluster->getSeedPixel()->charge());

            // mean cluster size
            npxvsxmym->Fill(xmod, ymod, static_cast<double>(assoc_cluster->size()));
            if(assoc_cluster->size() == 1)
                npx1vsxmym->Fill(xmod, ymod);
            if(assoc_cluster->size() == 2)
                npx2vsxmym->Fill(xmod, ymod);
            if(assoc_cluster->size() == 3)
                npx3vsxmym->Fill(xmod, ymod);
            if(assoc_cluster->size() == 4)
                npx4vsxmym->Fill(xmod, ymod);

            // residual MAD x, y, combined (sqrt(x*x + y*y))
            rmsxvsxmym->Fill(xmod, ymod, xabsdistance);
            rmsyvsxmym->Fill(xmod, ymod, yabsdistance);
            rmsxyvsxmym->Fill(xmod, ymod, fabs(sqrt(xdistance * xdistance + ydistance * ydistance)));

            hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
            hAssociatedTracksLocalPosition->Fill(m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept));
        }
        if(!has_associated_cluster) {
            hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
        }
        num_tracks++;
    }
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisDUT::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    hCutHisto->Scale(1 / double(num_tracks));
    clusterSizeAssocNorm->Scale(1 / clusterSizeAssoc->Integral());
}
