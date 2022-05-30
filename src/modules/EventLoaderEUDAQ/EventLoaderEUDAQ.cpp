/**
 * @file
 * @brief Implementation of module EventLoaderEUDAQ
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <algorithm>

#include "TDirectory.h"
#include "TH2F.h"

#include "EventLoaderEUDAQ.h"

#include "eudaq/PluginManager.hh"

using namespace corryvreckan;
using namespace std;

EventLoaderEUDAQ::EventLoaderEUDAQ(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)), m_longID(true) {

    config_.setDefault<bool>("long_detector_id", true);

    m_filenames = config_.getPathArray("file_name", true);
    m_longID = config_.get<bool>("long_detector_id");
}

void EventLoaderEUDAQ::initialize() {

    // Create files reader:
    reader = std::make_unique<SequentialReader>();
    for(const auto& file : m_filenames) {
        try {
            reader->addFile(file);
        } catch(...) {
            throw ModuleError("Unable to read input file \"" + file.string() + "\"");
        }
    }

    // Loop over all planes
    for(auto& detector : get_regular_detectors(true)) {
        auto detectorID = detector->getName();

        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(detectorID.c_str());
        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();

        // Create a hitmap for each detector
        std::string title = detectorID + ": hitmap;x [px];y [px];events";
        hitmap[detectorID] = new TH2F("hitmap",
                                      title.c_str(),
                                      detector->nPixels().X(),
                                      -0.5,
                                      detector->nPixels().X() - 0.5,
                                      detector->nPixels().Y(),
                                      -0.5,
                                      detector->nPixels().Y() - 0.5);
    }
}
StatusCode EventLoaderEUDAQ::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Read next event from EUDAQ reader:
    const eudaq::DetectorEvent& evt = reader->GetDetectorEvent();
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
            PixelVector deviceData;
            for(unsigned int ipix = 0; ipix < plane.HitPixels(); ++ipix) {
                auto col = static_cast<int>(plane.GetX(ipix));
                auto row = static_cast<int>(plane.GetY(ipix));

                if(col >= detector->nPixels().X() || row >= detector->nPixels().Y()) {
                    LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix with size "
                                 << detector->nPixels();
                }

                // Check if this pixel is masked
                if(detector->masked(col, row)) {
                    LOG(TRACE) << "Detector " << detectorID << ": pixel " << col << "," << row << " masked";
                    continue;
                }

                // when calibration is not available, set charge = tot, timestamp not available -> set to 0
                auto pixel = std::make_shared<Pixel>(
                    detectorID, col, row, static_cast<int>(plane.GetPixel(ipix)), plane.GetPixel(ipix), 0.);

                // Pixel gets timestamp of trigger assigned:
                pixel->timestamp(timestamp);
                deviceData.push_back(pixel);

                // Fill the hitmap
                hitmap[detectorID]->Fill(pixel->column(), pixel->row());
            }

            // Store on clipboard
            clipboard->putData(deviceData, detectorID);
        }
    }

    // Store event time on clipboard for subsequent modules
    // FIXME assumes trigger in center of two Mimosa26 frames:
    auto frame_length = Units::get(115.2, "us");
    clipboard->putEvent(std::make_shared<Event>(timestamp - frame_length, timestamp + frame_length));

    // Advance to next event if possible, otherwise end this run:
    if(!reader->NextEvent()) {
        LOG(INFO) << "No more events in data stream.";
        return StatusCode::Failure;
    };

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
