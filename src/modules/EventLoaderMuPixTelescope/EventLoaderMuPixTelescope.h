/**
 * @file
 * @brief Definition of module EventLoaderMuPixTelescope
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EVENTLOADERMUPIXTELESCOPE_H
#define EVENTLOADERMUPIXTELESCOPE_H 1
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
         * @param detectors Shared pointer to detectors of this module, detectors are assigned via name/tag pairs
         */
        EventLoaderMuPixTelescope(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EventLoaderMuPixTelescope() {}

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

        std::shared_ptr<Pixel> read_hit(const RawHit& h, uint tag, unsigned long corrected_fpgaTime);
        void fillBuffer();
        std::vector<uint> tags_{};
        double prev_event_end_{};
        std::map<uint, int> types_{};
        int eventNo_{};
        std::map<uint, long unsigned> counterHits_{};
        std::map<uint, long unsigned> removed_{}, stored_{};
        uint64_t ts_prev_{0};
        unsigned buffer_depth_{};
        bool eof_{false};
        std::map<uint, double> timeOffset_{};
        std::map<uint, std::string> names_{};
        std::string input_file_{};
        std::vector<std::shared_ptr<Detector>> detectors_;
        struct CompareTimeGreater {
            bool operator()(const std::shared_ptr<Pixel>& a, const std::shared_ptr<Pixel>& b) {
                return a->timestamp() > b->timestamp();
            }
        };
        // Buffer of timesorted pixel hits: (need to use greater here!)
        std::map<uint, std::priority_queue<std::shared_ptr<Pixel>, PixelVector, CompareTimeGreater>> pixelbuffers_;
        std::map<uint, PixelVector> pixels_{};
        std::string inputDirectory_;
        bool isSorted_;
        int runNumber_;
        double refFrequency_;
        bool use_both_timestamps_;
        uint nbitsTS_;
        uint nbitsToT_;
        uint timestampMask_;
        uint timestampMaskExtended_;
        uint totMask_;
        uint ckdivend_;
        uint ckdivend2_;
        double multiplierToT_;
        double maxToT_;
        double clockToTime_;
        BlockFile* blockFile_;
        TelescopeFrame tf_;

        // Histograms
        std::map<std::string, TH1F*> hPixelToT;
        std::map<std::string, TH1F*> hts_ToT;
        std::map<std::string, TH2F*> ts_TS1_ToT;
        std::map<std::string, TH1F*> hTimeStamp;
        std::map<std::string, TH1F*> hHitsEvent;
        std::map<std::string, TH1F*> hitsPerkEvent;
        std::map<std::string, TH2F*> hdiscardedHitmap;
        std::map<std::string, TH2F*> hHitMap;
        std::map<std::string, TH2F*> raw_fpga_vs_chip;
        std::map<std::string, TH2F*> raw_fpga_vs_chip_corrected;
        std::map<std::string, TH1F*> chip_delay;
        std::map<std::string, TH2F*> ts1_ts2;

        static std::map<std::string, int> typeString_to_typeID;
    };

} // namespace corryvreckan
#endif // EVENTLOADERMUPIXTELESCOPE_H
