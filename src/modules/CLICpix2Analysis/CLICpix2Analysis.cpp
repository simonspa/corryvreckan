#include "CLICpix2Analysis.h"

#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

using namespace corryvreckan;

CLICpix2Analysis::CLICpix2Analysis(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_DUT = m_config.get<std::string>("DUT");

    m_timeCutFrameEdge = m_config.get<double>("timeCutFrameEdge", Units::convert(20, "ns"));
    m_roi = m_config.getMatrix<int>("roi", std::vector<std::vector<int>>());

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
        if(!detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << " - track outside DUT area";
            continue;
        }

        // Check that track is within region of interest using winding number algorithm
        auto localIntercept = detector->globalToLocal(globalIntercept);
        if(winding_number(detector->getColumn(localIntercept), detector->getRow(localIntercept), m_roi) == 0) {
            is_within_roi = false;
        }

        // Check that it doesn't go through/near a masked pixel
        if(detector->hitMasked(track, 1.)) {
            LOG(DEBUG) << " - track close to masked pixel";
            continue;
        }

        // Discard tracks which are very close to the frame edges
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
            hPixelEfficiencyMap->Fill(Units::convert(detector->inPixelX(localIntercept), "um"),
                                      Units::convert(detector->inPixelY(localIntercept), "um"),
                                      has_associated_cluster);
        }
    }

    // Return value telling analysis to keep running
    return Success;
}

/* isLeft(): tests if a point is Left|On|Right of an infinite line.
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *    Input:  three points P0, P1, and P2
 *    Return: >0 for P2 left of the line through P0 and P1
 *            =0 for P2  on the line
 *            <0 for P2  right of the line
 *    See: Algorithm 1 "Area of Triangles and Polygons"
 */
int CLICpix2Analysis::isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2) {
    return ((pt1.first - pt0.first) * (pt2.second - pt0.second) - (pt2.first - pt0.first) * (pt1.second - pt0.second));
}

/* Winding number test for a point in a polygon
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *      Input:   x, y = a point,
 *               polygon = vector of vertex points of a polygon V[n+1] with V[n]=V[0]
 *      Return:  wn = the winding number (=0 only when P is outside)
 */
int CLICpix2Analysis::winding_number(int x, int y, std::vector<std::vector<int>> poly) {
    // Two points don't make an area
    if(poly.size() < 3) {
        LOG(DEBUG) << "No ROI given.";
        return 0;
    }

    int wn = 0; // the  winding number counter

    // loop through all edges of the polygon

    // edge from V[i] to  V[i+1]
    for(int i = 0; i < poly.size(); i++) {
        auto point_this = std::make_pair(poly.at(i).at(0), poly.at(i).at(1));
        auto point_next = (i + 1 < poly.size() ? std::make_pair(poly.at(i + 1).at(0), poly.at(i + 1).at(1))
                                               : std::make_pair(poly.at(0).at(0), poly.at(0).at(1)));

        // start y <= P.y
        if(point_this.second <= y) {
            // an upward crossing
            if(point_next.second > y) {
                // P left of  edge
                if(isLeft(point_this, point_next, std::make_pair(x, y)) > 0) {
                    // have  a valid up intersect
                    ++wn;
                }
            }
        } else {
            // start y > P.y (no test needed)

            // a downward crossing
            if(point_next.second <= y) {
                // P right of  edge
                if(isLeft(point_this, point_next, std::make_pair(x, y)) < 0) {
                    // have  a valid down intersect
                    --wn;
                }
            }
        }
    }
    return wn;
}
