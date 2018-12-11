#ifndef AnalysisCLICpix_H
#define AnalysisCLICpix_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <iostream>
#include <sstream>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class AnalysisCLICpix : public Module {

    public:
        // Constructors and destructors
        AnalysisCLICpix(Configuration config, std::shared_ptr<Detector> detector);

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;
        bool checkMasked(double, double);
        void fillClusterHistos(std::shared_ptr<Clusters>);
        bool checkProximity(Track*, std::shared_ptr<Tracks>);
        void fillResponseHistos(double, double, Cluster*);

        // Cluster/pixel histograms
        TH2F* hHitPixels;
        TH1F* hColumnHits;
        TH1F* hRowHits;

        TH1F* hClusterSizeAll;
        TH1F* hClusterTOTAll;
        TH1F* hClustersPerEvent;
        TH1F* hClustersVersusEventNo;
        TH1F* hClusterWidthRow;
        TH1F* hClusterWidthCol;

        // Track histograms
        TH1F* hGlobalTrackDifferenceX;
        TH1F* hGlobalTrackDifferenceY;
        TH1F* hGlobalResidualsX;
        TH1F* hGlobalResidualsY;
        TH1F* hAbsoluteResiduals;
        TH2F* hGlobalResidualsXversusX;
        TH2F* hGlobalResidualsXversusY;
        TH2F* hGlobalResidualsYversusY;
        TH2F* hGlobalResidualsYversusX;
        TH2F* hGlobalResidualsXversusColWidth;
        TH2F* hGlobalResidualsXversusRowWidth;
        TH2F* hGlobalResidualsYversusColWidth;
        TH2F* hGlobalResidualsYversusRowWidth;
        TH1F* hTrackInterceptRow;
        TH1F* hTrackInterceptCol;
        TH2F* hAbsoluteResidualMap;
        TH2F* hXresidualVersusYresidual;
        TH1F* hAssociatedClustersPerEvent;
        TH1F* hAssociatedClustersVersusEventNo;
        TH1F* hAssociatedClustersVersusTriggerNo;
        TH1F* hAssociatedClusterRow;
        TH1F* hAssociatedClusterColumn;
        TH1F* hFrameEfficiency;
        TH1F* hFrameTracks;
        TH1F* hFrameTracksAssociated;
        TH1F* hClusterSizeAssociated;
        TH1F* hClusterWidthRowAssociated;
        TH1F* hClusterWidthColAssociated;
        TH1F* hClusterTOTAssociated;
        TH1F* hClusterTOTAssociated1pix;
        TH1F* hClusterTOTAssociated2pix;
        TH1F* hClusterTOTAssociated3pix;
        TH1F* hClusterTOTAssociated4pix;
        TH1F* hPixelResponseX;
        TH1F* hPixelResponseGlobalX;
        TH1F* hPixelResponseXOddCol;
        TH1F* hPixelResponseXEvenCol;
        TH1F* hPixelResponseY;
        TH1F* hPixelResponseGlobalY;
        TH1F* hPixelResponseYOddCol;
        TH1F* hPixelResponseYEvenCol;
        TH2F* hEtaDistributionX;
        TH2F* hEtaDistributionY;
        TH1F* hResidualsLocalRow2pix;
        TH1F* hResidualsLocalCol2pix;
        TH1F* hClusterTOTRow2pix;
        TH1F* hClusterTOTCol2pix;
        TH1F* hPixelTOTRow2pix;
        TH1F* hPixelTOTCol2pix;
        TH1F* hClusterTOTRatioRow2pix;
        TH1F* hClusterTOTRatioCol2pix;

        // Maps
        TH2F* hTrackIntercepts;
        TH2F* hTrackInterceptsAssociated;
        TH2F* hGlobalClusterPositions;
        TH2F* hGlobalAssociatedClusterPositions;
        TH2F* hTrackInterceptsPixel;
        TH2F* hTrackInterceptsPixelAssociated;
        TH2F* hTrackInterceptsChip;
        TH2F* hTrackInterceptsChipAssociated;
        TH2F* hTrackInterceptsChipUnassociated;
        TH2F* hTrackInterceptsChipLost;
        TH2F* hPixelEfficiencyMap;
        TH2F* hChipEfficiencyMap;
        TH2F* hGlobalEfficiencyMap;
        TH2F* hInterceptClusterSize1;
        TH2F* hInterceptClusterSize2;
        TH2F* hInterceptClusterSize3;
        TH2F* hInterceptClusterSize4;
        TProfile2D* hPixelToTMap;

        TH2F* hMapClusterSizeAssociated;
        int m_nBinsX;
        int m_nBinsY;
        std::map<int, TH1F*> hMapClusterTOTAssociated1pix;

        // Member variables
        int m_eventNumber;
        int m_triggerNumber;
        std::map<int, double> m_hitPixels;
        double m_associationCut;
        double m_proximityCut;
        int m_lostHits;
        bool timepix3Telescope;
    };
} // namespace corryvreckan
#endif // AnalysisCLICpix_H
