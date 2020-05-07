/**
 * @file
 * @brief Implementation of module ImproveReferenceTimestamp
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ImproveReferenceTimestamp.h"
#include "objects/SpidrSignal.hpp"

using namespace corryvreckan;
using namespace std;

ImproveReferenceTimestamp::ImproveReferenceTimestamp(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {
    m_method = m_config.get<int>("improvement_method", 1);
    m_source = m_config.get<std::string>("signal_source", "W0013_G02");
    m_triggerLatency = m_config.get<double>("trigger_latency", Units::get<double>(0, "ns"));
}

void ImproveReferenceTimestamp::initialise() {
    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ImproveReferenceTimestamp::run(std::shared_ptr<Clipboard> clipboard) {

    // Recieved triggers
    std::vector<double> trigger_times;

    // Get trigger signals
    auto spidrData = clipboard->getData<SpidrSignal>(m_source);
    // Loop over all signals registered
    for(auto& signal : spidrData) {
        if(signal->type() == "trigger") {
            trigger_times.push_back(signal->timestamp() - m_triggerLatency);
        }
    }
    LOG(DEBUG) << "Number of triggers found: " << trigger_times.size();

    // Get the tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    LOG(DEBUG) << "Number of tracks found: " << tracks.size();
    for(auto& track : tracks) {

        double improved_time = track->timestamp();

        // Use trigger timestamp
        if(m_method == 0) {
            // Find trigger timestamp closest in time
            double diff = std::numeric_limits<double>::max();
            for(auto& trigger_time : trigger_times) {
                LOG(DEBUG) << " track: " << track->timestamp() << " trigger: " << trigger_time
                           << " diff: " << Units::display(abs(trigger_time - track->timestamp()), {"ns", "us", "s"})
                           << " diff stored: " << Units::display(diff, {"ns", "us", "s"});
                if(abs(trigger_time - track->timestamp()) < diff) {
                    improved_time = trigger_time;
                    diff = abs(trigger_time - track->timestamp());
                }
            }
        }

        // Use average track timestamp
        else if(m_method == 1) {
            int nhits = 0;
            double avg_track_time = 0;
            for(auto& cluster : track->getClusters()) {
                avg_track_time += cluster->timestamp();
                nhits++;
            }
            improved_time = round(avg_track_time / nhits);
            LOG(DEBUG) << setprecision(12) << "Reference track time "
                       << Units::display(track->timestamp(), {"ns", "us", "s"});
            LOG(DEBUG) << setprecision(12) << "Average track time " << Units::display(improved_time, {"ns", "us", "s"});
        }

        // Set improved reference timestamp
        track->setTimestamp(improved_time);

        LOG(DEBUG) << "End of track";
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}

void ImproveReferenceTimestamp::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
