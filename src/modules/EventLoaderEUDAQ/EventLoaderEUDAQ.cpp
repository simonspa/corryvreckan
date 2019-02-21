#include "EventLoaderEUDAQ.h"
#include "eudaq/PluginManager.hh"

#include <algorithm>

using namespace corryvreckan;
using namespace std;

EventLoaderEUDAQ::EventLoaderEUDAQ(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)), m_longID(true) {

    m_filename = m_config.getPath("file_name", true);
    m_longID = m_config.get<bool>("long_detector_id", true);
}

void EventLoaderEUDAQ::initialise() {

    // Create new file reader:
    try {
        reader = new eudaq::FileReader(m_filename, "");
    } catch(...) {
        throw ModuleError("Unable to read input file \"" + m_filename + "\"");
    }
}

StatusCode EventLoaderEUDAQ::run(std::shared_ptr<Clipboard> clipboard) {

    // Read next event from EUDAQ reader:
    const eudaq::DetectorEvent& evt = reader->Event();
    LOG(TRACE) << evt;

    // Convert timestamp to nanoseconds, using
    // TLU frequency: 48.001e6 Hz
    // TLU frequency multiplier: 8
    // FIXME cross-check that this conversion is correct
    auto timestamp = Units::get(static_cast<double>(evt.GetTimestamp()) / (48.001e6 * 8), "s");
    LOG(DEBUG) << "EUDAQ event " << evt.GetEventNumber() << " at " << Units::display(timestamp, {"ns", "us"});

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
            for(unsigned int ipix = 0; ipix < plane.HitPixels(); ++ipix) {
                auto col = static_cast<int>(plane.GetX(ipix));
                auto row = static_cast<int>(plane.GetY(ipix));

                // Check if this pixel is masked
                if(detector->masked(col, row)) {
                    LOG(TRACE) << "Detector " << detectorID << ": pixel " << col << "," << row << " masked";
                    continue;
                }

                Pixel* pixel = new Pixel(detectorID, row, col, static_cast<int>(plane.GetPixel(ipix)));
                pixel->setCharge(plane.GetPixel(ipix));

                // Pixel gets timestamp of trigger assigned:
                pixel->timestamp(timestamp);
                deviceData->push_back(pixel);
            }

            // Store on clipboard
            clipboard->put(detectorID, "pixels", reinterpret_cast<Objects*>(deviceData));
        }
    }

    // Store event time on clipboard for subsequent modules
    // FIXME assumes trigger in center of two Mimosa26 frames:
    auto frame_length = Units::get(115.2, "us");
    clipboard->put_event(std::make_shared<Event>(timestamp - frame_length, timestamp + frame_length));

    // Advance to next event if possible, otherwise end this run:
    if(!reader->NextEvent()) {
        LOG(INFO) << "No more events in data stream.";
        return StatusCode::Failure;
    };

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
