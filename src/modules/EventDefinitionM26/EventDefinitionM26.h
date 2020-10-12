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

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        std::vector<uint32_t> triggerIDs_{};
        long double timeshift_{};
        int shift_triggers_{};
        double response_time_m26_{};

        // EUDAQ2 readers for all required files
        eudaq::FileReaderUP readerTime_;
        eudaq::FileReaderUP readerDuration_;
        // Detector defining the event time
        std::string detector_time_;
        // Detector defining the event duration
        std::string detector_duration_;
        // input data files
        std::string timestamp_, duration_;

        TH1F* timebetweenMimosaEvents_;
        TH1F* timebetweenTLUEvents_;
        TH1D* hClipboardEventStart;
        TH1D* hClipboardEventStart_long;

        unsigned triggerTLU_{999}, triggerM26_{999}; // not nice, find better solution here!!!
        long double time_prev_{}, trig_prev_{}, time_trig_start_{}, time_trig_stop_{}, time_before_{}, time_after_{};
        long double time_trig_stop_prev_{};

        /**
         * @brief get_next_event_with_det
         * @param filereader: eudaq::FileReader
         * @param det: detetcor name to search for in data
         * @param begin: timestamp of begin of event
         * @param end: timestamp of end of event
         * @return
         */
        unsigned
        get_next_event_with_det(eudaq::FileReaderUP& filereader, std::string& det, long double& begin, long double& end);

        // EUDAQ configuration to be passed to the decoder instance
        eudaq::ConfigurationSPC eudaq_config_;
    };

} // namespace corryvreckan
