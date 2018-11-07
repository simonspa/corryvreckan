#include "AnalysisDUT.h"

#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

using namespace corryvreckan;

AnalysisDUT::AnalysisDUT(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_timeCutFrameEdge = m_config.get<double>("timeCutFrameEdge", static_cast<double>(Units::convert(20, "ns")));
    spatialCut = m_config.get<double>("spatialCut", static_cast<double>(Units::convert(50, "um")));
    chi2ndofCut = m_config.get<double>("chi2ndofCut", 3.);
}

void AnalysisDUT::initialise() {

    hClusterMapAssoc = new TH2F("clusterMapAssoc",
                                "clusterMapAssoc",
                                m_detector->nPixelsX(),
                                0,
                                m_detector->nPixelsX(),
                                m_detector->nPixelsY(),
                                0,
                                m_detector->nPixelsY());
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "clusterSizeMapAssoc",
                                          m_detector->nPixelsX(),
                                          0,
                                          m_detector->nPixelsX(),
                                          m_detector->nPixelsY(),
                                          0,
                                          m_detector->nPixelsY(),
                                          0,
                                          100);

    hClusterToTMapAssoc = new TProfile2D("clusterSizeToTAssoc",
                                         "clusterToTMapAssoc",
                                         m_detector->nPixelsX(),
                                         0,
                                         m_detector->nPixelsX(),
                                         m_detector->nPixelsY(),
                                         0,
                                         m_detector->nPixelsY(),
                                         0,
                                         1000);

    // Per-pixel histograms
    hHitMapAssoc = new TH2F("hitMapAssoc",
                            "hitMapAssoc",
                            m_detector->nPixelsX(),
                            0,
                            m_detector->nPixelsX(),
                            m_detector->nPixelsY(),
                            0,
                            m_detector->nPixelsY());
    hHitMapROI = new TH2F("hitMapROI",
                          "hitMapROI",
                          m_detector->nPixelsX(),
                          0,
                          m_detector->nPixelsX(),
                          m_detector->nPixelsY(),
                          0,
                          m_detector->nPixelsY());
    hPixelToTAssoc = new TH1F("pixelToTAssoc", "pixelToTAssoc", 32, 0, 31);
    hPixelToTMapAssoc = new TProfile2D("pixelToTMapAssoc",
                                       "pixelToTMapAssoc",
                                       m_detector->nPixelsX(),
                                       0,
                                       m_detector->nPixelsX(),
                                       m_detector->nPixelsY(),
                                       0,
                                       m_detector->nPixelsY(),
                                       0,
                                       255);

    associatedTracksVersusTime = new TH1F("associatedTracksVersusTime", "associatedTracksVersusTime", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "residualsX", 800, -0.1, 0.1);
    residualsY = new TH1F("residualsY", "residualsY", 800, -0.1, 0.1);

    residualsX1pix = new TH1F("residualsX1pix", "residualsX1pix", 400, -0.2, 0.2);
    residualsY1pix = new TH1F("residualsY1pix", "residualsY1pix", 400, -0.2, 0.2);
    residualsX2pix = new TH1F("residualsX2pix", "residualsX2pix", 400, -0.2, 0.2);
    residualsY2pix = new TH1F("residualsY2pix", "residualsY2pix", 400, -0.2, 0.2);

    clusterTotAssoc = new TH1F("clusterTotAssociated", "clusterTotAssociated", 10000, 0, 10000);
    clusterTotAssocNorm = new TH1F("clusterTotAssociatedNormalized", "clusterTotAssociatedNormalized", 10000, 0, 10000);
    clusterSizeAssoc = new TH1F("clusterSizeAssociated", "clusterSizeAssociated", 30, 0, 30);

    // In-pixel studies:
    auto pitch_x = static_cast<double>(Units::convert(m_detector->pitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->pitch().Y(), "um"));
    auto mod_axes = "x_{track} mod " + std::to_string(pitch_x) + "#mum;y_{track} mod " + std::to_string(pitch_y) + "#mum;";

    std::string title = "DUT x resolution;" + mod_axes + "MAD(#Deltax) [#mum]";
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

    residualsTime = new TH1F("residualsTime", "residualsTime", 20000, -1000, +1000);

    hTrackCorrelationX = new TH1F("hTrackCorrelationX", "hTrackCorrelationX", 4000, -10., 10.);
    hTrackCorrelationY = new TH1F("hTrackCorrelationY", "hTrackCorrelationY", 4000, -10., 10.);
    hTrackCorrelationTime = new TH1F("hTrackCorrelationTime", "hTrackCorrelationTime", 2000000, -5000, 5000);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime", "residualsTimeVsTime", 20000, 0, 200, 1000, -1000, +1000);
    residualsTimeVsSignal = new TH2F("residualsTimeVsSignal", "residualsTimeVsSignal", 20000, 0, 100000, 1000, -1000, +1000);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition", "hAssociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition", "hUnassociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);
}

StatusCode AnalysisDUT::run(Clipboard* clipboard) {

    // Get the telescope tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
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

                // Check distance between track and cluster
                ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());

                // Correlation plots
                hTrackCorrelationX->Fill(intercept.X() - cluster->globalX());
                hTrackCorrelationY->Fill(intercept.Y() - cluster->globalY());
                hTrackCorrelationTime->Fill(track->timestamp() - cluster->timestamp());

                auto associated_clusters = track->associatedClusters();
                if(std::find(associated_clusters.begin(), associated_clusters.end(), cluster) == associated_clusters.end()) {
                    LOG(DEBUG) << "No associated cluster found";
                    continue;
                }

                LOG(DEBUG) << "Found associated cluster";
                double xdistance = intercept.X() - cluster->globalX();
                double ydistance = intercept.Y() - cluster->globalY();
                double xabsdistance = fabs(xdistance);
                double yabsdistance = fabs(ydistance);

                // We now have an associated cluster
                has_associated_cluster = true;
                // FIXME need to understand local coord of clusters - why shifted? what's normal?
                auto clusterLocal = m_detector->globalToLocal(cluster->global());
                hClusterMapAssoc->Fill(m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal));
                hClusterSizeMapAssoc->Fill(m_detector->getColumn(clusterLocal),
                                           m_detector->getRow(clusterLocal),
                                           static_cast<double>(cluster->size()));
                hClusterToTMapAssoc->Fill(
                    m_detector->getColumn(clusterLocal), m_detector->getRow(clusterLocal), cluster->tot());

                clusterTotAssoc->Fill(cluster->tot());

                // Cluster charge normalized to path length in sensor:
                double norm = 1; // FIXME fabs(cos( turn*wt )) * fabs(cos( tilt*wt ));
                auto normalized_charge = cluster->tot() * norm;
                clusterTotAssocNorm->Fill(normalized_charge);

                // Fill per-pixel histograms
                for(auto& pixel : (*cluster->pixels())) {
                    hHitMapAssoc->Fill(pixel->column(), pixel->row());
                    if(is_within_roi) {
                        hHitMapROI->Fill(pixel->column(), pixel->row());
                    }
                    hPixelToTAssoc->Fill(pixel->tot());
                    hPixelToTMapAssoc->Fill(pixel->column(), pixel->row(), pixel->tot());
                }

                associatedTracksVersusTime->Fill(static_cast<double>(Units::convert(track->timestamp(), "s")));

                // Residuals
                residualsX->Fill(xdistance);
                residualsY->Fill(ydistance);

                if(cluster->size() == 1) {
                    residualsX1pix->Fill(xdistance);
                    residualsY1pix->Fill(ydistance);
                }
                if(cluster->size() == 2) {
                    residualsX2pix->Fill(xdistance);
                    residualsY2pix->Fill(ydistance);
                }

                clusterSizeAssoc->Fill(static_cast<double>(cluster->size()));

                // Fill in-pixel plots: (all as function of track position within pixel cell)
                if(is_within_roi) {
                    qvsxmym->Fill(xmod, ymod, cluster->tot());                     // cluster charge profile
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
    }

    // Return value telling analysis to keep running
    return Success;
}
