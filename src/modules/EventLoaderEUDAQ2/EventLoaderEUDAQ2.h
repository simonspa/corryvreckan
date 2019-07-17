/**
 * @file
 * @brief Definition of [EventLoaderEUDAQ2] module
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
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
    class EventLoaderEUDAQ2 : public Module {

        enum class EventPosition {
            UNKNOWN, // StandardEvent position unknown
            BEFORE,  // StandardEvent is before current event
            DURING,  // StandardEvent is during current event
            AFTER,   // StandardEvent is after current event
        };

        class EndOfFile : public Exception {};

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        EventLoaderEUDAQ2(Configuration config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialise();

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        /**
         * @brief Read and return the next event (smallest possible granularity) and return the decoded StandardEvent
         */
        std::shared_ptr<eudaq::StandardEvent> get_next_event();

        /**
         * @brief Check whether the current EUDAQ StandardEvent is within the defined Corryvreckan event
         * @param  clipboard  Shared pointer to the event clipboard
         * @param  evt        The EUDAQ StandardEvent to check
         * @return            Position of the StandardEvent with respect to the current Corryvreckan event
         */
        EventPosition is_within_event(std::shared_ptr<Clipboard> clipboard, std::shared_ptr<eudaq::StandardEvent> evt);

        /**
         * @brief Store pixel data from relevant detectors on the clipboard
         * @param evt       StandardEvent to read the pixel data from
         * @return Vector of pointers to pixels read from this event
         */
        Pixels* get_pixel_data(std::shared_ptr<eudaq::StandardEvent> evt);

        std::shared_ptr<Detector> m_detector;
        std::string m_filename{};
        double m_skip_time{};
        Matrix<std::string> adjust_event_times;

        // EUDAQ file reader instance to retrieve data from
        eudaq::FileReaderUP reader_;
        // Buffer of undecoded EUDAQ events
        std::vector<eudaq::EventSPC> events_;
        // Currently processed decoded EUDAQ StandardEvent:
        std::shared_ptr<eudaq::StandardEvent> event_;

        // EUDAQ configuration to be passed to the decoder instance
        eudaq::ConfigurationSPC eudaq_config_;

        // 2D histograms
        TH2F* hitmap;

        // 1D histograms
        TH1F* hHitTimes;
        TH1F* hHitTimes_long;
        TH1F* hPixelRawValues;
        TH1F* hPixelsPerEvent;
        TH1D* hEudaqEventStart;
        TH1D* hEudaqEventStart_long;
        TH1D* hClipboardEventStart;
        TH1D* hClipboardEventStart_long;
        TH1D* hClipboardEventEnd;
        TH1D* hClipboardEventDuration;

        TH1F* hPixelTimeEventBeginResidual;
        TH1F* hPixelTimeEventBeginResidual_wide;
        TH2F* hPixelTimeEventBeginResidualOverTime;

        std::map<size_t, TH1D*> hPixelTriggerTimeResidual;
        TH2D* hPixelTriggerTimeResidualOverTime;
        TH1D* hTriggersPerEvent;
    };

} // namespace corryvreckan
