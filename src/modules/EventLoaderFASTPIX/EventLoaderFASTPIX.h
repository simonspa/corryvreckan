/**
 * @file
 * @brief Definition of module EventLoaderFASTPIX
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH2Poly.h>
#include <TGraph.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class EventLoaderFASTPIX : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        EventLoaderFASTPIX(Configuration& config, std::shared_ptr<Detector> detector);

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
        bool loadData(const std::shared_ptr<Clipboard>& clipboard, PixelVector &deviceData, double spidr_timestamp);

        std::shared_ptr<Detector> m_detector;

        TH2Poly* hitmap;
        TH1F* test;
        TH1F* trigger_delta;

        int m_eventNumber;
        size_t m_triggerNumber;
        size_t m_lostTriggers;
        double m_prevTriggerTime;
        double m_prevScopeTriggerTime;
        uint16_t m_blockSize;

        double m_spidr_t0;
        double m_scope_t0;

        size_t m_filled;

        //std::string m_inputFile;
        std::ifstream m_inputFile;
        std::ofstream m_debugFile;

        std::vector<double> m_triggerNumbers;
        std::vector<double> m_triggerTimestamps;
        std::vector<double> m_triggerScopeTimestamps;
        std::vector<double> m_triggerScopeTimestampsCorrected;

        TGraph* trigger_graph;
        TGraph* trigger_scope_graph;
        TGraph* trigger_dt_ratio;
    };

} // namespace corryvreckan
