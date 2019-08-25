#include "AnalysisCLICpix.h"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;
using namespace std;

AnalysisCLICpix::AnalysisCLICpix(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_associationCut = m_config.get<double>("association_cut", Units::get<double>(100, "um"));
    m_proximityCut = m_config.get<double>("proximity_cut", Units::get<double>(125, "um"));
    timepix3Telescope = m_config.get<bool>("timepix3Telescope", false);
}

template <typename T> std::string convertToString(T number) {
    std::ostringstream ss;
    ss << number;
    return ss.str();
}

void AnalysisCLICpix::initialise() {
    // Initialise member variables
    m_eventNumber = 0;
    m_triggerNumber = 0;
    m_lostHits = 0.;

    int maxcounter = 256;

    // Cluster/pixel histograms
    hHitPixels = new TH2F("hHitPixels",
                          "hHitPixels",
                          m_detector->nPixels().X(),
                          0,
                          m_detector->nPixels().X(),
                          m_detector->nPixels().Y(),
                          0,
                          m_detector->nPixels().Y());
    hColumnHits = new TH1F("hColumnHits", "hColumnHits", m_detector->nPixels().X(), 0, m_detector->nPixels().X());
    hRowHits = new TH1F("hRowHits", "hRowHits", m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hPixelToTMap = new TProfile2D("pixelToTMap",
                                  "pixelToTMap",
                                  m_detector->nPixels().X(),
                                  0,
                                  m_detector->nPixels().X(),
                                  m_detector->nPixels().Y(),
                                  0,
                                  m_detector->nPixels().Y(),
                                  0,
                                  maxcounter - 1);
    hClusterSizeAll = new TH1F("hClusterSizeAll", "hClusterSizeAll", 20, 0, 20);
    hClusterChargeAll = new TH1F("hClusterChargeAll", "hClusterChargeAll", 50, 0, 50);
    hClustersPerEvent = new TH1F("hClustersPerEvent", "hClustersPerEvent", 500, 0, 500);
    hClustersVersusEventNo = new TH1F("hClustersVersusEventNo", "hClustersVersusEventNo", 60000, 0, 60000);
    hGlobalClusterPositions = new TH2F("hGlobalClusterPositions", "hGlobalClusterPositions", 200, -2.0, 2.0, 300, -1., 2);

    hClusterWidthRow = new TH1F("hClusterWidthRow", "hClusterWidthRow", 20, 0, 20);
    hClusterWidthCol = new TH1F("hClusterWidthCol", "hClusterWidthCol", 20, 0, 20);

    // Track histograms
    hGlobalTrackDifferenceX = new TH1F("hGlobalTrackDifferenceX", "hGlobalTrackDifferenceX", 1000, -10., 10.);
    hGlobalTrackDifferenceY = new TH1F("hGlobalTrackDifferenceY", "hGlobalTrackDifferenceY", 1000, -10., 10.);

    hGlobalResidualsX = new TH1F("hGlobalResidualsX", "hGlobalResidualsX", 600, -0.3, 0.3);
    hGlobalResidualsY = new TH1F("hGlobalResidualsY", "hGlobalResidualsY", 600, -0.3, 0.3);
    hAbsoluteResiduals = new TH1F("hAbsoluteResiduals", "hAbsoluteResiduals", 600, 0., 0.600);
    hGlobalResidualsXversusX =
        new TH2F("hGlobalResidualsXversusX", "hGlobalResidualsXversusX", 600, -3., 3., 600, -0.3, 0.3);
    hGlobalResidualsXversusY =
        new TH2F("hGlobalResidualsXversusY", "hGlobalResidualsXversusY", 600, -3., 3., 600, -0.3, 0.3);
    hGlobalResidualsYversusY =
        new TH2F("hGlobalResidualsYversusY", "hGlobalResidualsYversusY", 600, -3., 3., 600, -0.3, 0.3);
    hGlobalResidualsYversusX =
        new TH2F("hGlobalResidualsYversusX", "hGlobalResidualsYversusX", 600, -3., 3., 600, -0.3, 0.3);

    hGlobalResidualsXversusColWidth =
        new TH2F("hGlobalResidualsXversusColWidth", "hGlobalResidualsXversusColWidth", 30, 0, 30, 600, -0.3, 0.3);
    hGlobalResidualsXversusRowWidth =
        new TH2F("hGlobalResidualsXversusRowWidth", "hGlobalResidualsXversusRowWidth", 30, 0, 30, 600, -0.3, 0.3);
    hGlobalResidualsYversusColWidth =
        new TH2F("hGlobalResidualsYversusColWidth", "hGlobalResidualsYversusColWidth", 30, 0, 30, 600, -0.3, 0.3);
    hGlobalResidualsYversusRowWidth =
        new TH2F("hGlobalResidualsYversusRowWidth", "hGlobalResidualsYversusRowWidth", 30, 0, 30, 600, -0.3, 0.3);

    hTrackInterceptRow = new TH1F("hTrackInterceptRow", "hTrackInterceptRow", 660, -1., 65.);
    hTrackInterceptCol = new TH1F("hTrackInterceptCol", "hTrackInterceptCol", 660, -1., 65.);

    hAbsoluteResidualMap = new TH2F("hAbsoluteResidualMap", "hAbsoluteResidualMap", 50, 0, 50, 25, 0, 25);
    hXresidualVersusYresidual =
        new TH2F("hXresidualVersusYresidual", "hXresidualVersusYresidual", 600, -0.3, 0.3, 600, -0.3, 0.3);

    hAssociatedClustersPerEvent = new TH1F("hAssociatedClustersPerEvent", "hAssociatedClustersPerEvent", 50, 0, 50);
    ;
    hAssociatedClustersVersusEventNo =
        new TH1F("hAssociatedClustersVersusEventNo", "hAssociatedClustersVersusEventNo", 60000, 0, 60000);
    hAssociatedClustersVersusTriggerNo =
        new TH1F("hAssociatedClustersVersusTriggerNo", "hAssociatedClustersVersusTriggerNo", 50, 0, 50);
    hAssociatedClusterRow =
        new TH1F("hAssociatedClusterRow", "hAssociatedClusterRow", m_detector->nPixels().Y(), 0, m_detector->nPixels().Y());
    hAssociatedClusterColumn = new TH1F(
        "hAssociatedClusterColumn", "hAssociatedClusterColumn", m_detector->nPixels().X(), 0, m_detector->nPixels().X());
    hFrameEfficiency = new TH1F("hFrameEfficiency", "hFrameEfficiency", 6000, 0, 6000);
    hFrameTracks = new TH1F("hFrameTracks", "hFrameTracks", 6000, 0, 6000);
    hFrameTracksAssociated = new TH1F("hFrameTracksAssociated", "hFrameTracksAssociated", 6000, 0, 6000);

    hClusterSizeAssociated = new TH1F("hClusterSizeAssociated", "hClusterSizeAssociated", 20, 0, 20);
    hClusterWidthRowAssociated = new TH1F("hClusterWidthRowAssociated", "hClusterWidthRowAssociated", 20, 0, 20);
    hClusterWidthColAssociated = new TH1F("hClusterWidthColAssociated", "hClusterWidthColAssociated", 20, 0, 20);

    hClusterChargeAssociated = new TH1F("hClusterChargeAssociated", "hClusterChargeAssociated", 50, 0, 50);
    hClusterChargeAssociated1pix = new TH1F("hClusterChargeAssociated1pix", "hClusterChargeAssociated1pix", 50, 0, 50);
    hClusterChargeAssociated2pix = new TH1F("hClusterChargeAssociated2pix", "hClusterChargeAssociated2pix", 50, 0, 50);
    hClusterChargeAssociated3pix = new TH1F("hClusterChargeAssociated3pix", "hClusterChargeAssociated3pix", 50, 0, 50);
    hClusterChargeAssociated4pix = new TH1F("hClusterChargeAssociated4pix", "hClusterChargeAssociated4pix", 50, 0, 50);

    hPixelResponseX = new TH1F("hPixelResponseX", "hPixelResponseX", 600, -0.3, 0.3);
    hPixelResponseY = new TH1F("hPixelResponseY", "hPixelResponseY", 600, -0.3, 0.3);

    hPixelResponseGlobalX = new TH1F("hPixelResponseGlobalX", "hPixelResponseGlobalX", 600, -0.3, 0.3);
    hPixelResponseGlobalY = new TH1F("hPixelResponseGlobalY", "hPixelResponseGlobalY", 600, -0.3, 0.3);

    hPixelResponseXOddCol = new TH1F("hPixelResponseXOddCol", "hPixelResponseXOddCol", 600, -0.3, 0.3);
    hPixelResponseXEvenCol = new TH1F("hPixelResponseXEvenCol", "hPixelResponseXEvenCol", 600, -0.3, 0.3);
    hPixelResponseYOddCol = new TH1F("hPixelResponseYOddCol", "hPixelResponseYOddCol", 600, -0.3, 0.3);
    hPixelResponseYEvenCol = new TH1F("hPixelResponseYEvenCol", "hPixelResponseYEvenCol", 600, -0.3, 0.3);

    hEtaDistributionX = new TH2F("hEtaDistributionX", "hEtaDistributionX", 25, 0, 25, 25, 0, 25);
    hEtaDistributionY = new TH2F("hEtaDistributionY", "hEtaDistributionY", 25, 0, 25, 25, 0, 25);

    hResidualsLocalRow2pix = new TH1F("hResidualsLocalRow2pix", "hResidualsLocalRow2pix", 600, -0.3, 0.3);
    hResidualsLocalCol2pix = new TH1F("hResidualsLocalCol2pix", "hResidualsLocalCol2pix", 600, -0.3, 0.3);
    hClusterChargeRow2pix = new TH1F("hClusterChargeRow2pix", "hClusterChargeRow2pix", 50, 0, 50);
    hClusterChargeCol2pix = new TH1F("hClusterChargeCol2pix", "hClusterChargeCol2pix", 50, 0, 50);
    hClusterChargeRatioRow2pix = new TH1F("hClusterChargeRatioRow2pix", "hClusterChargeRatioRow2pix", 100, 0, 1);
    hClusterChargeRatioCol2pix = new TH1F("hClusterChargeRatioCol2pix", "hClusterChargeRatioCol2pix", 100, 0, 1);
    hPixelTOTRow2pix = new TH1F("hPixelTOTRow2pix", "hPixelTOTRow2pix", 50, 0, 50);
    hPixelTOTCol2pix = new TH1F("hPixelTOTCol2pix", "hPixelTOTCol2pix", 50, 0, 50);

    // Maps
    hTrackIntercepts = new TH2F("hTrackIntercepts", "hTrackIntercepts", 200, -2.0, 2.0, 300, -1., 2);
    hTrackInterceptsAssociated =
        new TH2F("hTrackInterceptsAssociated", "hTrackInterceptsAssociated", 200, -2.0, 2.0, 300, -1., 2);
    hGlobalAssociatedClusterPositions =
        new TH2F("hGlobalAssociatedClusterPositions", "hGlobalAssociatedClusterPositions", 200, -2.0, 2.0, 300, -1., 2);
    hTrackInterceptsPixel = new TH2F("hTrackInterceptsPixel", "hTrackInterceptsPixel", 50, 0, 50, 25, 0, 25);
    hTrackInterceptsPixelAssociated =
        new TH2F("hTrackInterceptsPixelAssociated", "hTrackInterceptsPixelAssociated", 50, 0, 50, 25, 0, 25);
    hTrackInterceptsChip = new TH2F("hTrackInterceptsChip",
                                    "hTrackInterceptsChip",
                                    m_detector->nPixels().X() + 1,
                                    -0.5,
                                    m_detector->nPixels().X() + 0.5,
                                    m_detector->nPixels().Y() + 1,
                                    -0.5,
                                    m_detector->nPixels().Y() + 0.5);
    hTrackInterceptsChipAssociated = new TH2F("hTrackInterceptsChipAssociated",
                                              "hTrackInterceptsChipAssociated",
                                              m_detector->nPixels().X() + 1,
                                              -0.5,
                                              m_detector->nPixels().X() + 0.5,
                                              m_detector->nPixels().Y() + 1,
                                              -0.5,
                                              m_detector->nPixels().Y() + 0.5);
    hTrackInterceptsChipUnassociated = new TH2F("hTrackInterceptsChipUnassociated",
                                                "hTrackInterceptsChipUnassociated",
                                                m_detector->nPixels().X() + 1,
                                                -0.5,
                                                m_detector->nPixels().X() + 0.5,
                                                m_detector->nPixels().Y() + 1,
                                                -0.5,
                                                m_detector->nPixels().Y() + 0.5);
    hTrackInterceptsChipLost = new TH2F("hTrackInterceptsChipLost",
                                        "hTrackInterceptsChipLost",
                                        m_detector->nPixels().X() + 1,
                                        -0.5,
                                        m_detector->nPixels().X() + 0.5,
                                        m_detector->nPixels().Y() + 1,
                                        -0.5,
                                        m_detector->nPixels().Y() + 0.5);

    hPixelEfficiencyMap = new TH2F("hPixelEfficiencyMap", "hPixelEfficiencyMap", 50, 0, 50, 25, 0, 25);
    hChipEfficiencyMap = new TH2F("hChipEfficiencyMap",
                                  "hChipEfficiencyMap",
                                  m_detector->nPixels().X() + 1,
                                  -0.5,
                                  m_detector->nPixels().X() + 0.5,
                                  m_detector->nPixels().Y() + 1,
                                  -0.5,
                                  m_detector->nPixels().Y() + 0.5);
    hGlobalEfficiencyMap = new TH2F("hGlobalEfficiencyMap", "hGlobalEfficiencyMap", 200, -2.0, 2.0, 300, -1., 2);

    hInterceptClusterSize1 = new TH2F("hInterceptClusterSize1", "hInterceptClusterSize1", 25, 0, 25, 25, 0, 25);
    hInterceptClusterSize2 = new TH2F("hInterceptClusterSize2", "hInterceptClusterSize2", 25, 0, 25, 25, 0, 25);
    hInterceptClusterSize3 = new TH2F("hInterceptClusterSize3", "hInterceptClusterSize3", 25, 0, 25, 25, 0, 25);
    hInterceptClusterSize4 = new TH2F("hInterceptClusterSize4", "hInterceptClusterSize4", 25, 0, 25, 25, 0, 25);

    m_nBinsX = 32;
    m_nBinsY = 32;
    hMapClusterSizeAssociated = new TH2F("hMapClusterSizeAssociated",
                                         "hMapClusterSizeAssociated",
                                         m_nBinsX,
                                         0,
                                         m_detector->nPixels().X(),
                                         m_nBinsY,
                                         0,
                                         m_detector->nPixels().Y());

    for(int x = 0; x < m_nBinsX; x++) {
        for(int y = 0; y < m_nBinsY; y++) {
            int id = x + y * m_nBinsX;
            std::string name = "hMapClusterChargeAssociated1pix" + convertToString(id);
            TH1F* hMapEntryClusterChargeAssociated1pix = new TH1F(name.c_str(), name.c_str(), 50, 0, 50);
            hMapClusterChargeAssociated1pix[id] = hMapEntryClusterChargeAssociated1pix;
        }
    }
}

StatusCode AnalysisCLICpix::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the clicpix clusters in this event
    auto clusters = clipboard->get<Cluster>(m_detector->name());
    if(clusters == nullptr) {
        LOG(DEBUG) << "No clusters for " << m_detector->name() << " on the clipboard";
        return StatusCode::Success;
    }

    // If this is the first or last trigger don't use the data
    if((m_triggerNumber == 0 || m_triggerNumber == 19) && !timepix3Telescope) {
        m_eventNumber++;
        m_triggerNumber++;
        return StatusCode::Success;
    }

    // Fill the histograms that only need clusters/pixels
    fillClusterHistos(clusters);

    // Get the tracks in this event
    auto tracks = clipboard->get<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Set counters
    double nClustersAssociated = 0, nValidTracks = 0;

    // Loop over tracks, and compare with Clicpix clusters
    for(auto& track : (*tracks)) {
        if(!track) {
            continue;
        }

        // Cut on the track chi2/ndof
        if(track->chi2ndof() < 3.0) {
            continue;
        }

        // Get the track intercept with the clicpix plane (global and local
        // co-ordinates)
        PositionVector3D<Cartesian3D<double>> trackIntercept = m_detector->getIntercept(track);
        PositionVector3D<Cartesian3D<double>> trackInterceptLocal = m_detector->globalToLocal(trackIntercept);

        // Plot the difference between track intercepts and all clicpix clusters
        // Also record which cluster is the closest
        bool matched = false;
        auto trackclusters = track->clusters();
        Cluster* bestCluster = nullptr;
        double xresidualBest = 10000.;
        double yresidualBest = 10000.;
        double absoluteresidualBest = sqrt(xresidualBest * xresidualBest + yresidualBest * yresidualBest);

        for(auto& cluster : (trackclusters)) {

            // Get the distance between this cluster and the track intercept (global)
            double xcorr = cluster->global().x() - trackIntercept.X();
            double ycorr = cluster->global().y() - trackIntercept.Y();

            // Fill histograms on correlations
            hGlobalTrackDifferenceX->Fill(xcorr);
            hGlobalTrackDifferenceY->Fill(ycorr);

            // Look if this cluster can be considered associated.
            // Cut on the distance from the track in x and y
            if(fabs(ycorr) > m_associationCut || fabs(xcorr) > m_associationCut)
                continue;

            // Found a matching cluster
            matched = true;

            // Check if this is a better match than the previously found cluster, and
            // store the information
            double absoluteresidual = sqrt(xcorr * xcorr + ycorr * ycorr);
            if(absoluteresidual <= absoluteresidualBest) {
                absoluteresidualBest = absoluteresidual;
                xresidualBest = xcorr;
                yresidualBest = ycorr;
                bestCluster = cluster;
            }
        }

        // Get the track intercept position along the chip
        double chipInterceptCol = m_detector->getColumn(trackInterceptLocal);
        double chipInterceptRow = m_detector->getRow(trackInterceptLocal);

        // Get the track intercept position along the pixel
        double pixelInterceptX = m_detector->inPixel(trackInterceptLocal).X();
        double pixelInterceptY = m_detector->inPixel(trackInterceptLocal).Y();

        // Cut on the track intercept - this makes sure that it actually went
        // through the chip
        if(chipInterceptCol < 0.5 || chipInterceptRow < 0.5 || chipInterceptCol > (m_detector->nPixels().X() - 0.5) ||
           chipInterceptRow > (m_detector->nPixels().Y() - 0.5))
            continue;

        // Check if the hit is near a masked pixel
        bool hitMasked = checkMasked(chipInterceptRow, chipInterceptCol);
        if(hitMasked)
            continue;

        // Check there are no other tracks nearby
        bool proximityCut = checkProximity(track, tracks);
        if(proximityCut)
            continue;

        // Now have tracks that went through the device
        hTrackIntercepts->Fill(trackIntercept.X(), trackIntercept.Y());
        hFrameTracks->Fill(m_eventNumber);
        nValidTracks++;
        hTrackInterceptsChip->Fill(chipInterceptCol, chipInterceptRow);
        hTrackInterceptsPixel->Fill(pixelInterceptX, pixelInterceptY);

        if(matched) {

            // Some test plots of pixel response function
            fillResponseHistos(trackIntercept.X(), trackIntercept.Y(), (bestCluster));

            // Add this cluster to the list of associated clusters held by this track.
            // This will allow alignment to take place
            track->addAssociatedCluster(bestCluster);

            // Now fill all of the histograms/efficiency counters that we want
            hTrackInterceptsAssociated->Fill(trackIntercept.X(), trackIntercept.Y());
            hGlobalResidualsX->Fill(xresidualBest);
            hGlobalResidualsY->Fill(yresidualBest);
            hGlobalAssociatedClusterPositions->Fill((bestCluster)->global().x(), (bestCluster)->global().y());
            nClustersAssociated++;
            hAssociatedClusterRow->Fill((bestCluster)->row());
            hAssociatedClusterColumn->Fill((bestCluster)->column());
            hTrackInterceptsChipAssociated->Fill(chipInterceptCol, chipInterceptRow);
            hTrackInterceptsPixelAssociated->Fill(pixelInterceptX, pixelInterceptY);
            hFrameTracksAssociated->Fill(m_eventNumber);
            hGlobalResidualsXversusX->Fill(trackIntercept.X(), xresidualBest);
            hGlobalResidualsXversusY->Fill(trackIntercept.Y(), xresidualBest);
            hGlobalResidualsYversusY->Fill(trackIntercept.Y(), yresidualBest);
            hGlobalResidualsYversusX->Fill(trackIntercept.X(), yresidualBest);
            //      hGlobalResidualsXversusColWidth->Fill((*bestCluster)->colWidth(),xresidualBest);
            //      hGlobalResidualsXversusRowWidth->Fill((*bestCluster)->rowWidth(),xresidualBest);
            //      hGlobalResidualsYversusColWidth->Fill((*bestCluster)->colWidth(),yresidualBest);
            //      hGlobalResidualsYversusRowWidth->Fill((*bestCluster)->rowWidth(),yresidualBest);
            hAbsoluteResidualMap->Fill(
                pixelInterceptX, pixelInterceptY, sqrt(xresidualBest * xresidualBest + yresidualBest * yresidualBest));
            hXresidualVersusYresidual->Fill(xresidualBest, yresidualBest);
            hAbsoluteResiduals->Fill(sqrt(xresidualBest * xresidualBest + yresidualBest * yresidualBest));
            hClusterChargeAssociated->Fill((bestCluster)->charge());
            hClusterSizeAssociated->Fill(static_cast<double>(bestCluster->size()));
            //      hClusterWidthColAssociated->Fill((*bestCluster)->colWidth());
            //      hClusterWidthRowAssociated->Fill((*bestCluster)->rowWidth());

            hMapClusterSizeAssociated->Fill(chipInterceptCol, chipInterceptRow, (bestCluster)->charge());

            if((bestCluster)->size() == 1) {
                hClusterChargeAssociated1pix->Fill((bestCluster)->charge());
                hInterceptClusterSize1->Fill(pixelInterceptX, pixelInterceptY);
                int id = static_cast<int>(floor(chipInterceptCol * m_nBinsX / m_detector->nPixels().X()) +
                                          floor(chipInterceptRow * m_nBinsY / m_detector->nPixels().Y()) * m_nBinsX);
                hMapClusterChargeAssociated1pix[id]->Fill((bestCluster)->charge());
            }
            if((bestCluster)->size() == 2) {
                hClusterChargeAssociated2pix->Fill((bestCluster)->charge());
                hInterceptClusterSize2->Fill(pixelInterceptX, pixelInterceptY);
            }
            if((bestCluster)->size() == 3) {
                hClusterChargeAssociated3pix->Fill((bestCluster)->charge());
                hInterceptClusterSize3->Fill(pixelInterceptX, pixelInterceptY);
            }
            if((bestCluster)->size() == 4) {
                hClusterChargeAssociated4pix->Fill((bestCluster)->charge());
                hInterceptClusterSize4->Fill(pixelInterceptX, pixelInterceptY);
            }
            auto pixels = bestCluster->pixels();
            for(auto& pixel : pixels) {
                hPixelToTMap->Fill(pixel->column(), pixel->row(), pixel->raw());
            }

        } else {

            // Search for lost hits. Basically loop through all pixels in all clusters
            // and see if any are close.
            // Large clusters (such as from deltas) can pull the cluster centre
            // sufficiently far from the track
            bool pixelMatch = false;
            /*      for (itCorrelate = clusters->begin(); itCorrelate != clusters->end(); ++itCorrelate) {
        if(pixelMatch) break;
        // Loop over pixels
        PixelVector::const_iterator itPixel;
        for (itPixel = (*itCorrelate)->pixels().begin(); itPixel != (*itCorrelate)->pixels().end(); itPixel++) {

          // Get the pixel global position
          LOG(TRACE) <<"New pixel, row = "<<(*itPixel)->row()<<", column = "<<(*itPixel)->column();
          PositionVector3D<Cartesian3D<double> > pixelPositionLocal =
      m_detector->getLocalPosition((*itPixel)->column(),(*itPixel)->row()); PositionVector3D<Cartesian3D<double> >
      pixelPositionGlobal = *(m_detector->m_localToGlobal) * pixelPositionLocal;

          // Check if it is close to the track
          if( fabs( pixelPositionGlobal.X() - trackIntercept.X() ) > m_associationCut ||
              fabs( pixelPositionGlobal.Y() - trackIntercept.Y() ) > m_associationCut ) continue;

          pixelMatch=true;
          break;
        }
      }
      // Increment counter for number of hits found this way
*/ if(pixelMatch) {
                m_lostHits++;
                hTrackInterceptsChipLost->Fill(chipInterceptCol, chipInterceptRow);
            } //*/

            if(!pixelMatch)
                hTrackInterceptsChipUnassociated->Fill(chipInterceptCol, chipInterceptRow);
        }
    }

    // Histograms relating to this mimosa frame
    hAssociatedClustersPerEvent->Fill(nClustersAssociated);
    hAssociatedClustersVersusEventNo->Fill(m_eventNumber, nClustersAssociated);
    hAssociatedClustersVersusTriggerNo->Fill(m_triggerNumber, nClustersAssociated);
    if(nValidTracks != 0)
        hFrameEfficiency->Fill(m_eventNumber, nClustersAssociated / nValidTracks);

    // Increment event counter and trigger number
    m_eventNumber++;
    m_triggerNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisCLICpix::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";

    // Compare number of valid tracks, and number of clusters associated in order
    // to get global efficiency
    int nTracks = static_cast<int>(hTrackIntercepts->GetEntries());
    int nClusters = static_cast<int>(hGlobalAssociatedClusterPositions->GetEntries());
    double efficiency = nClusters / nTracks;
    double errorEfficiency = (1. / nTracks) * sqrt(nClusters * (1 - efficiency));

    if(nTracks == 0) {
        efficiency = 0.;
        errorEfficiency = 1.;
    }

    LOG(INFO) << "***** Clicpix efficiency calculation *****";
    LOG(INFO) << "***** ntracks: " << nTracks << ", nclusters " << nClusters;
    LOG(INFO) << "***** Efficiency: " << 100. * efficiency << " +/- " << 100. * errorEfficiency << " %";
    LOG(INFO) << "***** If including the " << m_lostHits << " lost pixel hits, this becomes "
              << 100. * (m_lostHits + nClusters) / nTracks << " %";
}

// Check if a track has gone through or near a masked pixel
bool AnalysisCLICpix::checkMasked(double chipInterceptRow, double chipInterceptCol) {

    // Get the pixel row and column number
    int rowNumber = static_cast<int>(ceil(chipInterceptRow));
    int colNumber = static_cast<int>(ceil(chipInterceptCol));

    // Check if that pixel is masked, or the neighbour to the left, right, etc...
    if(m_detector->masked(colNumber - 1, rowNumber - 1))
        return true;
    if(m_detector->masked(colNumber, rowNumber - 1))
        return true;
    if(m_detector->masked(colNumber + 1, rowNumber - 1))
        return true;
    if(m_detector->masked(colNumber - 1, rowNumber))
        return true;
    if(m_detector->masked(colNumber, rowNumber))
        return true;
    if(m_detector->masked(colNumber + 1, rowNumber))
        return true;
    if(m_detector->masked(colNumber - 1, rowNumber + 1))
        return true;
    if(m_detector->masked(colNumber, rowNumber + 1))
        return true;
    if(m_detector->masked(colNumber + 1, rowNumber + 1))
        return true;

    // If not masked
    return false;
}

// Check if there is another track close to the selected track.
// "Close" is defined as the intercept at the clicpix
bool AnalysisCLICpix::checkProximity(Track* track, std::shared_ptr<TrackVector> tracks) {

    // Get the intercept of the interested track at the dut
    bool close = false;
    PositionVector3D<Cartesian3D<double>> trackIntercept = m_detector->getIntercept(track);

    // Loop over all other tracks and check if they intercept close to the track
    // we are considering
    TrackVector::iterator itTrack;
    for(itTrack = tracks->begin(); itTrack != tracks->end(); itTrack++) {

        // Get the track
        Track* track2 = (*itTrack);
        // Get the track intercept with the clicpix plane (global co-ordinates)
        PositionVector3D<Cartesian3D<double>> trackIntercept2 = m_detector->getIntercept(track2);
        // If track == track2 do nothing
        if(trackIntercept.X() == trackIntercept2.X() && trackIntercept.Y() == trackIntercept2.Y())
            continue;
        if(fabs(trackIntercept.X() - trackIntercept2.X()) <= m_proximityCut ||
           fabs(trackIntercept.Y() - trackIntercept2.Y()) <= m_proximityCut)
            close = true;
    }
    return close;
}

// Small sub-routine to fill histograms that only need clusters
void AnalysisCLICpix::fillClusterHistos(std::shared_ptr<ClusterVector> clusters) {

    // Pick up column to generate unique pixel id
    int nCols = m_detector->nPixels().X();
    ClusterVector::iterator itc;

    // Check if this is a new clicpix frame (each frame may be in several events)
    // and
    // fill histograms
    bool newFrame = false;
    for(itc = clusters->begin(); itc != clusters->end(); ++itc) {

        // Loop over pixels and check if there are pixels not known
        auto pixels = (*itc)->pixels();
        for(auto& itp : pixels) {
            // Check if this clicpix frame is still the current
            int pixelID = itp->column() + nCols * itp->row();
            if(m_hitPixels[pixelID] != itp->raw()) {
                // New frame! Reset the stored pixels and trigger number
                if(!newFrame) {
                    m_hitPixels.clear();
                    newFrame = true;
                }
                m_hitPixels[pixelID] = itp->raw();
                m_triggerNumber = 0;
            }
            hHitPixels->Fill(itp->column(), itp->row());
            hColumnHits->Fill(itp->column());
            hRowHits->Fill(itp->row());
        }

        // Fill cluster histograms
        hClusterSizeAll->Fill(static_cast<double>((*itc)->size()));
        hClusterChargeAll->Fill((*itc)->charge());
        hGlobalClusterPositions->Fill((*itc)->global().x(), (*itc)->global().y());
    }

    hClustersPerEvent->Fill(static_cast<double>(clusters->size()));
    hClustersVersusEventNo->Fill(m_eventNumber, static_cast<double>(clusters->size()));

    return;
}

// Sub-routine to look at pixel response, ie. how far from the pixel is the
// track intercept for the pixel to still see charge
void AnalysisCLICpix::fillResponseHistos(double trackInterceptX, double trackInterceptY, Cluster* cluster) {

    // Loop over pixels in the cluster and show their distance from the track
    // intercept
    auto pixels = cluster->pixels();
    for(auto& pixel : pixels) {
        // Get the pixel local then global position
        PositionVector3D<Cartesian3D<double>> pixelPositionLocal =
            m_detector->getLocalPosition(pixel->column(), pixel->row());
        PositionVector3D<Cartesian3D<double>> pixelPositionGlobal = m_detector->localToGlobal(pixelPositionLocal);

        // Fill the response histograms
        hPixelResponseX->Fill(pixelPositionGlobal.X() - trackInterceptX);
        hPixelResponseY->Fill(pixelPositionGlobal.Y() - trackInterceptY);
    }

    return;
}
