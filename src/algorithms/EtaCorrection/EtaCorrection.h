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
    /** @ingroup Algorithms
     */
    class EtaCorrection : public Algorithm {

    public:
        // Constructors and destructors
        EtaCorrection(Configuration config, std::vector<Detector*> detectors);
        ~EtaCorrection() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise(){};

    private:
        void applyEta(Cluster* cluster, Detector* detector);

        // Member variables
        int m_eventNumber;
        double m_chi2ndofCut;
        std::string m_etaFormulaX;
        std::map<std::string, TF1*> m_etaCorrectorX;
        std::map<std::string, bool> m_correctX;
        std::string m_etaFormulaY;
        std::map<std::string, TF1*> m_etaCorrectorY;
        std::map<std::string, bool> m_correctY;
    };
} // namespace corryvreckan
#endif // EtaCorrection_H
