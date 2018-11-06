#ifndef EtaCalculation_H
#define EtaCalculation_H 1

#include <TCanvas.h>
#include <TF1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <iostream>

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
        EtaCalculation(Configuration config, std::shared_ptr<Detector> detector);
        ~EtaCalculation() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        ROOT::Math::XYVector pixelIntercept(Track* tr);
        void calculateEta(Track* track, Cluster* cluster);
        std::string fit(TF1* function, std::string fname, TProfile* profile);

        std::shared_ptr<Detector> m_detector;
        double m_chi2ndofCut;
        std::string m_etaFormulaX;
        TF1* m_etaFitX;
        std::string m_etaFormulaY;
        TF1* m_etaFitY;

        // Histograms
        TH2F* m_etaDistributionX;
        TH2F* m_etaDistributionY;
        TProfile* m_etaDistributionXprofile;
        TProfile* m_etaDistributionYprofile;
    };
} // namespace corryvreckan
#endif // EtaCalculation_H
