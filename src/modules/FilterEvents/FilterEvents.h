/**
 * @file
 * @brief Definition of module FILTEREVENTS
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */
#ifndef FILTEREVENTS_H
#define FILTEREVENTS_H
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
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
    class FilterEvents : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        FilterEvents(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);

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
        // Histogram with filters applied
        TH1F* hFilter_;

        long unsigned max_number_tracks_{};
        long unsigned min_number_tracks_{};
        long unsigned min_clusters_per_reference_{};
        long unsigned max_clusters_per_reference_{};
        bool only_tracks_on_dut_{};
        std::map<std::string, std::function<bool(const std::string&)>> tag_filter_funcs_{};

        /**
         * @brief Function to filter events based on the number of tracks
         * @param clipboard with the current event
         * @return true if number of tracks is within sopecified range, false otherwise
         */
        bool filter_tracks(const std::shared_ptr<Clipboard>& clipboard);

        /**
         * @brief Function to filter events based on the number of clusters on each reerence plane
         * @param clipboard with the current event
         * @return true if number of clusters on one plane is within sopecified range, false otherwise
         */
        bool filter_cluster(const std::shared_ptr<Clipboard>& clipboard);

        /**
         * @brief Function to load decoded tag filters
         * @param tag_filter_map map of tag filters as strings
         */
        void load_tag_filters(const std::map<std::string, std::string>& tag_filter_map);

        /**
         * @brief Function to filter events based on tag requirements
         * @param clipboard with the current event
         * @return true if tags in configuration fulfill the specified requirements, false otherwise
         */
        bool filter_tags(const std::shared_ptr<Clipboard>& clipboard);
    };

} // namespace corryvreckan
#endif // FILTEREVENTS.H
