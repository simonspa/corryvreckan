#include "AnalysisDUT.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisDUT::AnalysisDUT(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_timeCutFrameEdge = m_config.get<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    chi2ndofCut = m_config.get<double>("chi2ndof_cut", 3.);
}

void AnalysisDUT::initialise() {

    hClusterMapAssoc = new TH2F("clusterMapAssoc",
                                "clusterMapAssoc; cluster col; cluster row",
                                m_detector->nPixels().X(),
                                0,
                                m_detector->nPixels().X(),
                                m_detector->nPixels().Y(),
                                0,
                                m_detector->nPixels().Y());
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "clusterSizeMapAssoc; cluster size; #entries",
                                          m_detector->nPixels().X(),
                                          0,
                                          m_detector->nPixels().X(),
                                          m_detector->nPixels().Y(),
                                          0,
                                          m_detector->nPixels().Y(),
                                          0,
                                          100);
    hClusterChargeMapAssoc = new TProfile2D("clusterChargeMapAssoc",
                                            "clusterSizeChargeAssoc; cluster charge [e]; #entries",
                                            m_detector->nPixels().X(),
                                            0,
                                            m_detector->nPixels().X(),
                                            m_detector->nPixels().Y(),
                                            0,
                                            m_detector->nPixels().Y(),
                                            0,
                                            500);

    // Per-pixel histograms
    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "hitMapAssoc; hit column; hit row",
                            m_detector->nPixels().X(),
                            0,
                            m_detector->nPixels().X(),
                            m_detector->nPixels().Y(),
                            0,
                            m_detector->nPixels().Y());
    hHitMapROI = new TH2F("hitMapROI",
                          "hitMapROI; hit column; hit row",
                          m_detector->nPixels().X(),
                          0,
                          m_detector->nPixels().X(),
                          m_detector->nPixels().Y(),
                          0,
                          m_detector->nPixels().Y());
    hPixelRawValueAssoc = new TH1F("pixelRawValueAssoc", "pixelRawValueAssoc;pixel raw value;#entries", 1024, 0, 1024);
    hPixelRawValueMapAssoc = new TProfile2D("pixelRawValueMapAssoc",
                                            "pixelRawValueMapAssoc;pixel raw values;# entries",
                                            m_detector->nPixels().X(),
                                            0,
                                            m_detector->nPixels().X(),
                                            m_detector->nPixels().Y(),
                                            0,
                                            m_detector->nPixels().Y(),
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

    // cut flow histogram
    std::string title = m_detector->name() + ": number of clusters discarded by cut;cut;events";
    hCutFlow = new TH1F("cut_flow", title.c_str(), 4, 1, 5);

    // In-pixel studies:
    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));
    auto mod_axes = "x_{track} mod " + std::to_string(pitch_x) + "#mum;y_{track} mod " + std::to_string(pitch_y) + "#mum;";

    title = "DUT x resolution;" + mod_axes + "MAD(#Deltax) [#mum]";
    rmsxvsxmym = new TProfile2D(
        "rmsxvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT y resolution;" + mod_axes + "MAD(#Deltay) [#mum]";
    rmsyvsxmym = new TProfile2D(
        "rmsyvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT resolution;" + mod_axes + "MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    rmsxyvsxmym = new TProfile2D(
        "rmsxyvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT cluster charge map;" + mod_axes + "<cluster charge> [ke]";
    qvsxmym = new TProfile2D(
        "qvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y, 0, 250);

    title = "DUT cluster charge map, Moyal approx;" + mod_axes + "cluster charge MPV [ke]";
    qMoyalvsxmym = new TProfile2D(
        "qMoyalvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y, 0, 250);

    title = "DUT seed pixel charge map;" + mod_axes + "<seed pixel charge> [ke]";
    pxqvsxmym = new TProfile2D(
        "pxqvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y, 0, 250);

    title = "DUT cluster size map;" + mod_axes + "<pixels/cluster>";
    npxvsxmym = new TProfile2D(
        "npxvsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y, 0, 4.5);

    title = "DUT 1-pixel cluster map;" + mod_axes + "clusters";
    npx1vsxmym =
        new TH2F("npx1vsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT 2-pixel cluster map;" + mod_axes + "clusters";
    npx2vsxmym =
        new TH2F("npx2vsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT 3-pixel cluster map;" + mod_axes + "clusters";
    npx3vsxmym =
        new TH2F("npx3vsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    title = "DUT 4-pixel cluster map;" + mod_axes + "clusters";
    npx4vsxmym =
        new TH2F("npx4vsxmym", title.c_str(), static_cast<int>(pitch_x), 0, pitch_x, static_cast<int>(pitch_y), 0, pitch_y);

    // Efficiency maps
    hPixelEfficiencyMap = new TProfile2D("hPixelEfficiencyMap",
                                         "hPixelEfficiencyMap;column;row;efficiency",
                                         static_cast<int>(pitch_x),
                                         0,
                                         pitch_x,
                                         static_cast<int>(pitch_y),
                                         0,
                                         pitch_y,
                                         0,
                                         1);
    hChipEfficiencyMap = new TProfile2D("hChipEfficiencyMap",
                                        "hChipEfficiencyMap;column;row;efficiency",
                                        m_detector->nPixels().X(),
                                        0,
                                        m_detector->nPixels().X(),
                                        m_detector->nPixels().Y(),
                                        0,
                                        m_detector->nPixels().Y(),
                                        0,
                                        1);
    hGlobalEfficiencyMap = new TProfile2D("hGlobalEfficiencyMap",
                                          "hGlobalEfficiencyMap;column;row;efficiency",
                                          300,
                                          -1.5 * m_detector->size().X(),
                                          1.5 * m_detector->size().X(),
                                          300,
                                          -1.5 * m_detector->size().Y(),
                                          1.5 * m_detector->size().Y(),
                                          0,
                                          1);

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
                                              "hAssociatedTracksLocalPosition;local intercept x [mm];local intercept y [mm]",
                                              m_detector->nPixels().X(),
                                              0,
                                              m_detector->nPixels().X(),
                                              m_detector->nPixels().Y(),
                                              0,
                                              m_detector->nPixels().Y());
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
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        int noFoundClusters = 0;
        int noTotalAssocClusters = 0;
        // Flags to select clusters and tracks
        bool has_associated_cluster = false;
        bool is_within_roi = true;

        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            LOG(DEBUG) << " - track discarded due to Chi2/ndof";
            hCutFlow->Fill(1.0);
            continue;
        }

        // Check if it intercepts the DUT
        auto globalIntercept = m_detector->getIntercept(track);
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        if(!m_detector->hasIntercept(track, 0.5)) {
            LOG(DEBUG) << " - track outside DUT area";
            if(track->hasClosestCluster())
              hCutFlow->Fill(2.0);
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        if(!m_detector->isWithinROI(track)) {
            is_within_roi = false;
        }

        // Check that it doesn't go through/near a masked pixel
        if(m_detector->hitMasked(track, 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            if(track->hasClosestCluster())
              hCutFlow->Fill(3.0);
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
            hCutFlow->Fill(4.0);
            continue;
        } else if(fabs(track->timestamp() - event->start()) < m_timeCutFrameEdge) {
            // Early edge - eventStart points to the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - event->start()), {"us", "ns"}) << " at "
                       << Units::display(track->timestamp(), {"us"});
            hCutFlow->Fill(4.0);
            continue;
        }

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

                // Check distance between track and cluster
                ROOT::Math::XYZPoint intercept = track->intercept(cluster->global().z());

                // Correlation plots
                hTrackCorrelationX->Fill(intercept.X() - cluster->global().x());
                hTrackCorrelationY->Fill(intercept.Y() - cluster->global().y());
                hTrackCorrelationTime->Fill(track->timestamp() - cluster->timestamp());

                double posDiff = sqrt((intercept.X() - cluster->global().x()) * (intercept.X() - cluster->global().x()) +
                                      (intercept.Y() - cluster->global().y()) * (intercept.Y() - cluster->global().y()));

                hTrackCorrelationPos->Fill(posDiff);
                hTrackCorrelationPosVsCorrelationTime->Fill(track->timestamp() - cluster->timestamp(), posDiff);

                auto associated_clusters = track->associatedClusters();
                noTotalAssocClusters = int(associated_clusters.size());
                
                if(track->hasClosestCluster()){
                  if(track->getClosestCluster() != cluster){
                    hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
                    continue;
                  }
                }else{
                  hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
                  continue;
                }

                /*
                if(std::find(associated_clusters.begin(), associated_clusters.end(), cluster) == associated_clusters.end()) {
                    LOG(DEBUG) << "No associated cluster found";
                    hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
                    continue;
                }
                */


                LOG(DEBUG) << "Found associated cluster";
                noFoundClusters++;
                assoc_cluster_counter++;

                double xdistance = intercept.X() - cluster->global().x();
                double ydistance = intercept.Y() - cluster->global().y();
                double xabsdistance = fabs(xdistance);
                double yabsdistance = fabs(ydistance);
                double tdistance = track->timestamp() - cluster->timestamp();

                // We now have an associated cluster
                has_associated_cluster = true;
                // FIXME need to understand local coord of clusters - why shifted? what's normal?
                auto clusterLocal = m_detector->globalToLocal(cluster->global());
                hClusterMapAssoc->Fill(m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal));
                hClusterSizeMapAssoc->Fill(m_detector->getColumn(clusterLocal),
                                           m_detector->getRow(clusterLocal),
                                           static_cast<double>(cluster->size()));

                // Cluster charge normalized to path length in sensor:
                double norm = 1; // FIXME fabs(cos( turn*wt )) * fabs(cos( tilt*wt ));
                // FIXME: what does this mean? To my understanding we have the correct charge here already...
                auto normalized_charge = cluster->charge() * norm;

                // clusterChargeAssoc->Fill(normalized_charge);
                clusterChargeAssoc->Fill(cluster->charge());
                hClusterChargeMapAssoc->Fill(
                    m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal), cluster->charge());

                // Fill per-pixel histograms
                for(auto& pixel : (*cluster->pixels())) {
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
                residualsPos->Fill(sqrt(xdistance * xdistance + ydistance * ydistance));
                residualsPosVsresidualsTime->Fill(tdistance, sqrt(xdistance * xdistance + ydistance * ydistance));

                if(cluster->size() == 1) {
                    residualsX1pix->Fill(xdistance);
                    residualsY1pix->Fill(ydistance);
                }
                if(cluster->size() == 2) {
                    residualsX2pix->Fill(xdistance);
                    residualsY2pix->Fill(ydistance);
                }

                // Time residuals
                residualsTime->Fill(tdistance);
                residualsTimeVsTime->Fill(tdistance, track->timestamp());
                residualsTimeVsSignal->Fill(tdistance, cluster->charge());

                clusterSizeAssoc->Fill(static_cast<double>(cluster->size()));
                clusterSizeAssocNorm->Fill(static_cast<double>(cluster->size()));

                // Fill in-pixel plots: (all as function of track position within pixel cell)
                if(is_within_roi) {
                    qvsxmym->Fill(xmod, ymod, cluster->charge());                  // cluster charge profile
                    qMoyalvsxmym->Fill(xmod, ymod, exp(-normalized_charge / 3.5)); // norm. cluster charge profile

                    // mean charge of cluster seed
                    pxqvsxmym->Fill(xmod, ymod, cluster->getSeedPixel()->charge());

                    // mean cluster size
                    npxvsxmym->Fill(xmod, ymod, static_cast<double>(cluster->size()));
                    if(cluster->size() == 1)
                        npx1vsxmym->Fill(xmod, ymod);
                    if(cluster->size() == 2)
                        npx2vsxmym->Fill(xmod, ymod);
                    if(cluster->size() == 3)
                        npx3vsxmym->Fill(xmod, ymod);
                    if(cluster->size() == 4)
                        npx4vsxmym->Fill(xmod, ymod);

                    // residual MAD x, y, combined (sqrt(x*x + y*y))
                    rmsxvsxmym->Fill(xmod, ymod, xabsdistance);
                    rmsyvsxmym->Fill(xmod, ymod, yabsdistance);
                    rmsxyvsxmym->Fill(xmod, ymod, fabs(sqrt(xdistance * xdistance + ydistance * ydistance)));
                }

                track->addAssociatedCluster(cluster);
                hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());
                hAssociatedTracksLocalPosition->Fill(m_detector->getColumn(localIntercept),
                                                     m_detector->getRow(localIntercept));

                // Only allow one associated cluster per track
                break;
            }
        }

        // Efficiency plots:
        hGlobalEfficiencyMap->Fill(globalIntercept.X(), globalIntercept.Y(), has_associated_cluster);
        hChipEfficiencyMap->Fill(
            m_detector->getColumn(localIntercept), m_detector->getRow(localIntercept), has_associated_cluster);

        // For pixels, only look at the ROI:
        if(is_within_roi) {
            hPixelEfficiencyMap->Fill(xmod, ymod, has_associated_cluster);
        }
        LOG(DEBUG) << "No of associated clusters found for current track: " << noFoundClusters;
        LOG(DEBUG) << "Total number of assoc. clusters: for current track: " << noTotalAssocClusters;
    }
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisDUT::finalise() {
    LOG(INFO) << "Total number of associated clusters: " << assoc_cluster_counter;
    clusterSizeAssocNorm->Scale(1 / clusterSizeAssoc->Integral());
}
