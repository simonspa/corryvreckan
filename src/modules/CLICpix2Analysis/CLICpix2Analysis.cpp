#include "CLICpix2Analysis.h"

#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

using namespace corryvreckan;

CLICpix2Analysis::CLICpix2Analysis(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    m_DUT = m_config.get<std::string>("DUT");

    m_timeCutFrameEdge = m_config.get<double>("timeCutFrameEdge", Units::convert(20, "ns"));

    spatialCut = m_config.get<double>("spatialCut", Units::convert(50, "um"));
    chi2ndofCut = m_config.get<double>("chi2ndofCut", 3.);
}

void CLICpix2Analysis::initialise() {

    // Get DUT detector:
    auto det = get_detector(m_DUT);

    hClusterMapAssoc = new TH2F(
        "clusterMapAssoc", "clusterMapAssoc", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    hClusterSizeMapAssoc = new TProfile2D("clusterSizeMapAssoc",
                                          "clusterSizeMapAssoc",
                                          det->nPixelsX(),
                                          0,
                                          det->nPixelsX(),
                                          det->nPixelsY(),
                                          0,
                                          det->nPixelsY(),
                                          0,
                                          100);

    hClusterToTMapAssoc = new TProfile2D("clusterSizeToTAssoc",
                                         "clusterToTMapAssoc",
                                         det->nPixelsX(),
                                         0,
                                         det->nPixelsX(),
                                         det->nPixelsY(),
                                         0,
                                         det->nPixelsY(),
                                         0,
                                         1000);

    // Per-pixel histograms
    hHitMapAssoc =
        new TH2F("hitMapAssoc", "hitMapAssoc", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    hHitMapROI =
        new TH2F("hitMapROI", "hitMapROI", det->nPixelsX(), 0, det->nPixelsX(), det->nPixelsY(), 0, det->nPixelsY());
    hPixelToTAssoc = new TH1F("pixelToTAssoc", "pixelToTAssoc", 32, 0, 31);
    hPixelToTMapAssoc = new TProfile2D("pixelToTMapAssoc",
                                       "pixelToTMapAssoc",
                                       det->nPixelsX(),
                                       0,
                                       det->nPixelsX(),
                                       det->nPixelsY(),
                                       0,
                                       det->nPixelsY(),
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
    auto pitch_x = Units::convert(det->pitch().X(), "um");
    auto pitch_y = Units::convert(det->pitch().Y(), "um");
    auto mod_axes = "x_{track} mod " + std::to_string(pitch_x) + "#mum;y_{track} mod " + std::to_string(pitch_y) + "#mum;";

    std::string title = "DUT x resolution;" + mod_axes + "MAD(#Deltax) [#mum]";
    rmsxvsxmym = new TProfile2D("rmsxvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT y resolution;" + mod_axes + "MAD(#Deltay) [#mum]";
    rmsyvsxmym = new TProfile2D("rmsyvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT resolution;" + mod_axes + "MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    rmsxyvsxmym = new TProfile2D("rmsxyvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT cluster charge map;" + mod_axes + "<cluster charge> [ke]";
    qvsxmym = new TProfile2D("qvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y, 0, 250);

    title = "DUT cluster charge map, Moyal approx;" + mod_axes + "cluster charge MPV [ke]";
    qMoyalvsxmym = new TProfile2D("qMoyalvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y, 0, 250);

    title = "DUT seed pixel charge map;" + mod_axes + "<seed pixel charge> [ke]";
    pxqvsxmym = new TProfile2D("pxqvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y, 0, 250);

    title = "DUT cluster size map;" + mod_axes + "<pixels/cluster>";
    npxvsxmym = new TProfile2D("npxvsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y, 0, 4.5);

    title = "DUT 1-pixel cluster map;" + mod_axes + "clusters";
    npx1vsxmym = new TH2F("npx1vsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT 2-pixel cluster map;" + mod_axes + "clusters";
    npx2vsxmym = new TH2F("npx2vsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT 3-pixel cluster map;" + mod_axes + "clusters";
    npx3vsxmym = new TH2F("npx3vsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    title = "DUT 4-pixel cluster map;" + mod_axes + "clusters";
    npx4vsxmym = new TH2F("npx4vsxmym", title.c_str(), pitch_x, 0, pitch_x, pitch_y, 0, pitch_y);

    // Efficiency maps
    hPixelEfficiencyMap =
        new TProfile2D("hPixelEfficiencyMap", "hPixelEfficiencyMap", pitch_x, 0, pitch_x, pitch_y, 0, pitch_y, 0, 1);
    hChipEfficiencyMap = new TProfile2D("hChipEfficiencyMap",
                                        "hChipEfficiencyMap",
                                        det->nPixelsX(),
                                        0,
                                        det->nPixelsX(),
                                        det->nPixelsY(),
                                        0,
                                        det->nPixelsY(),
                                        0,
                                        1);
    hGlobalEfficiencyMap = new TProfile2D("hGlobalEfficiencyMap",
                                          "hGlobalEfficiencyMap",
                                          300,
                                          -1.5 * det->size().X(),
                                          1.5 * det->size().X(),
                                          300,
                                          -1.5 * det->size().Y(),
                                          1.5 * det->size().Y(),
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

StatusCode CLICpix2Analysis::run(Clipboard* clipboard) {

    // Get the telescope tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT detector:
    auto detector = get_detector(m_DUT);

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
        auto globalIntercept = detector->getIntercept(track);
        auto localIntercept = detector->globalToLocal(globalIntercept);

        if(!detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        if(!detector->isWithinROI(track)) {
            is_within_roi = false;
        }

        // Check that it doesn't go through/near a masked pixel
        if(detector->hitMasked(track, 1.)) {
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
        double xmod = Units::convert(detector->inPixelX(localIntercept), "um");
        double ymod = Units::convert(detector->inPixelY(localIntercept), "um");

        // Get the DUT clusters from the clipboard
        Clusters* clusters = (Clusters*)clipboard->get(m_DUT, "clusters");
        if(clusters == NULL) {
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

                double xdistance = intercept.X() - cluster->globalX();
                double ydistance = intercept.Y() - cluster->globalY();
                double xabsdistance = fabs(xdistance);
                double yabsdistance = fabs(ydistance);
                if(xabsdistance > spatialCut || yabsdistance > spatialCut) {
                    LOG(DEBUG) << "    - Discarding DUT cluster with distance (" << Units::display(xabsdistance, {"um"})
                               << "," << Units::display(yabsdistance, {"um"}) << ")";
                    continue;
                }

                LOG(DEBUG) << "    - Found associated cluster with distance (" << Units::display(xabsdistance, {"um"}) << ","
                           << Units::display(yabsdistance, {"um"}) << ")";

                // We now have an associated cluster
                has_associated_cluster = true;
                // FIXME need to understand local coord of clusters - why shifted? what's normal?
                auto clusterLocal = detector->globalToLocal(cluster->global());
                hClusterMapAssoc->Fill(detector->getColumn(clusterLocal), detector->getRow(clusterLocal));
                hClusterSizeMapAssoc->Fill(
                    detector->getColumn(clusterLocal), detector->getRow(clusterLocal), cluster->size());
                hClusterToTMapAssoc->Fill(detector->getColumn(clusterLocal), detector->getRow(clusterLocal), cluster->tot());

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

                associatedTracksVersusTime->Fill(Units::convert(track->timestamp(), "s"));

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

                clusterSizeAssoc->Fill(cluster->size());

                // Fill in-pixel plots: (all as function of track position within pixel cell)
                if(is_within_roi) {
                    qvsxmym->Fill(xmod, ymod, cluster->tot());                     // cluster charge profile
                    qMoyalvsxmym->Fill(xmod, ymod, exp(-normalized_charge / 3.5)); // norm. cluster charge profile

                    // mean charge of cluster seed
                    pxqvsxmym->Fill(xmod, ymod, cluster->getSeedPixel()->charge());

                    // mean cluster size
                    npxvsxmym->Fill(xmod, ymod, cluster->size());
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
            detector->getColumn(localIntercept), detector->getRow(localIntercept), has_associated_cluster);

        // For pixels, only look at the ROI:
        if(is_within_roi) {
            hPixelEfficiencyMap->Fill(xmod, ymod, has_associated_cluster);
        }
    }

    // Return value telling analysis to keep running
    return Success;
}
