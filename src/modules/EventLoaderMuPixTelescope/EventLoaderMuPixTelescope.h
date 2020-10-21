/**
 * @file
 * @brief Definition of module EventLoaderMuPixTelescope
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include <queue>
#include "core/module/Module.hpp"

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
        int typeString_to_typeID(string typeString);
        void fillBuffer();
        uint m_tag{};
        int m_type{};
        int m_removed{};
        int m_buffer_depth{};
        bool m_eof{false};
        std::string m_input_file{};
        std::shared_ptr<Detector> m_detector;
        struct CompareTimeGreater {
            bool operator()(const std::shared_ptr<Pixel> a, const std::shared_ptr<Pixel> b) {
                return a->timestamp() > b->timestamp();
            }
        };
        // Buffer of timesorted pixel hits: (need to use greater here!)
        std::priority_queue<std::shared_ptr<Pixel>, PixelVector, CompareTimeGreater> m_pixelbuffer;
        std::string m_inputDirectory;
        bool m_isSorted;
        bool m_ts2IsGray;
        int m_runNumber;
        BlockFile* m_blockFile;
        TelescopeFrame m_tf;

        // Histograms
        TH2F* hHitMap;
        TH1F* hPixelToT;
        TH1F* hTimeStamp;
    };

} // namespace corryvreckan
