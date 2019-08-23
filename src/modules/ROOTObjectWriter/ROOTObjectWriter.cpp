/**
 * @file
 * @brief Implementation of ROOT data file writer module
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ROOTObjectWriter.h"

#include <fstream>
#include <string>
#include <utility>

#include <TBranchElement.h>
#include <TClass.h>

#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace corryvreckan;

ROOTObjectWriter::ROOTObjectWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {}
/**
 * @note Objects cannot be stored in smart pointers due to internal ROOT logic
 */
ROOTObjectWriter::~ROOTObjectWriter() {
    // Delete all object pointers
    for(auto& index_data : write_list_) {
        delete index_data.second;
    }
}

void ROOTObjectWriter::initialise() {
    // Create output file
    output_file_name_ =
        createOutputFile(corryvreckan::add_file_extension(m_config.get<std::string>("file_name", "data"), "root"), true);
    output_file_ = std::make_unique<TFile>(output_file_name_.c_str(), "RECREATE");
    output_file_->cd();

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

    // Create event tree:
    event_tree_ = std::make_unique<TTree>("Event", (std::string("Tree of Events").c_str()));
}

StatusCode ROOTObjectWriter::run(std::shared_ptr<Clipboard> clipboard) {

    if(!clipboard->event_defined()) {
        ModuleError("No Clipboard event defined, cannot continue");
    }

    auto event = clipboard->get_event();
    event_tree_->Branch("global", event.get());
    event_tree_->Fill();

    auto data = clipboard->get_all();
    LOG(DEBUG) << "Clipboard has " << data.size() << " different object types.";

    for(auto& block : data) {
        try {
            auto type_idx = block.first;
            auto class_name = corryvreckan::demangle(type_idx.name());
            auto class_name_full = corryvreckan::demangle(type_idx.name(), true);
            LOG(TRACE) << "Received objects of type \"" << class_name << "\"";

            // Check if these objects should be stored
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

                auto objects = std::static_pointer_cast<Objects>(detector_block.second);
                LOG(TRACE) << " - " << detector_name << ": " << objects->size();

                // Create a new branch of the correct type if this object has not been received before
                auto index_tuple = std::make_tuple(type_idx, detector_name);
                if(write_list_.find(index_tuple) == write_list_.end()) {

                    // Add vector of objects to write to the write list
                    write_list_[index_tuple] = new std::vector<Object*>();
                    auto addr = &write_list_[index_tuple];

                    if(trees_.find(class_name) == trees_.end()) {
                        // Create new tree
                        output_file_->cd();
                        trees_.emplace(
                            class_name,
                            std::make_unique<TTree>(class_name.c_str(), (std::string("Tree of ") + class_name).c_str()));
                    }

                    std::string branch_name = detector_name.empty() ? "global" : detector_name;

                    trees_[class_name]->Bronch(
                        branch_name.c_str(), (std::string("std::vector<") + class_name_full + "*>").c_str(), addr);
                }

                // Fill the branch vector
                for(auto& object : *objects) {
                    ++write_cnt_;
                    write_list_[index_tuple]->push_back(object);
                }
            }
        } catch(...) {
            LOG(WARNING) << "Cannot process object of type" << corryvreckan::demangle(block.first.name());
            return StatusCode::NoData;
        }
    }

    LOG(TRACE) << "Writing new objects to tree";
    output_file_->cd();

    // Fill the tree with the current received messages
    for(auto& tree : trees_) {
        tree.second->Fill();
    }

    // Clear the current message list
    for(auto& index_data : write_list_) {
        index_data.second->clear();
    }

    return StatusCode::Success;
}

void ROOTObjectWriter::finalise() {
    LOG(TRACE) << "Writing objects to file";
    output_file_->cd();

    int branch_count = 0;
    for(auto& tree : trees_) {
        // Update statistics
        branch_count += tree.second->GetListOfBranches()->GetEntries();
    }

    // Finish writing to output file
    output_file_->Write();

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects to " << branch_count << " branches in file:" << std::endl
                << output_file_name_;
}
