#ifndef TelescopeAnalysis_H
#define TelescopeAnalysis_H 1

#include <iostream>
#include "TH1F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/MCParticle.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class TelescopeAnalysis : public Algorithm {

    public:
        // Constructors and destructors
        TelescopeAnalysis(Configuration config, std::vector<Detector*> detectors);
        ~TelescopeAnalysis() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise(){};

    private:
        ROOT::Math::XYZPoint closestApproach(ROOT::Math::XYZPoint position, MCParticles* particles);

        // Histograms for each of the devices
        std::map<std::string, TH1F*> telescopeMCresidualsLocalX;
        std::map<std::string, TH1F*> telescopeMCresidualsLocalY;
        std::map<std::string, TH1F*> telescopeMCresidualsX;
        std::map<std::string, TH1F*> telescopeMCresidualsY;

        std::map<std::string, TH1F*> telescopeResidualsLocalX;
        std::map<std::string, TH1F*> telescopeResidualsLocalY;
        std::map<std::string, TH1F*> telescopeResidualsX;
        std::map<std::string, TH1F*> telescopeResidualsY;

        // Histograms at the position of the DUT
        TH1F* telescopeResolution;

        // Parameters
        double chi2ndofCut;
    };
} // namespace corryvreckan
#endif // TelescopeAnalysis_H
