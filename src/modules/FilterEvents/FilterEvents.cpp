/**
 * @file
 * @brief Implementation of module FilterEvents
 *
 * @copyright Copyright (c) 2022 CERN and the Corryvreckan authors.
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
    config_.setDefault<bool>("only_tracks_on_dut", false);
    config_.setDefault<unsigned>("min_clusters_per_plane", 0);
    config_.setDefault<unsigned>("max_clusters_per_plane", 100);
    config_.setDefaultMap<std::string, std::string>("filter_tags", std::map<std::string, std::string>{});

    min_number_tracks_ = config_.get<unsigned>("min_tracks");
    max_number_tracks_ = config_.get<unsigned>("max_tracks");
    min_clusters_per_reference_ = config_.get<unsigned>("min_clusters_per_plane");
    max_clusters_per_reference_ = config_.get<unsigned>("max_clusters_per_plane");
    only_tracks_on_dut_ = config_.get<bool>("only_tracks_on_dut");

    if(only_tracks_on_dut_ && get_duts().size() != 1) {
        LOG(WARNING) << "Multiple DUTs in geometry, only_tracks_on_dut_ forced to true";
        only_tracks_on_dut_ = false;
    }

    auto tag_filters = config_.getMap<std::string, std::string>("filter_tags", std::map<std::string, std::string>{});
    load_tag_filters(tag_filters);

    hFilter_ = new TH1F("FilteredEvents", "Events filtered;events", 6, 0.5, 6.5);
    hFilter_->GetXaxis()->SetBinLabel(1, "Events");
    std::string label = (only_tracks_on_dut_ ? "Too few tracks on dut " : "Too few tracks");
    hFilter_->GetXaxis()->SetBinLabel(2, label.c_str());
    label = (only_tracks_on_dut_ ? "Too many tracks on dut " : "Too many tracks");
    hFilter_->GetXaxis()->SetBinLabel(3, label.c_str());
    hFilter_->GetXaxis()->SetBinLabel(4, "Too few clusters");
    hFilter_->GetXaxis()->SetBinLabel(5, "Too many clusters");
    hFilter_->GetXaxis()->SetBinLabel(6, "Events passed ");
}

StatusCode FilterEvents::run(const std::shared_ptr<Clipboard>& clipboard) {

    hFilter_->Fill(1); // number of events
    auto status = filter_tracks(clipboard) ? StatusCode::DeadTime : StatusCode::Success;
    status = filter_cluster(clipboard) ? StatusCode::DeadTime : status;
    status = filter_tags(clipboard) ? StatusCode::DeadTime : status;

    if(status == StatusCode::Success) {
        hFilter_->Fill(6);
    }
    return status;
}

void FilterEvents::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(STATUS) << "Skipped " << hFilter_->GetBinContent(1) << " events. Events passed " << hFilter_->GetBinContent(6);
}

bool FilterEvents::filter_tracks(const std::shared_ptr<Clipboard>& clipboard) {
    auto tracks = clipboard->getData<Track>();
    auto num_tracks = tracks.size();

    // check how many tracks are on the dut
    if(only_tracks_on_dut_) {
        num_tracks = 0;
        for(const auto& track : tracks) {
            if(get_duts().front()->hasIntercept(track.get()))
                num_tracks++;
        }
    }

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
    for(auto& detector : get_regular_detectors(false)) {
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

void FilterEvents::load_tag_filters(const std::map<std::string, std::string>& tag_filter_map) {
    for(auto& [tag_name, tag_filter] : tag_filter_map) {
        // locate range brackets if they exist
        std::size_t open_bracket_pos = tag_filter.find("[");
        std::size_t close_bracket_pos = tag_filter.find("]");
        if((open_bracket_pos != std::string::npos) && (close_bracket_pos != std::string::npos)) {
            if((open_bracket_pos != 0) || (close_bracket_pos != tag_filter.size() - 1)) {
                throw std::invalid_argument("invalid key value : range-based tag filter should start with an opening "
                                            "bracket and end with a closing bracket. Check for typos");
            }
            // found range brackets, now fetch range values
            std::vector<double> range_values = corryvreckan::split<double>(
                tag_filter.substr(open_bracket_pos + 1, close_bracket_pos - open_bracket_pos - 1), ":");
            if(range_values.size() > 2) {
                throw std::invalid_argument("invalid key value : tag range should hold two values in brackets, separated by "
                                            "a semicolon. Check for extra semicolon");
            }
            // define range-based filtering function
            double min = range_values.at(0);
            double max = range_values.at(1);
            tag_filter_funcs_[tag_name] = [min, max](const std::string& val) {
                double value = corryvreckan::from_string<double>(val);
                return !(value < min) && !(max < value);
            };
        } else {
            // define list-based filtering function
            std::vector<std::string> list = corryvreckan::split<std::string>(tag_filter, ",");
            tag_filter_funcs_[tag_name] = [list](const std::string& val) {
                return std::find(list.begin(), list.end(), val) != list.end();
            };
        }
    }
}

bool FilterEvents::filter_tags(const std::shared_ptr<Clipboard>& clipboard) {
    auto event = clipboard->getEvent();
    for(auto& [tag_name, filter_func] : tag_filter_funcs_) {
        try {
            auto tag_value = event->getTag(tag_name);
            if(tag_value.empty()) {
                return true;
            } else {
                bool is_tag_filter_passed = filter_func(tag_value);
                LOG(TRACE) << "Event with tag : " << tag_name << " -- value : " << tag_value << " -- "
                           << (is_tag_filter_passed ? "PASSED" : "REJETED");
                return !is_tag_filter_passed;
            }
        } catch(std::out_of_range& e) {
            throw MissingKeyError(tag_name, config_.getName());
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config_, "filter_tags", e.what());
        }
    }
    return false;
}
