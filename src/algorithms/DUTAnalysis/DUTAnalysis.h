#ifndef DUTAnalysis_H
#define DUTAnalysis_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"

namespace corryvreckan {
    class DUTAnalysis : public Algorithm {

    public:
        // Constructors and destructors
        DUTAnalysis(Configuration config, std::vector<Detector*> detectors);
        ~DUTAnalysis() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Histograms
        TH1F* tracksVersusTime;
        TH1F* associatedTracksVersusTime;
        TH1F* residualsX;
        TH1F* residualsY;
        TH1F* clusterTotAssociated;
        TH1F* clusterSizeAssociated;
        TH1F* hTrackCorrelationX;
        TH1F* hTrackCorrelationY;
        TH1F* hTrackCorrelationTime;
        TH1F* residualsTime;
        TH2F* clusterToTVersusTime;
        TH2F* residualsTimeVsTime;

      	TH1F* residualsXMCtruth;

        TH2F* hAssociatedTracksGlobalPosition;
        TH2F* hUnassociatedTracksGlobalPosition;

        TH1F* tracksVersusPowerOnTime;
        TH1F* associatedTracksVersusPowerOnTime;

        // Member variables
        std::string m_DUT;
        int m_eventNumber;
        int m_nAlignmentClusters;
        bool m_useMCtruth;
        long long int m_powerOnTime;
        long long int m_powerOffTime;
        long long int m_shutterOpenTime;
        long long int m_shutterCloseTime;
        bool m_digitalPowerPulsing;
    };
}
#endif // DUTAnalysis_H
