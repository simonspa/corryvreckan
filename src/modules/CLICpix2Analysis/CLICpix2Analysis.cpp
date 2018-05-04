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
    clusterSizeAssoc = new TH1F("clusterSizeAssociated", "clusterSizeAssociated", 30, 0, 30);

    // Efficiency maps
    hPixelEfficiencyMap = new TProfile2D("hPixelEfficiencyMap",
                                         "hPixelEfficiencyMap",
                                         Units::convert(det->pitch().X(), "um"),
                                         0,
                                         Units::convert(det->pitch().X(), "um"),
                                         Units::convert(det->pitch().Y(), "um"),
                                         0,
                                         Units::convert(det->pitch().Y(), "um"),
                                         0,
                                         1);
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
        // Flag for efficiency calculation
        bool cluster_associated = false;

        LOG(DEBUG) << "Looking at next track";

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            continue;
        }

        // Check if it intercepts the DUT
        PositionVector3D<Cartesian3D<double>> globalIntercept = detector->getIntercept(track);
        if(!detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
            continue;
        }

        // Discard tracks which are very close to the frame edges
        //
        if(fabs(track->timestamp() - clipboard->get_persistent("currentTime")) < m_timeCutFrameEdge) {
            // Late edge - the currentTime has been updated by Timexpi3EventLoader to point to the end of the frame`
            LOG(DEBUG) << " - track close to end of readout frame: "
                       << Units::display(fabs(track->timestamp() - clipboard->get_persistent("currentTime")), {"us", "ns"})
                       << " at " << Units::display(track->timestamp(), {"us"});
            continue;
        } else if(fabs(track->timestamp() - clipboard->get_persistent("currentTime") +
                       clipboard->get_persistent("eventLength")) < m_timeCutFrameEdge) {
            // Early edge - subtract the eventLength from the current time to see the beginning of the frame
            LOG(DEBUG) << " - track close to start of readout frame: "
                       << Units::display(fabs(track->timestamp() - clipboard->get_persistent("currentTime") +
                                              clipboard->get_persistent("eventLength")),
                                         {"us", "ns"})
                       << " at " << Units::display(track->timestamp(), {"us"});
            continue;
        }

        // Check that it doesn't go through/near a masked pixel
        if(detector->hitMasked(track, 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }

        // FIXME check that track is within fiducial area

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
                cluster_associated = true;
                // FIXME need to understand local coord of clusters - why shifted? what's normal?
                auto clusterLocal = detector->globalToLocal(cluster->global());
                hClusterMapAssoc->Fill(detector->getColumn(clusterLocal), detector->getRow(clusterLocal));
                hClusterSizeMapAssoc->Fill(
                    detector->getColumn(clusterLocal), detector->getRow(clusterLocal), cluster->size());
                hClusterToTMapAssoc->Fill(detector->getColumn(clusterLocal), detector->getRow(clusterLocal), cluster->tot());

                clusterTotAssoc->Fill(cluster->tot());

                // Fill per-pixel histograms
                for(auto& pixel : (*cluster->pixels())) {
                    hHitMapAssoc->Fill(pixel->column(), pixel->row());
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

                track->addAssociatedCluster(cluster);
                hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());

                // Only allow one associated cluster per track
                break;
            }
        }

        // Efficiency plots:
        hGlobalEfficiencyMap->Fill(globalIntercept.X(), globalIntercept.Y(), cluster_associated);

        auto localIntercept = detector->globalToLocal(globalIntercept);
        hChipEfficiencyMap->Fill(detector->getColumn(localIntercept), detector->getRow(localIntercept), cluster_associated);
        hPixelEfficiencyMap->Fill(Units::convert(detector->inPixelX(localIntercept), "um"),
                                  Units::convert(detector->inPixelY(localIntercept), "um"),
                                  cluster_associated);
    }

    // Return value telling analysis to keep running
    return Success;
}
