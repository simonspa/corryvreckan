/**
 * @file
 * @brief Definition of module AnalysisParticleFlux
 *
 * @copyright Copyright (c) 2021 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef AnalysisParticleFlux_H
#define AnalysisParticleFlux_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <cmath>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class AnalysisParticleFlux : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        AnalysisParticleFlux(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

        /**
         * @brief [Finalise module]
         */
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        int m_eventNumber;
        int m_numberOfTracks;
        double m_trackIntercept;
        double m_angleConversion;
        std::string m_angleUnit;

        // Azimuthal histogram
        int m_azimuthGranularity;
        double m_azimuthLow;
        double m_azimuthHigh;
        TH1D* m_azimuthHistogram;

        // Zenith histogram
        int m_zenithGranularity;
        double m_zenithLow;
        double m_zenithHigh;
        TH1D* m_zenithHistogram;

        // Combined histogram
        TH2D* m_combinedHistogram;

        // Flux histograms
        TH1D* m_azimuthFlux;
        TH1D* m_zenithFlux;
        TH2D* m_combinedFlux;

        void calculateAngles(Track* track);
        double solidAngle(double zenithLow, double zenithHigh, double azimuthLow, double azimuthHigh);
    };

} // namespace corryvreckan
#endif // AnalysisParticleFlux_H