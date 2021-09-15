/**
 * @file
 * @brief Implementation of module EventFilter
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventFilter.h"

using namespace corryvreckan;

EventFilter::EventFilter(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

void EventFilter::initialize() {

    config_.setDefault<unsigned>("minTracks", 0);
    config_.setDefault<unsigned>("maxTracks", 100);
    config_.setDefault<unsigned>("minHits_per_plane", 0);
    config_.setDefault<unsigned>("maxHits_per_plane", 100);

    minNumberTracks_ = config_.get<unsigned>("minTracks");
    maxNumberTracks_ = config_.get<unsigned>("maxTracks");
    minHitsPerReference_ = config_.get<unsigned>("minHits_per_plane");
    maxHitsPerReference_ = config_.get<unsigned>("maxHits_per_plane");
}

StatusCode EventFilter::run(const std::shared_ptr<Clipboard>& clipboard) {

    eventsTotal_++;
    unsigned numTracks = clipboard->getData<Track>().size();
    if(numTracks > maxNumberTracks_) {
        eventsSkipped_++;
        LOG(TRACE) << "Number of tracks above maximum";
        return StatusCode::DeadTime;
    } else if(numTracks < minNumberTracks_) {
        eventsSkipped_++;
        LOG(TRACE) << "Number of tracks below minimum";
        return StatusCode::DeadTime;
    }

    // Loop over all reference detectors
    for(auto& detector : get_detectors()) {
        // skip duts and auxilliary
        if(detector->isAuxiliary() || detector->isDUT()) {
            continue;
        }
        // else check if desired number of hits
        std::string det = detector->getName();
        unsigned numClusters = clipboard->getData<Cluster>(det).size();
        if(numClusters > maxHitsPerReference_) {
            eventsSkipped_++;
            LOG(TRACE) << "Number of hits on above maximum";
            return StatusCode::DeadTime;
        }
        if(numClusters < minHitsPerReference_) {
            eventsSkipped_++;
            LOG(TRACE) << "Number of hits on below minimum";
            return StatusCode::DeadTime;
        }
    }
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventFilter::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(STATUS) << "Skipped " << eventsSkipped_ << " events of " << eventsTotal_;
}
