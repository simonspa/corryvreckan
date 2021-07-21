/**
 * @file
 * @brief Definition of module EventDefinitionM26
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>

#include "core/module/Module.hpp"
#include "eudaq/FileReader.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class EventDefinitionM26 : public Module {
        class EndOfFile : public Exception {};

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        EventDefinitionM26(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>&) override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        std::vector<long double> _starts{};
        std::vector<long double> _ends{};
        std::vector<long double> _pivots{};
        std::vector<long double> _triggers{};
        int skipped_events_{};
        long double _pivotCurrent;
        std::vector<uint32_t> triggerIDs_{};
        bool pixelated_timing_layer_{};
        bool use_all_mimosa_hits_{};
        bool add_trigger_{};
        long double timeshift_{};
        int shift_triggers_{};
        double skip_time_{};
        double add_begin_{};
        double add_end_{};
        int pivot_min_{};
        int pivot_max_{};
        // EUDAQ2 readers for all required files
        eudaq::FileReaderUP readerTime_;
        eudaq::FileReaderUP readerDuration_;
        // Detector defining the event time
        // Note: detector defining duration of event is always "MIMOSA26"
        std::string detector_time_;
        // input data files
        std::string timestamp_, duration_;

        std::pair<long double, long double> _oldEvent{};
        // HIER WEITER MIT DEN PLOTS, FUELLEN UND RANGES DEFINIEREN
        TH2F* _pivot_vs_next_dtrigger;
        TH2F* _pivot_vs_priv_dtrigger;
        TH2F* _pivot_vs_next_event;
        TH2F* _pivot_vs_priv_event;

        TH1F* timebetweenMimosaEvents_;
        TH1F* timebetweenTLUEvents_;
        TH1F* eventDuration_;
        TH1F* timeBeforeTrigger_;
        TH1F* timeAfterTrigger_;
        TH1F* pivotPixel_;
        TH1D* hClipboardEventStart;
        TH1D* hClipboardEventStart_long;

        const static double framelength_;
        unsigned triggerTLU_{}, triggerM26_{};
        long double time_prev_{}, trig_prev_{}, time_trig_start_{}, time_trig_stop_{}, time_before_{}, time_after_{};
        long double time_trig_stop_prev_{};

        /**
         * @brief get_next_event_with_det
         * @param filereader: eudaq::FileReader
         * @param det: detector name to search for in data
         * @param begin: timestamp of begin of event
         * @param end: timestamp of end of event
         * @return
         */
        unsigned get_next_event_with_det(const eudaq::FileReaderUP& filereader,
                                         const std::string& det,
                                         long double& begin,
                                         long double& end);

        // EUDAQ configuration to be passed to the decoder instance
        eudaq::ConfigurationSPC eudaq_config_;
    };

} // namespace corryvreckan
