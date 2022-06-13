/**
 * @file
 * @brief Definition of module EventLoaderFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH2Poly.h>
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
        bool loadEvent(PixelVector& deviceData,
                       std::map<std::string, std::string>& eventTags,
                       double spidr_timestamp,
                       bool discard = false);
        double getRawTimestamp();
        double getTimestamp();

        std::shared_ptr<Detector> m_detector;

        bool m_triggerSync;

        TH2Poly* hitmap;
        TH1F* seed_tot_inner;
        TH1F* seed_tot_outer;
        TH1F* pixels_per_event;
        TH1F* pixel_timestamps;
        TH1F *pixel_distance, *pixel_distance_min, *pixel_distance_max, *pixel_distance_row, *pixel_distance_col;
        TH1F* trigger_dt;
        TH1F *event_status, *missing_peaks;
        TH2F* missing_peaks_vs_size;

        int m_eventNumber;
        size_t m_triggerNumber;
        size_t m_missingTriggers;
        size_t m_discardedEvents;
        double m_timeScaler;

        size_t m_loadedEvents;
        size_t m_incompleteEvents;
        size_t m_noiseEvents;

        std::streampos m_prevEvent;

        double m_prevTriggerTime;
        double m_prevScopeTriggerTime;
        uint16_t m_blockSize;

        double m_spidr_t0;
        double m_scope_t0;

        std::ifstream m_inputFile;

        std::vector<double> m_scopeTriggerNumbers;
        std::vector<double> m_scopeTriggerTimestamps;
        std::vector<double> m_spidrTriggerNumbers;
        std::vector<double> m_spidrTriggerTimestamps;

        std::vector<double> m_ratioTriggerNumbers;
        std::vector<double> m_dtRatio;
    };

} // namespace corryvreckan
