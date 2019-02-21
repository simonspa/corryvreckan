/**
 * @file
 * @brief Implementation of [EventLoaderEUDAQ2] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderEUDAQ2.h"
#include "eudaq/FileReader.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

using namespace corryvreckan;

EventLoaderEUDAQ2::EventLoaderEUDAQ2(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_filename = m_config.getPath("file_name", true);
}

void EventLoaderEUDAQ2::initialise() {

    auto detectorID = m_detector->name();
    LOG(DEBUG) << "Initialise for detector " + detectorID;
    // Initialise member variables
    m_eventNumber = 0;
    m_totalEvents = 0;

    // open the input file with the eudaq reader
    try {
        reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), m_filename);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << m_filename
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(m_config, "file_path", "Parsing error!");
    }
}

StatusCode EventLoaderEUDAQ2::run(std::shared_ptr<Clipboard>) {

    // Read next event from EUDAQ2 reader:
    m_eventNumber++;
    auto evt = reader->GetNextEvent();
    if(!evt) {
        LOG(DEBUG) << "!ev --> return, empty event!";
        return StatusCode::NoData;
    }

    //        auto evt_tags = [] (const std::map<std::string, std::string>& tags) {
    //            std::stringstream output;
    //            for(const auto& tag : tags) {
    //                output << tag.first << ": " << tag.second << ", ";
    //            }
    //            return output.str();
    //        };

    //        LOG(DEBUG) << "#ev: "<< evt->GetEventNumber()
    //                    << ", #Run " << evt->GetRunNumber()
    //                    << ", TSBeg " << evt->GetTimestampBegin()
    //                    << ", TSEnd " << evt->GetTimestampEnd()
    //                    << ", descr " << evt->GetDescription()
    //                    << ", evID " << evt->GetEventID()
    //                    << ", isBORE " << evt->IsBORE()
    //                    << ", isEORE " << evt->IsEORE()
    ////                    << ", isFake " << evt->IsFlagFake()
    ////                    << ", isPacket " << evt->IsFlagPacket()
    ////                    << ", isTS " << evt->IsFlagTimestamp()
    ////                    << ", istTrg " << evt->IsFlagTrigger()
    //                    << ", tags " << evt_tags(evt->GetTags());

    auto stdev = eudaq::StandardEvent::MakeShared();
    if(eudaq::StdEventConverter::Convert(evt, stdev, nullptr)) {
        if(stdev->NumPlanes() == 0) {
            return StatusCode::NoData;
        }
        auto plane = stdev->GetPlane(0);
        auto nHits = plane.GetPixels<int>().size(); // number of hits
        auto nPixels = plane.TotalPixels();         // total pixels in matrix
        LOG(INFO) << "Number of Hits: " << nHits << " / total pixel number: " << nPixels;
        for(unsigned int i = 0; i < nHits; i++) {
            LOG(INFO) << "\t x: " << plane.GetX(i, 0) << " y: " << plane.GetY(i, 0);
        }
    }

    // Convert timestamp to nanoseconds, using
    // AIDA TLU frequency: 40 MHz
    // (see https://github.com/PaoloGB/firmware_AIDA/raw/master/Documentation/Latex/Main_TLU.pdf)
    // TLU frequency multiplier: 25
    //        auto timestamp = Units::get(static_cast<double>(evt.GetTimestamp()) * 25, "ns");
    //        LOG(DEBUG) << "EUDAQ event " << evt.GetEventNumber() << " at " << Units::display(timestamp, {"ns", "us"});

    //        if(evt.IsBORE()) {
    //            // Process begin-of-run
    //            LOG(DEBUG) << "Found BORE";
    //            try {
    //                eudaq::PluginManager::Initialize(evt);
    //            } catch(const eudaq::Exception&) {
    //                throw ModuleError("Unknown event types encountered");
    //            }
    //        } else if(evt.IsEORE()) {
    //            LOG(INFO) << "Found EORE";
    //        } else {
    //            eudaq::StandardEvent sevt = eudaq::PluginManager::ConvertToStandard(evt);
    //        }
    //    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderEUDAQ2::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
