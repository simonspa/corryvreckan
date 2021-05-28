/**
 * @file
 * @brief Implementation of module AnalysisTracks
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisTracks.h"

using namespace corryvreckan;

AnalysisTracks::AnalysisTracks(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

void AnalysisTracks::initialize() {

    std::string title;
    for(auto& detector : get_detectors()) {
        if(detector->isAuxiliary())
            continue;
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(detector->getName().c_str());

        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();
        title = "Disance between tracks ; distance [mm]; entries";
        _distance_between_tracks_[detector->getName().c_str()] = new TH1F("distance_tracks", title.c_str(), 1000, 0, 10);
        title = "Tracks with same hits ; # tracks with same hit; entries";
        _tracks_per_hit_[detector->getName().c_str()] = new TH1F("number_tracks_with same hit", title.c_str(), 15, 0, 15);
        title = "Cluster vs tracks ; # tracks; #clusters";
        clusters_vs_tracks_[detector->getName().c_str()] =
            new TH2F("distance_tracks", title.c_str(), 25, 0, 25, 200, 0, 200);
    }

    // Initialise member variables
    eventNumber_ = 0;
}

StatusCode AnalysisTracks::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all detectors
    for(auto& detector : get_detectors()) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }

    // Increment event counter
    eventNumber_++;

    auto tracks = clipboard->getData<Track>();
    if(!tracks.size())
        return StatusCode::Success;
    std::map<std::string, uint> clusters;
    for(auto d : get_detectors()) {
        if(d->isAuxiliary())
            continue;
        clusters[d->getName()] = clipboard->getData<Cluster>(d->getName()).size();
        clusters_vs_tracks_.at(d->getName())->Fill(tracks.size(), clusters.at(d->getName()));
    }
    // Loop over all tracks and get clusters assigned to tracks as well as the intersections
    std::map<std::string, std::map<std::pair<double, double>, int>> track_clusters;
    std::map<std::string, std::vector<XYZPoint>> intersects; // local coordinates
    for(auto& track : tracks) {
        for(auto d : get_detectors()) {
            if(d->isAuxiliary())
                continue;
            intersects[d->getName()].push_back(d->globalToLocal(track->getState(d->getName())));
            if(d->isDUT() || track->getClusterFromDetector(d->getName()) == nullptr)
                continue;
            auto c = track->getClusterFromDetector(d->getName());
            track_clusters[d->getName()][std::make_pair<double, double>(c->column(), c->row())] += 1;
        }
    }
    // Now fill the histos
    for(auto const& [key, val] : intersects) {
        for(uint i = 0; i < val.size(); ++i) {
            auto j = i + 1;
            while(j < val.size()) {
                auto p = val.at(i) - val.at(j);
                _distance_between_tracks_.at(key)->Fill((p.Mag2()));
                j++;
            }
        }
    }
    for(auto const& [key, val] : track_clusters) {
        for(auto const& [k, v] : val) {
            _tracks_per_hit_.at(key)->Fill(v);
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisTracks::finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) {

    LOG(DEBUG) << "Analysed " << eventNumber_ << " events";
}
