#ifndef EtaCorrection_H
#define EtaCorrection_H 1

#include <iostream>
#include "TCanvas.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"

#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EtaCorrection : public Module {

    public:
        // Constructors and destructors
        EtaCorrection(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EtaCorrection() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise(){};

    private:
        void applyEta(Cluster* cluster, std::shared_ptr<Detector> detector);

        // Member variables
        std::string m_etaFormulaX;
        std::map<std::string, TF1*> m_etaCorrectorX;
        std::map<std::string, bool> m_correctX;
        std::string m_etaFormulaY;
        std::map<std::string, TF1*> m_etaCorrectorY;
        std::map<std::string, bool> m_correctY;
    };
} // namespace corryvreckan
#endif // EtaCorrection_H
