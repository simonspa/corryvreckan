/**
 * @file
 * @brief Implementation of module TextWriter
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "JSONWriter.h"
#include "core/utils/file.h"

using namespace corryvreckan;

JSONWriter::JSONWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

void JSONWriter::initialise() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Create output file
    output_file_name_ =
        createOutputFile(corryvreckan::add_file_extension(config_.get<std::string>("file_name", "data"), "json"), true);
    output_file_ = std::make_unique<std::ofstream>(output_file_name_);

    *output_file_ << "[" << std::endl;

    // Read include and exclude list
    if(config_.has("include") && config_.has("exclude")) {
        throw InvalidValueError(config_, "exclude", "include and exclude parameter are mutually exclusive");
    } else if(config_.has("include")) {
        auto inc_arr = config_.getArray<std::string>("include");
        include_.insert(inc_arr.begin(), inc_arr.end());
    } else if(config_.has("exclude")) {
        auto exc_arr = config_.getArray<std::string>("exclude");
        exclude_.insert(exc_arr.begin(), exc_arr.end());
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode JSONWriter::run(std::shared_ptr<Clipboard> clipboard) {

    if(!clipboard->isEventDefined()) {
        ModuleError("No Clipboard event defined, cannot continue");
    }

    auto data = clipboard->getAll();
    LOG(DEBUG) << "Clipboard has " << data.size() << " different object types.";

    // open a new subarray for this event
    if(m_eventNumber == 0)
        *output_file_ << "[" << std::endl;
    else
        *output_file_ << "," << std::endl << "[" << std::endl;

    for(auto& block : data) {
        try {
            auto type_idx = block.first;
            auto class_name = corryvreckan::demangle(type_idx.name());
            auto class_name_full = corryvreckan::demangle(type_idx.name(), true);
            LOG(TRACE) << "Received objects of type \"" << class_name << "\"";

            // Check if these objects hsould be read
            if((!include_.empty() && include_.find(class_name) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(class_name) != exclude_.end())) {
                LOG(TRACE) << "Ignoring object " << corryvreckan::demangle(type_idx.name())
                           << " because it has been excluded or not explicitly included";
                continue;
            }

            for(auto& detector_block : block.second) {
                // Get the detector name
                std::string detector_name;
                if(!detector_block.first.empty()) {
                    detector_name = detector_block.first;
                }

                auto objects = std::static_pointer_cast<ObjectVector>(detector_block.second);
                for(auto& object : *objects) {
                    *output_file_ << TBufferJSON::ToJSON(object);
                    // add delimiter for all but the last element
                    if(object == objects->back() && detector_block == *(--block.second.end()) && block == *(--data.end())) {
                        *output_file_ << std::endl;
                    } else
                        *output_file_ << "," << std::endl;
                }
            }
        } catch(...) {
            LOG(WARNING) << "Cannot process object of type" << corryvreckan::demangle(block.first.name());
            return StatusCode::NoData;
        }
    }
    // close event array
    *output_file_ << "]";
    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void JSONWriter::finalise() {

    // finalize the JSON Array, add one empty Object to satisfy JSON Rules
    *output_file_ << std::endl << "]";
    // Print statistics
    LOG(STATUS) << "Wrote " << m_eventNumber << " events to file:" << output_file_name_ << std::endl;
}
