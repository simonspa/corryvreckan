/**
 * @file
 * @brief Definition of module EventLoaderTimestamp
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
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
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/SpidrSignal.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class EventLoaderTimestamp : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        EventLoaderTimestamp(Configuration& config, std::shared_ptr<Detector> detector);

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

        bool decodeNextWord();
        void fillBuffer();

    private:
        std::shared_ptr<Detector> m_detector;

        // configuration parameters:
        std::string m_inputDirectory;

        // Member variables
        std::vector<std::unique_ptr<std::ifstream>> m_files;
        std::vector<std::unique_ptr<std::ifstream>>::iterator m_file_iterator;

        bool eof_reached;
        size_t m_buffer_depth;
        unsigned long long int m_syncTime;
        bool m_clearedHeader;
        long long int m_syncTimeTDC;
        int m_TDCoverflowCounter;
        double m_eventLength;

        int m_prevTriggerNumber;
        int m_triggerOverflowCounter;

        double m_time_offset;

        template <typename T> struct CompareTimeGreater {
            bool operator()(const std::shared_ptr<T> a, const std::shared_ptr<T> b) {
                return a->timestamp() > b->timestamp();
            }
        };

        std::priority_queue<std::shared_ptr<SpidrSignal>, SpidrSignalVector, CompareTimeGreater<SpidrSignal>>
            sorted_signals_;
    };

} // namespace corryvreckan
