#include "Metronome.h"
#include "objects/Event.hpp"

using namespace corryvreckan;
using namespace std;

Metronome::Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_eventLength = m_config.get<double>("event_length", Units::get<double>(10, "us"));
}

void Metronome::initialise() {

    // Set initial values for the start and stop time of the first event:
    m_eventStart = 0.0;
    m_eventEnd = m_eventLength;
}

StatusCode Metronome::run(std::shared_ptr<Clipboard> clipboard) {

    // Set up the current event:
    LOG(DEBUG) << "Defining event, time frame " << Units::display(m_eventStart, {"us", "ms", "s"}) << " to "
               << Units::display(m_eventEnd, {"us", "ms", "s"});
    clipboard->put_event(std::make_shared<Event>(m_eventStart, m_eventEnd));

    // Increment the current event's start and end times by the configured event length
    m_eventStart = m_eventEnd;
    m_eventEnd += m_eventLength;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
