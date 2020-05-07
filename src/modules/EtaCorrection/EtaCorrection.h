/**
 * @file
 * @brief Definition of module EtaCorrection
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EtaCorrection_H
#define EtaCorrection_H 1

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
    class EtaCorrection : public Module {

    public:
        // Constructors and destructors
        EtaCorrection(Configuration config, std::shared_ptr<Detector> detector);
        ~EtaCorrection() {}

        // Functions
        void initialize() override;
        StatusCode run(std::shared_ptr<Clipboard> clipboard) override;

    private:
        void applyEta(Cluster* cluster);

        std::shared_ptr<Detector> m_detector;
        std::string m_etaFormulaX;
        TF1* m_etaCorrectorX;
        bool m_correctX;
        std::string m_etaFormulaY;
        TF1* m_etaCorrectorY;
        bool m_correctY;
    };
} // namespace corryvreckan
#endif // EtaCorrection_H
