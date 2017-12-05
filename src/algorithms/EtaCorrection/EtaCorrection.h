#ifndef EtaCorrection_H
#define EtaCorrection_H 1

#include <iostream>
#include "TCanvas.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"

#include "core/Detector.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class EtaCorrection : public Algorithm {

    public:
        // Constructors and destructors
        EtaCorrection(Configuration config, std::vector<Detector*> detectors);
        ~EtaCorrection() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        std::string m_DUT;
        int m_eventNumber;
        double m_chi2ndofCut;
        Detector* m_detector;
        std::string m_etaFormulaX;
        std::vector<double> m_etaConstantsX;
        TF1* m_etaCorrectorX;
        std::string m_etaFormulaY;
        std::vector<double> m_etaConstantsY;
        TF1* m_etaCorrectorY;

        // Histograms
        TH2F* m_etaDistributionX;
        TH2F* m_etaDistributionY;
        TProfile* m_etaDistributionXprofile;
        TProfile* m_etaDistributionYprofile;
        TH2F* m_etaDistributionXcorrected;
        TH2F* m_etaDistributionYcorrected;
    };
} // namespace corryvreckan
#endif // EtaCorrection_H
