/**
 * @file
 * @brief Definition of module Metronome
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef Metronome_H
#define Metronome_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Metronome : public Module {

    public:
        // Constructors and destructors
        Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Metronome() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        double m_eventStart, m_eventEnd, m_eventLength;
        uint32_t m_triggersPerEvent, m_triggers;
    };
} // namespace corryvreckan
#endif // Metronome_H
