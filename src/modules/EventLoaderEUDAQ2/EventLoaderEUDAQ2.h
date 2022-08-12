/**
 * @file
 * @brief Definition of module EventLoaderEUDAQ2
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <iostream>
#include <queue>
#include <vector>

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include <eudaq/FileReader.hh>
#include <eudaq/StandardEvent.hh>
#include <eudaq/StdEventConverter.hh>

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
    class EventLoaderEUDAQ2 : public Module {

        class EndOfFile : public Exception {};

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        EventLoaderEUDAQ2(Configuration& config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

        /**
         * @brief [Finalise this module]
         */
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        /**
         * @brief Read and return the next decoded StandardEvent (smallest possible granularity) from timesorted buffer
         */
        std::shared_ptr<eudaq::StandardEvent> get_next_sorted_std_event();
        /**
         * @brief Read and return the next event (smallest possible granularity) and return the decoded StandardEvent
         */
        std::shared_ptr<eudaq::StandardEvent> get_next_std_event();

        /**
         * @brief Check whether the current EUDAQ StandardEvent is within the defined Corryvreckan event
         * @param  clipboard  Shared pointer to the event clipboard
         * @param  evt        The EUDAQ StandardEvent to check
         * @return            Position of the StandardEvent with respect to the current Corryvreckan event
         */
        Event::Position is_within_event(const std::shared_ptr<Clipboard>& clipboard,
                                        std::shared_ptr<eudaq::StandardEvent> evt) const;

        /**
         * @brief Helper function to retrieve event tags and creating plots from them
         * @param evt Shared pointer to the current event
         */
        void retrieve_event_tags(const eudaq::EventSPC evt);

        /**
         * @brief Store pixel data from relevant detectors on the clipboard
         * @param evt       StandardEvent to read the pixel data from
         * @param plane_id  ID of the EUDAQ2 StandardEvent plane to be read and stored
         * @return Vector of pointers to pixels read from this event
         */
        PixelVector get_pixel_data(std::shared_ptr<eudaq::StandardEvent> evt, int plane_id) const;

        /**
         * @brief Filter the incoming EUDAQ2 events for the correct detector and detector type
         * @param  evt The EUDAQ2 StdEvt to be scrutinized
         * @param plane_id If found within the event, the plane ID if the detector is stored in the reference
         * @return     Verdict whether this is the detector we are looking for or not
         */
        bool filter_detectors(std::shared_ptr<eudaq::StandardEvent> evt, int& plane_id) const;

        std::shared_ptr<Detector> detector_;
        std::string filename_{};
        bool get_time_residuals_{};
        bool get_tag_histograms_{};
        bool get_tag_profiles_{};
        bool ignore_bore_{};
        bool veto_triggers_{};
        bool inclusive_{};
        bool sync_by_trigger_{};
        double skip_time_{};
        Matrix<std::string> adjust_event_times_;
        int buffer_depth_;
        int shift_triggers_;

        size_t hits_ = 0;

        // EUDAQ file reader instance to retrieve data from
        eudaq::FileReaderUP reader_;

        // Buffer of undecoded EUDAQ events
        std::queue<eudaq::EventSPC> events_raw_;
        std::queue<eudaq::StandardEventSP> events_decoded_;

        // Currently processed decoded EUDAQ StandardEvent:
        std::shared_ptr<eudaq::StandardEvent> event_;

        // custom comparator for time-sorted priority_queue
        struct CompareTimeGreater {
            bool operator()(const std::shared_ptr<eudaq::StandardEvent> a, const std::shared_ptr<eudaq::StandardEvent> b) {
                return a->GetTimeBegin() > b->GetTimeBegin();
            }
        };
        // Buffer of timesorted decoded EUDAQ StandardEvents: (need to use greater here!)
        std::priority_queue<std::shared_ptr<eudaq::StandardEvent>,
                            std::vector<std::shared_ptr<eudaq::StandardEvent>>,
                            CompareTimeGreater>
            sorted_events_;

        // EUDAQ configuration to be passed to the decoder instance
        eudaq::ConfigurationSPC eudaq_config_;

        // 2D histograms
        TH2F* hitmap;

        // 2D profiles
        TProfile2D* hRawValuesMap;

        // 1D histograms
        TH1F* hPixelTimes;
        TH1F* hPixelTimes_long;
        TH1F* hPixelRawValues;
        TH1F* hPixelMultiplicityPerEudaqEvent;
        TH1F* hPixelMultiplicityPerCorryEvent;
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
        TH1D* hEudaqeventsPerCorry;
        TH2D* hHitsVersusEUDAQ2Frames;

        std::map<std::string, TH1D*> tagHist;
        std::map<std::string, TProfile*> tagProfile;
    };

} // namespace corryvreckan
