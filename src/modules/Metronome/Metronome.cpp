/**
 * @file
 * @brief Implementation of module Metronome
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Metronome.h"
#include "objects/Event.hpp"

using namespace corryvreckan;
using namespace std;

Metronome::Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)), m_triggers(0) {

    m_eventLength = m_config.get<double>("event_length", Units::get<double>(10, "us"));
    m_triggersPerEvent = m_config.get<uint32_t>("triggers", 0);
}

void Metronome::initialise() {

    // Set initial values for the start and stop time of the first event:
    m_eventStart = m_config.get<double>("skip_time", 0.);
    m_eventEnd = m_eventStart + m_eventLength;
}

StatusCode Metronome::run(std::shared_ptr<Clipboard> clipboard) {

    // Set up the current event:
    auto event = std::make_shared<Event>(m_eventStart, m_eventEnd);
    LOG(DEBUG) << "Defining event, time frame " << Units::display(m_eventStart, {"us", "ms", "s"}) << " to "
               << Units::display(m_eventEnd, {"us", "ms", "s"});

    if(m_triggersPerEvent > 1) {
        LOG(DEBUG) << "Adding " << m_triggersPerEvent << " triggers to event, IDs " << m_triggers << "-"
                   << (m_triggers + m_triggersPerEvent - 1);
    } else if(m_triggersPerEvent > 0) {
        LOG(DEBUG) << "Adding " << m_triggersPerEvent << " trigger to event, ID " << m_triggers;
    }
    for(uint32_t i = 0; i < m_triggersPerEvent; i++) {
        event->addTrigger(m_triggers++, (m_eventStart + m_eventEnd) / 2);
    }

    clipboard->putEvent(event);

    // Increment the current event's start and end times by the configured event length
    m_eventStart = m_eventEnd;
    m_eventEnd += m_eventLength;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
