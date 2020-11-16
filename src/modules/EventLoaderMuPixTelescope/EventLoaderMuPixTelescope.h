/**
 * @file
 * @brief Definition of module EventLoaderMuPixTelescope
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

//#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include <queue>
#include "core/module/Module.hpp"

// mupix telescope includes
#include "blockfile.hpp"
#include "telescope_frame.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class EventLoaderMuPixTelescope : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        EventLoaderMuPixTelescope(Configuration& config, std::shared_ptr<Detector> detector);

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
        StatusCode read_sorted(const std::shared_ptr<Clipboard>& clipboard);
        StatusCode read_unsorted(const std::shared_ptr<Clipboard>& clipboard);
        void fillBuffer();
        uint tag_{};
        double prev_event_end_{};
        int type_{};
        int eventNo_{};
        long unsigned counterHits_{};
        long unsigned removed_{}, stored_{};
        uint64_t ts_prev_{0};
        unsigned buffer_depth_{};
        bool eof_{false};
        double timeOffset_{};
        std::string input_file_{};
        std::shared_ptr<Detector> detector_;
        struct CompareTimeGreater {
            bool operator()(const std::shared_ptr<Pixel>& a, const std::shared_ptr<Pixel>& b) {
                return a->timestamp() > b->timestamp();
            }
        };
        // Buffer of timesorted pixel hits: (need to use greater here!)
        std::priority_queue<std::shared_ptr<Pixel>, PixelVector, CompareTimeGreater> pixelbuffer_;
        PixelVector pixels_{};
        std::string inputDirectory_;
        bool isSorted_;
        bool ts2IsGray_;
        int runNumber_;
        BlockFile* blockFile_;
        TelescopeFrame tf_;

        // Histograms
        TH1F* hPixelToT;
        TH1F* hTimeStamp;
        TH1F* hHitsEvent;
        TH1F* hitsPerkEvent;
        TH2F* hdiscardedHitmap;
        TH2F* hHitMap;
        static std::map<std::string, int> typeString_to_typeID;
    };

} // namespace corryvreckan
