/**
 * @file
 * @brief Implementation of module TextWriter
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TextWriter.h"
#include "core/utils/file.h"

using namespace corryvreckan;

TextWriter::TextWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {}

void TextWriter::initialise() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->name();
    }

    // Create output file
    output_file_name_ =
        createOutputFile(corryvreckan::add_file_extension(m_config.get<std::string>("file_name", "data"), "txt"), true);
    output_file_ = std::make_unique<std::ofstream>(output_file_name_);

    *output_file_ << "# Corryvreckan ASCII data" << std::endl << std::endl;

    // Read include and exclude list
    if(m_config.has("include") && m_config.has("exclude")) {
        throw InvalidValueError(m_config, "exclude", "include and exclude parameter are mutually exclusive");
    } else if(m_config.has("include")) {
        auto inc_arr = m_config.getArray<std::string>("include");
        include_.insert(inc_arr.begin(), inc_arr.end());
    } else if(m_config.has("exclude")) {
        auto exc_arr = m_config.getArray<std::string>("exclude");
        exclude_.insert(exc_arr.begin(), exc_arr.end());
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode TextWriter::run(std::shared_ptr<Clipboard> clipboard) {

    if(!clipboard->isEventDefined()) {
        ModuleError("No Clipboard event defined, cannot continue");
    }

    // Print the current event:
    *output_file_ << "=== " << m_eventNumber << " ===" << std::endl;
    *output_file_ << *clipboard->getEvent() << std::endl;

    auto data = clipboard->getAll();
    LOG(DEBUG) << "Clipboard has " << data.size() << " different object types.";

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
                } else {
                    detector_name = "<global>";
                }

                *output_file_ << "--- " << detector_name << " ---" << std::endl;

                auto objects = std::static_pointer_cast<ObjectVector>(detector_block.second);
                for(auto& object : *objects) {
                    *output_file_ << *object << std::endl;
                }
            }
        } catch(...) {
            LOG(WARNING) << "Cannot process object of type" << corryvreckan::demangle(block.first.name());
            return StatusCode::NoData;
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void TextWriter::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
