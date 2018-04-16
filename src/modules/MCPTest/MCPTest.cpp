#include "MCPTest.h"

using namespace corryvreckan;
using namespace std;

MCPTest::MCPTest(Configuration config, std::vector<Detector*> detectors) : Module(std::move(config), std::move(detectors)) {}

void MCPTest::initialise() {}

StatusCode MCPTest::run(Clipboard* clipboard) {

    LOG(DEBUG) << "Running event " << m_eventNumber;

    // Loop over all Timepix3 and make plots
    for(auto& detector : get_detectors()) {
        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }
        LOG(DEBUG) << std::endl << "Picked up " << pixels->size() << " objects from detector " << detector->name();

        for(auto& pixel : (*pixels)) {
            auto local = detector->getLocalPosition(pixel->m_row, pixel->m_column);
            auto global = detector->localToGlobal(local);
            LOG(DEBUG) << "Pixel Hit @ (" << pixel->m_column << "," << pixel->m_row << ")" << std::endl
                       << " local:  " << local << std::endl
                       << " global: " << global;
        }
    }
    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void MCPTest::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
