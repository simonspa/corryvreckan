#include "Metronome.h"

using namespace corryvreckan;
using namespace std;

Metronome::Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_eventLength = m_config.get<double>("event_length", static_cast<double>(Units::convert(10, "us")));
}

void Metronome::initialise() {

    // Set initial values for the start and stop time of the first event:
    m_eventStart = 0.0;
    m_eventEnd = m_eventLength;
}

StatusCode Metronome::run(std::shared_ptr<Clipboard> clipboard) {

    // Set up the clipboard persistent storage for the current event:
    clipboard->put_persistent("eventStart", m_eventStart);
    clipboard->put_persistent("eventEnd", m_eventEnd);
    clipboard->put_persistent("eventLength", m_eventLength);

    // Increment the current event's start and end times by the configured event length
    m_eventStart = m_eventEnd;
    m_eventEnd += m_eventLength;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
