#ifndef EtaCalculation_H
#define EtaCalculation_H 1

#include <iostream>
#include "TCanvas.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"

#include "core/Detector.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EtaCalculation : public Module {

    public:
        // Constructors and destructors
        EtaCalculation(Configuration config, std::vector<Detector*> detectors);
        ~EtaCalculation() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        ROOT::Math::XYVector pixelIntercept(Track* tr, Detector* det);
        void calculateEta(Track* track, Cluster* cluster);
        std::string fit(TF1* function, std::string fname, TProfile* profile);

        // Member variables
        double m_chi2ndofCut;
        std::string m_etaFormulaX;
        std::map<std::string, TF1*> m_etaFitX;
        std::string m_etaFormulaY;
        std::map<std::string, TF1*> m_etaFitY;

        // Histograms
        std::map<std::string, TH2F*> m_etaDistributionX;
        std::map<std::string, TH2F*> m_etaDistributionY;
        std::map<std::string, TProfile*> m_etaDistributionXprofile;
        std::map<std::string, TProfile*> m_etaDistributionYprofile;
    };
} // namespace corryvreckan
#endif // EtaCalculation_H
