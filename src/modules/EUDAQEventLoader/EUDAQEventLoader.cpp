#include "EUDAQEventLoader.h"
#include "eudaq/PluginManager.hh"

#include <algorithm>

using namespace corryvreckan;
using namespace std;

EUDAQEventLoader::EUDAQEventLoader(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)), m_longID(true) {

    m_filename = m_config.get<std::string>("file_name");
    m_longID = m_config.get<bool>("long_detector_id", true);
}

void EUDAQEventLoader::initialise() {

    // Initialise histograms per device
    for(auto& detector : get_detectors()) {
    }

    // Create new file reader:
    try {
        reader = new eudaq::FileReader(m_filename, "");
    } catch(...) {
        throw ModuleError("Unable to read input file \"" + m_filename + "\"");
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EUDAQEventLoader::run(Clipboard* clipboard) {

    // Read next event from EUDAQ reader:
    const eudaq::DetectorEvent& evt = reader->Event();
    LOG(TRACE) << evt;

    if(evt.IsBORE()) {
        // Process begin-of-run
        LOG(DEBUG) << "Found BORE";
        try {
            eudaq::PluginManager::Initialize(evt);
        } catch(const eudaq::Exception&) {
            throw ModuleError("Unknown event types encountered");
        }
    } else if(evt.IsEORE()) {
        LOG(INFO) << "Found EORE";
    } else {
        eudaq::StandardEvent sevt = eudaq::PluginManager::ConvertToStandard(evt);

        for(size_t iplane = 0; iplane < sevt.NumPlanes(); ++iplane) {
            // Get EUDAQ StandardPlane
            const eudaq::StandardPlane& plane = sevt.GetPlane(iplane);

            // Build Corryvreckan detector ID and check if this detector should be read:
            std::string detectorID =
                (m_longID ? (plane.Sensor() + "_") : "plane") + std::to_string(plane.ID()); // : (plane.ID()));

            if(!has_detector(detectorID)) {
                LOG(DEBUG) << "Skipping unknown detector " << detectorID;
                continue;
            }

            auto detector = get_detector(detectorID);

            // Make a new container for the data
            Pixels* deviceData = new Pixels();
            for(size_t ipix = 0; ipix < plane.HitPixels(); ++ipix) {
                auto col = plane.GetX(ipix);
                auto row = plane.GetY(ipix);

                // Check if this pixel is masked
                if(detector->masked(col, row)) {
                    LOG(TRACE) << "Detector " << detectorID << ": pixel " << col << "," << row << " masked";
                    continue;
                }

                Pixel* pixel = new Pixel(detectorID, row, col, plane.GetPixel(ipix));
                pixel->timestamp(m_eventNumber);
                deviceData->push_back(pixel);
            }

            // Store on clipboard
            clipboard->put(detectorID, "pixels", (Objects*)deviceData);
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Advance to next event if possible, otherwise end this run:
    if(!reader->NextEvent()) {
        LOG(INFO) << "No more events in data stream.";
        return Failure;
    };

    // Return value telling analysis to keep running
    return Success;
}

void EUDAQEventLoader::finalise() {

    LOG(DEBUG) << "Read " << m_eventNumber << " events";
}
