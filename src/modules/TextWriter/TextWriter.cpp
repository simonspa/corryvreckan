/**
 * @file
 * @brief Implementation of a module writing data to text
 * Copyright (c) 2019 CERN and the Corryvreckan authors.
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
    LOG(TRACE) << "Writing new objects to text file";

    // Print the current event:
    *output_file_ << "=== " << m_eventNumber << " ===" << std::endl;

    for(auto& objName : clipboard->listCollections()) {

        Objects* objects = clipboard->get(objName);
        LOG(DEBUG) << "Got " << objects->size() << " entries for object " << objName;

        if(objects->size() == 0) {
            LOG(DEBUG) << "Nothing to write for this object type";
            continue;
        }

        const Object* first_object = *(objects)->begin();
        auto* cls = TClass::GetClass(typeid(*(first_object)));

        std::string cls_name = cls->GetName();
        std::string corry_namespace = "corryvreckan::";
        size_t cls_iterator = cls_name.find(corry_namespace);

        std::string objType = cls_name.replace(cls_iterator, corry_namespace.size(), "");

        if((*(*objects).begin())->getDetectorID() != "") {

            if((!include_.empty() && include_.find(objType) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(objType) != exclude_.end())) {
                LOG(TRACE) << "Won't write objects of type " << objType << " due to explicit user request";
            } else {
                *output_file_ << "--- " << (*(*objects).begin())->getDetectorID() << " ---" << std::endl;
                *output_file_ << "+++ " << objType << " +++" << std::endl;

                for(auto& obj : (*objects)) {
                    *output_file_ << *(obj) << std::endl;
                }
            }
        } else {
            if((!include_.empty() && include_.find(objType) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(objType) != exclude_.end())) {
                LOG(TRACE) << "Won't write objects of type " << objName << " due to explicit user request";
            } else {
                *output_file_ << "--- <global> ---" << std::endl;
                *output_file_ << "+++ " << objType << " +++" << std::endl;

                for(auto& obj : (*objects)) {
                    *output_file_ << *(obj) << std::endl;
                }
            }
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
