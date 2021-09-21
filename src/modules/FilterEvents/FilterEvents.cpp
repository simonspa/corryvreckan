/**
 * @file
 * @brief Implementation of module EventFilter
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "FilterEvents.h"

using namespace corryvreckan;

FilterEvents::FilterEvents(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

void FilterEvents::initialize() {

    config_.setDefault<unsigned>("min_tracks", 0);
    config_.setDefault<unsigned>("max_tracks", 100);
    config_.setDefault<unsigned>("min_clusters_per_plane", 0);
    config_.setDefault<unsigned>("maxC_custers_per_plane", 100);

    min_number_tracks_ = config_.get<unsigned>("min_tracks");
    max_number_tracks_ = config_.get<unsigned>("max_tacks");
    min_clusters_per_reference_ = config_.get<unsigned>("min_clusters_per_plane");
    max_clusters_per_reference_ = config_.get<unsigned>("max_clusters_per_plane");

    hFilter_ = new TH1F("FilteredEvents", "Events filtered;events", 6, 0.5, 6.5);
    hFilter_->GetXaxis()->SetBinLabel(1, "Events");
    hFilter_->GetXaxis()->SetBinLabel(2, "Too few tracks");
    hFilter_->GetXaxis()->SetBinLabel(3, "Too many tracks");
    hFilter_->GetXaxis()->SetBinLabel(4, "Too few clusters");
    hFilter_->GetXaxis()->SetBinLabel(5, "Too many clusters");
    hFilter_->GetXaxis()->SetBinLabel(6, "Events passed ");
}

StatusCode FilterEvents::run(const std::shared_ptr<Clipboard>& clipboard) {

    hFilter_->Fill(1); // number of events
    auto status = filter_tracks(clipboard) ? StatusCode::DeadTime : StatusCode::Success;
    status = filter_cluster(clipboard) ? StatusCode::DeadTime : status;

    if(status == StatusCode::Success) {
        hFilter_->Fill(6);
    }
    return status;
}

void FilterEvents::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(STATUS) << "Skipped " << hFilter_->GetBinContent(1) << " events of " << hFilter_->GetBinContent(6);
}

bool FilterEvents::filter_tracks(const std::shared_ptr<Clipboard>& clipboard) {
    auto num_tracks = clipboard->getData<Track>().size();
    if(num_tracks > max_number_tracks_) {
        hFilter_->Fill(2); // too many tracks
        LOG(TRACE) << "Number of tracks above maximum";
        return true;
    } else if(num_tracks < min_number_tracks_) {
        hFilter_->Fill(3); //  too few tracks
        LOG(TRACE) << "Number of tracks below minimum";
        return true;
    }
    return false;
}

bool FilterEvents::filter_cluster(const std::shared_ptr<Clipboard>& clipboard) {
    // Loop over all reference detectors
    for(auto& detector : get_detectors()) {
        // skip duts and auxiliary
        if(detector->isAuxiliary() || detector->isDUT()) {
            continue;
        }
        std::string det = detector->getName();
        // Check if number of Clusters on plane is within acceptance
        auto num_clusters = clipboard->getData<Cluster>(det).size();
        if(num_clusters > max_clusters_per_reference_) {
            hFilter_->Fill(4); //  too many clusters
            LOG(TRACE) << "Number of Clusters on above maximum";
            return true;
        }
        if(num_clusters < min_clusters_per_reference_) {
            hFilter_->Fill(5); //  too few clusters
            LOG(TRACE) << "Number of Clusters on below minimum";
            return true;
        }
    }
    return false;
}
