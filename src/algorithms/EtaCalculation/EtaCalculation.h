#ifndef EtaCalculation_H
#define EtaCalculation_H 1

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
    /** @ingroup Algorithms
     */
    class EtaCalculation : public Algorithm {

    public:
        // Constructors and destructors
        EtaCalculation(Configuration config, std::vector<Detector*> detectors);
        ~EtaCalculation() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise(){};

    private:
        ROOT::Math::XYVector pixelIntercept(Track* tr, Detector* det);
        void calculateEta(Track* track, Cluster* cluster);

        // Member variables
        double m_chi2ndofCut;
        std::string m_etaFormulaX;
        std::map<std::string, TF1*> m_etaCorrectorX;
        std::map<std::string, bool> m_correctX;
        std::string m_etaFormulaY;
        std::map<std::string, TF1*> m_etaCorrectorY;
        std::map<std::string, bool> m_correctY;

        // Histograms
        std::map<std::string, TH2F*> m_etaDistributionX;
        std::map<std::string, TH2F*> m_etaDistributionY;
        std::map<std::string, TProfile*> m_etaDistributionXprofile;
        std::map<std::string, TProfile*> m_etaDistributionYprofile;
    };
} // namespace corryvreckan
#endif // EtaCalculation_H
