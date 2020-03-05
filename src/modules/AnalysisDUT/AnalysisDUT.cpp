#include "AnalysisDUT.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisDUT::AnalysisDUT(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_timeCutFrameEdge = m_config.get<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    chi2ndofCut = m_config.get<double>("chi2ndof_cut", 3.);
    useClosestCluster = m_config.get<bool>("use_closest_cluster", true);
}

void AnalysisDUT::initialise() {

    hClusterMapAssoc = new TH2F("clusterMapAssoc",
                                "clusterMapAssoc; cluster col; cluster row",
                                m_detector->nPixels().X(),
                                -0.5,
                                m_detector->nPixels().X() - 0.5,
                                m_detector->nPixels().Y(),
                                -0.5,
                                m_detector->nPixels().Y() - 0.5);
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "clusterSizeMapAssoc; cluster size; #entries",
                                          m_detector->nPixels().X(),
                                          -0.5,
                                          m_detector->nPixels().X() - 0.5,
                                          m_detector->nPixels().Y(),
                                          -0.5,
                                          m_detector->nPixels().Y() - 0.5,
                                          0,
                                          100);
    hClusterChargeMapAssoc = new TProfile2D("clusterChargeMapAssoc",
                                            "clusterSizeChargeAssoc; cluster charge [e]; #entries",
                                            m_detector->nPixels().X(),
                                            -0.5,
                                            m_detector->nPixels().X() - 0.5,
                                            m_detector->nPixels().Y(),
                                            -0.5,
                                            m_detector->nPixels().Y() - 0.5,
                                            0,
                                            500);

    // Per-pixel histograms
    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "hitMapAssoc; hit column; hit row",
                            m_detector->nPixels().X(),
                            -0.5,
                            m_detector->nPixels().X() - 0.5,
                            m_detector->nPixels().Y(),
                            -0.5,
                            m_detector->nPixels().Y() - 0.5);
    hHitMapROI = new TH2F("hitMapROI",
                          "hitMapROI; hit column; hit row",
                          m_detector->nPixels().X(),
                          -0.5,
                          m_detector->nPixels().X() - 0.5,
                          m_detector->nPixels().Y(),
                          -0.5,
                          m_detector->nPixels().Y() - 0.5);
    hPixelRawValueAssoc = new TH1F("pixelRawValueAssoc", "pixelRawValueAssoc;pixel raw value;#entries", 1024, 0, 1024);
    hPixelRawValueMapAssoc = new TProfile2D("pixelRawValueMapAssoc",
                                            "pixelRawValueMapAssoc;pixel raw values;# entries",
                                            m_detector->nPixels().X(),
                                            -0.5,
                                            m_detector->nPixels().X() - 0.5,
                                            m_detector->nPixels().Y(),
                                            -0.5,
                                            m_detector->nPixels().Y() - 0.5,
                                            0,
                                            255);

    associatedTracksVersusTime =
        new TH1F("associatedTracksVersusTime", "associatedTracksVersusTime;time [s];# associated tracks", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "residualsX;x_{track}-x_{hit}  [mm];# entries", 800, -0.1, 0.1);
    residualsY = new TH1F("residualsY", "residualsY;y_{track}-y_{hit}  [mm];# entries", 800, -0.1, 0.1);
    residualsPos = new TH1F("residualsPos", "residualsPos;|pos_{track}-pos_{hit}|  [mm];# entries", 800, -0.1, 0.1);
    residualsPosVsresidualsTime =
        new TH2F("residualsPosVsresidualsTime",
                 "residualsPosVsresidualsTime;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [mm];# entries",
                 20000,
                 -1000,
                 +1000,
                 800,
                 0.,
                 0.2);

    residualsX1pix = new TH1F("residualsX1pix", "residualsX1pix;x_{track}-x_{hit} [mm];# entries", 400, -0.2, 0.2);
    residualsY1pix = new TH1F("residualsY1pix", "residualsY1pix;y_{track}-y_{hit} [mm];# entries", 400, -0.2, 0.2);
    residualsX2pix = new TH1F("residualsX2pix", "residualsX2pix;x_{track}-x_{hit} [mm];# entries", 400, -0.2, 0.2);
    residualsY2pix = new TH1F("residualsY2pix", "residualsY2pix;y_{track}-y_{hit} [mm];# entries", 400, -0.2, 0.2);

    clusterChargeAssoc =
        new TH1F("clusterChargeAssociated", "clusterChargeAssociated;cluster charge [e];# entries", 10000, 0, 10000);
    clusterSizeAssoc = new TH1F("clusterSizeAssociated", "clusterSizeAssociated;cluster size; # entries", 30, 0, 30);
    clusterSizeAssocNorm = new TH1F(
        "clusterSizeAssociatedNormalized", "clusterSizeAssociatedNormalized;cluster size normalized;#entries", 30, 0, 30);
    clusterWidthRowAssoc =
        new TH1F("clusterWidthRowAssociated", "clusterWidthRowAssociated;cluster size row; # entries", 30, 0, 30);
    clusterWidthColAssoc =
        new TH1F("clusterWidthColAssociated", "clusterWidthColAssociated;cluster size col; # entries", 30, 0, 30);

    // In-pixel studies:
    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));
    std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

    // cut flow histogram
    std::string title = m_detector->name() + ": number of tracks discarded by different cuts;cut type;tracks";
    hCutHisto = new TH1F("hCutHisto", title.c_str(), 4, 1, 5);
    hCutHisto->GetXaxis()->SetBinLabel(1, "High Chi2");
    hCutHisto->GetXaxis()->SetBinLabel(2, "Outside DUT area");
    hCutHisto->GetXaxis()->SetBinLabel(3, "Close to masked pixel");
    hCutHisto->GetXaxis()->SetBinLabel(4, "Close to frame begin/end");

    title = "DUT x resolution;" + mod_axes + "MAD(#Deltax) [#mum]";
    rmsxvsxmym = new TProfile2D("rmsxvsxmym",
                                title.c_str(),
                                static_cast<int>(pitch_x),
                                -pitch_x / 2.,
                                pitch_x / 2.,
                                static_cast<int>(pitch_y),
                                -pitch_y / 2.,
                                pitch_y / 2.);

    title = "DUT y resolution;" + mod_axes + "MAD(#Deltay) [#mum]";
    rmsyvsxmym = new TProfile2D("rmsyvsxmym",
                                title.c_str(),
                                static_cast<int>(pitch_x),
                                -pitch_x / 2.,
                                pitch_x / 2.,
                                static_cast<int>(pitch_y),
                                -pitch_y / 2.,
                                pitch_y / 2.);

    title = "DUT resolution;" + mod_axes + "MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    rmsxyvsxmym = new TProfile2D("rmsxyvsxmym",
                                 title.c_str(),
                                 static_cast<int>(pitch_x),
                                 -pitch_x / 2.,
                                 pitch_x / 2.,
                                 static_cast<int>(pitch_y),
                                 -pitch_y / 2.,
                                 pitch_y / 2.);

    title = "DUT cluster charge map;" + mod_axes + "<cluster charge> [ke]";
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

    title = "DUT cluster charge map, Moyal approx;" + mod_axes + "cluster charge MPV [ke]";
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

    title = "DUT seed pixel charge map;" + mod_axes + "<seed pixel charge> [ke]";
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

    title = "DUT cluster size map;" + mod_axes + "<pixels/cluster>";
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

    title = "DUT 1-pixel cluster map;" + mod_axes + "clusters";
    npx1vsxmym = new TH2F("npx1vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "DUT 2-pixel cluster map;" + mod_axes + "clusters";
    npx2vsxmym = new TH2F("npx2vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "DUT 3-pixel cluster map;" + mod_axes + "clusters";
    npx3vsxmym = new TH2F("npx3vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    title = "DUT 4-pixel cluster map;" + mod_axes + "clusters";
    npx4vsxmym = new TH2F("npx4vsxmym",
                          title.c_str(),
                          static_cast<int>(pitch_x),
                          -pitch_x / 2.,
                          pitch_x / 2.,
                          static_cast<int>(pitch_y),
                          -pitch_y / 2.,
                          pitch_y / 2.);

    residualsTime = new TH1F("residualsTime", "residualsTime;time_{track}-time_{hit} [ns];#entries", 20000, -1000, +1000);

    hTrackCorrelationX =
        new TH1F("hTrackCorrelationX", "hTrackCorrelationX;x_{track}-x_{hit} [mm];# entries", 4000, -10., 10.);
    hTrackCorrelationY =
        new TH1F("hTrackCorrelationY", "hTrackCorrelationY;y_{track}-y_{hit} [mm];# entries", 4000, -10., 10.);
    hTrackCorrelationPos =
        new TH1F("hTrackCorrelationPos", "hTrackCorrelationPos;|pos_{track}-pos_{hit}| [mm];# entries", 2100, -1., 10.);
    hTrackCorrelationTime = new TH1F(
        "hTrackCorrelationTime", "hTrackCorrelationTime;time_{track}-time_{hit} [ns];# entries", 20000, -5000, 5000);
    hTrackCorrelationPosVsCorrelationTime =
        new TH2F("hTrackCorrelationPosVsCorrelationTime",
                 "hTrackCorrelationPosVsCorrelationTime;time_{track}-time_{hit} [ns];|pos_{track}-pos_{hit}| [mm];# entries",
                 20000,
                 -5000,
                 5000,
                 2100,
                 -1.,
                 10.);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime",
                                   "residualsTimeVsTime;time [ns];time_{track}-time_{hit} [mm];# entries",
                                   20000,
                                   0,
                                   200,
                                   1000,
                                   -1000,
                                   +1000);
    residualsTimeVsSignal = new TH2F("residualsTimeVsSignal",
                                     "residualsTimeVsSignal;cluster charge [e];time_{track}-time_{hit} [mm];# entries",
                                     20000,
                                     0,
                                     100000,
                                     1000,
                                     -1000,
                                     +1000);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition",
                 "hAssociatedTracksGlobalPosition;global intercept x [mm];global intercept y [mm]",
                 200,
                 -10,
                 10,
                 200,
                 -10,
                 10);
    hAssociatedTracksLocalPosition = new TH2F("hAssociatedTracksLocalPosition",
                                              "hAssociatedTracksLocalPosition;local intercept x [px];local intercept y [px]",
                                              m_detector->nPixels().X(),
                                              -0.5,
                                              m_detector->nPixels().X() - 0.5,
                                              m_detector->nPixels().Y(),
                                              -0.5,
                                              m_detector->nPixels().Y() - 0.5);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition",
                 "hUnassociatedTracksGlobalPosition; global intercept x [mm]; global intercept y [mm]",
                 200,
                 -10,
                 10,
                 200,
                 -10,
                 10);
}

StatusCode AnalysisDUT::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        // Flags to select clusters and tracks
        bool has_associated_cluster = false;
        bool is_within_roi = true;

        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            hCutHisto->Fill(1);
            num_tracks++;
            continue;
        }

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track);
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        if(!m_detector->hasIntercept(track, 0.5)) {
            LOG(DEBUG) << " - track outside DUT area";
            hCutHisto->Fill(2);
            num_tracks++;
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track)) {
            is_within_roi = false;
        }

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track, 1.)) {
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
        for(auto assoc_cluster : track->associatedClusters()) {
            LOG(DEBUG) << " - Looking at next associated DUT cluster";

            // if closest cluster should be used continue if current associated cluster is not the closest one
            if(useClosestCluster) {
                if(track->getClosestCluster() != assoc_cluster) {
                    continue;
                }
            }
            has_associated_cluster = true;

            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(assoc_cluster->global().z());
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

            // FIXME need to understand local coord of clusters - why shifted? what's normal?
            auto clusterLocal = m_detector->globalToLocal(assoc_cluster->global());
            hClusterMapAssoc->Fill(m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal));
            hClusterSizeMapAssoc->Fill(m_detector->getColumn(clusterLocal),
                                       m_detector->getRow(clusterLocal),
                                       static_cast<double>(assoc_cluster->size()));

            // Cluster charge normalized to path length in sensor:
            double norm = 1; // FIXME fabs(cos( turn*wt )) * fabs(cos( tilt*wt ));
            // FIXME: what does this mean? To my understanding we have the correct charge here already...
            auto cluster_charge = assoc_cluster->charge();
            auto normalized_charge = cluster_charge * norm;

            // clusterChargeAssoc->Fill(normalized_charge);
            clusterChargeAssoc->Fill(cluster_charge);
            hClusterChargeMapAssoc->Fill(
                m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal), cluster_charge);

            // Fill per-pixel histograms
            for(auto& pixel : assoc_cluster->pixels()) {
                hHitMapAssoc->Fill(pixel->column(), pixel->row());
                if(is_within_roi) {
                    hHitMapROI->Fill(pixel->column(), pixel->row());
                }
                hPixelRawValueAssoc->Fill(pixel->raw());
                hPixelRawValueMapAssoc->Fill(pixel->column(), pixel->row(), pixel->raw());
            }

            associatedTracksVersusTime->Fill(static_cast<double>(Units::convert(track->timestamp(), "s")));

            // Residuals
            residualsX->Fill(xdistance);
            residualsY->Fill(ydistance);
            residualsPos->Fill(posDiff);
            residualsPosVsresidualsTime->Fill(tdistance, posDiff);

            if(assoc_cluster->size() == 1) {
                residualsX1pix->Fill(xdistance);
                residualsY1pix->Fill(ydistance);
            }
            if(assoc_cluster->size() == 2) {
                residualsX2pix->Fill(xdistance);
                residualsY2pix->Fill(ydistance);
            }

            // Time residuals
            residualsTime->Fill(tdistance);
            residualsTimeVsTime->Fill(tdistance, track->timestamp());
            residualsTimeVsSignal->Fill(tdistance, cluster_charge);

            clusterSizeAssoc->Fill(static_cast<double>(assoc_cluster->size()));
            clusterSizeAssocNorm->Fill(static_cast<double>(assoc_cluster->size()));
            clusterWidthRowAssoc->Fill(assoc_cluster->rowWidth());
            clusterWidthColAssoc->Fill(assoc_cluster->columnWidth());

            // Fill in-pixel plots: (all as function of track position within pixel cell)
            if(is_within_roi) {
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
            }
            hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
            hAssociatedTracksLocalPosition->Fill(localIntercept.X(), localIntercept.Y());
        }
        if(!has_associated_cluster) {
            hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
        }
        num_tracks++;
    }
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisDUT::finalise() {
    hCutHisto->Scale(1 / double(num_tracks));
    clusterSizeAssocNorm->Scale(1 / clusterSizeAssoc->Integral());
}
