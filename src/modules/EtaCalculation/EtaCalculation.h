/**
 * @file
 * @brief Definition of module EtaCalculation
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EtaCalculation_H
#define EtaCalculation_H 1

#include <TCanvas.h>
#include <TF1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <iostream>

#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EtaCalculation : public Module {

    public:
        // Constructors and destructors
        EtaCalculation(Configuration config, std::shared_ptr<Detector> detector);
        ~EtaCalculation() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

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
