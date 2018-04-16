#include "EUDAQ2EventLoader.h"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

using namespace corryvreckan;
using namespace std;

EUDAQ2EventLoader::EUDAQ2EventLoader(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_filename = m_config.get<std::string>("file_name");
}

void EUDAQ2EventLoader::initialise() {

    // Initialise histograms per device
    for(auto& detector : get_detectors()) {
    }

    // Create new file reader:
    try {
        reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        throw ModuleError("Unable to read input file \"" + m_filename + "\"");
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EUDAQ2EventLoader::run(Clipboard* clipboard) {

    // auto reader_ev =
    // auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(reader_ev);
    // if(!stdev) {
    auto stdev = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(reader->GetNextEvent(), stdev, nullptr); // no conf
    // }
    auto& ev = *(stdev.get());

    LOG(DEBUG) << "Called onEvent " << ev.GetEventNumber();
    LOG(DEBUG) << "Number of Planes " << ev.NumPlanes();

    // FIXME store all event tags on clipboard as EUDAQSignals?

    // Loop over all contained planes
    for(unsigned int p = 0; p < ev.NumPlanes(); p++) {

        const eudaq::StandardPlane& plane = ev.GetPlane(p);
        LOG(TRACE) << "Plane ID     " << plane.ID();
        LOG(TRACE) << "Plane Size   " << sizeof(plane);
        LOG(TRACE) << "Plane Frames " << plane.NumFrames();

        for(unsigned int f = 0; f < plane.NumFrames(); f++) {
            for(unsigned int index = 0; index < plane.HitPixels(f); index++) {
                SimpleStandardHit hit((int)plane.GetX(index, f), (int)plane.GetY(index, f));
                hit.setTOT((int)plane.GetPixel(index, f));
                hit.setLVL1(f);
            }

            // Increment event counter
            m_eventNumber++;

            // Return value telling analysis to keep running
            return Success;
        }

        void EUDAQ2EventLoader::finalise() { LOG(DEBUG) << "Analysed " << m_eventNumber << " events"; }
