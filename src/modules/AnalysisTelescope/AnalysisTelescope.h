/**
 * @file
 * @brief Definition of module AnalysisTelescope
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef AnalysisTelescope_H
#define AnalysisTelescope_H 1

#include <TH1F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/MCParticle.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class AnalysisTelescope : public Module {

    public:
        // Constructors and destructors
        AnalysisTelescope(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        ROOT::Math::XYZPoint closestApproach(ROOT::Math::XYZPoint position, const MCParticleVector& particles);

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
        std::map<std::string, TH1F*> telescopeResolutionX;
        std::map<std::string, TH1F*> telescopeResolutionY;

        // Parameters
        double chi2ndofCut;
    };
} // namespace corryvreckan
#endif // AnalysisTelescope_H
