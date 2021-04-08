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

ImproveReferenceTimestamp::ImproveReferenceTimestamp(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    config_.setDefault<std::string>("signal_source", "W0013_G02");
    config_.setDefault<double>("trigger_latency", Units::get<double>(0, "ns"));
    config_.setDefault<double>("search_window", Units::get<double>(100, "ns"));

    m_source = config_.get<std::string>("signal_source");
    m_triggerLatency = config_.get<double>("trigger_latency");
    m_searchWindow = config_.get<double>("search_window");
}

void ImproveReferenceTimestamp::initialize() {
    // Initialise member variables
    m_eventNumber = 0;
    m_improvedTriggers = 0;
    m_noTriggerFound = 0;

    hTriggersPerEvent = new TH1D("hTriggersPerEvent", "hTriggersPerEvent;triggers per event;# entries", 20, -0.5, 19.5);
    hTracksPerEvent = new TH1D("hTracksPerEvent", "hTracksPerEvent;tracks per event;# entries", 20, -0.5, 19.5);
    hTrackTriggerTimeCorrelation = new TH1D("hTrackTriggerTimeCorrelation",
                                            "hTrackTriggerTimeCorrelation;ts_{track} - ts_{trigger} [ns];# entries",
                                            1000,
                                            -500.5,
                                            499.5);
}

StatusCode ImproveReferenceTimestamp::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Received triggers
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

    hTriggersPerEvent->Fill(static_cast<double>(trigger_times.size()));
    hTracksPerEvent->Fill(static_cast<double>(tracks.size()));

    for(auto& track : tracks) {

        double improved_time = track->timestamp();

        // Find trigger timestamp closest in time
        double diff = std::numeric_limits<double>::max();
        if(!trigger_times.size()) {
            // set to non-sense timestamp because we cannot delete from clipboard
            track->setTimestamp(-1);
            m_noTriggerFound++;
        } else {
            for(auto& trigger_time : trigger_times) {

                hTrackTriggerTimeCorrelation->Fill(track->timestamp() - trigger_time);

                LOG(DEBUG) << " track: " << track->timestamp() << " trigger: " << trigger_time
                           << " diff: " << Units::display(abs(trigger_time - track->timestamp()), {"ns", "us", "s"})
                           << " diff stored: " << Units::display(diff, {"ns", "us", "s"});
                if(abs(trigger_time - track->timestamp()) < diff) {
                    improved_time = trigger_time;
                    diff = abs(trigger_time - track->timestamp());
                }
            }
            if(diff < m_searchWindow) {
                track->setTimestamp(improved_time);
                m_improvedTriggers++;
            } else {
                track->setTimestamp(-1);
                m_noTriggerFound++;
            }
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}

void ImproveReferenceTimestamp::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(STATUS) << "Analysed " << m_eventNumber << " events, improved track timestamps: " << m_improvedTriggers
                << ", no trigger found: " << m_noTriggerFound;
}
