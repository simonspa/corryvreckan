/**
 * @file
 * @brief Definition of [EventLoaderEUDAQ2] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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
         * @brief get event start/end: if event exists on clipboard --> take this, otherwise take input values and put new
         * event onto clipboard
         * @param start Start time of the currently read detector data
         * @param end End time of the currently read detector data
         * @param clipboard The clipboard of the event
         * @return pair with event start and end time, either from definded event on clipboard or from local data
         */
        std::pair<double, double> get_event_times(double start, double end, std::shared_ptr<Clipboard>& clipboard);

        /**
         * @brief Process events: extract hits and put onto clipboard, also define event start/end
         * @param evt: event or subevent to be processed, clipboard
         */
        void process_event(eudaq::EventSPC evt, std::shared_ptr<Clipboard>& clipboard);

        std::shared_ptr<Detector> m_detector;
        std::string m_filename{};
        bool m_skipBeforeT0;

        eudaq::FileReaderUP reader;

        // Pixel histograms
        TH2F* hitmap;
        TH1F* hHitTimes;

        TH1F* hPixelsPerFrame;
        TH1D* hEventBegin;
    };

} // namespace corryvreckan
