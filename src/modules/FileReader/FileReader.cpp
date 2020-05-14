/**
 * @file
 * @brief Implementation of ROOT data file reader module
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * @remarks The implementation of this module is based on the ROOTObjectReader module of the Allpix Squared project
 */

#include "FileReader.h"

#include <climits>
#include <string>
#include <utility>

#include <TBranch.h>
#include <TKey.h>
#include <TObjArray.h>
#include <TProcessID.h>
#include <TTree.h>

#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/Track.hpp"
#include "objects/objects.h"

using namespace corryvreckan;

FileReader::FileReader(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

/**
 * @note Objects cannot be stored in smart pointers due to internal ROOT logic
 */
FileReader::~FileReader() {
    for(auto object_inf : object_info_array_) {
        delete object_inf.objects;
    }
}

/**
 * Adds lambda function map to convert a vector of generic objects to a templated vector of objects containing this
 * particular type of object from its typeid.
 */
template <typename T> static void add_creator(FileReader::ObjectCreatorMap& map) {
    map[typeid(T)] = [&](std::vector<Object*> objects, std::string detector, const std::shared_ptr<Clipboard>& clipboard) {
        std::vector<std::shared_ptr<T>> data;
        // Copy the objects to data vector
        for(auto& object : objects) {
            data.push_back(std::make_shared<T>(*static_cast<T*>(object)));
        }

        // Fix the object references (NOTE: we do this after insertion as otherwise the objects could have been relocated)
        for(size_t i = 0; i < objects.size(); ++i) {
            auto& prev_obj = *objects[i];
            auto addr = data[i].get();
            auto& new_obj = addr;

            // Only update the reference for objects that have been referenced before
            if(prev_obj.TestBit(kIsReferenced)) {
                auto pid = TProcessID::GetProcessWithUID(new_obj);
                if(pid->GetObjectWithID(prev_obj.GetUniqueID()) != &prev_obj) {
                    LOG(ERROR) << "Duplicate object IDs, cannot correctly resolve previous history!";
                }
                prev_obj.ResetBit(kIsReferenced);
                new_obj->SetBit(kIsReferenced);
                pid->PutObjectWithID(new_obj);
            }
        }

        // Store the ojects on the clipboard:
        if(detector.empty()) {
            clipboard->putData(std::move(data));
        } else {
            clipboard->putData(std::move(data), detector);
        }
    };
}

/**
 * Uses SFINAE trick to call the add_creator function for all template arguments of a container class. Used to add creators
 * for every object in a tuple of objects.
 */
template <template <typename...> class T, typename... Args>
static void gen_creator_map_from_tag(FileReader::ObjectCreatorMap& map, type_tag<T<Args...>>) {
    std::initializer_list<int> value{(add_creator<Args>(map), 0)...};
    (void)value;
}

/**
 * Wrapper function to make the SFINAE trick in \ref gen_creator_map_from_tag work.
 */
template <typename T> static FileReader::ObjectCreatorMap gen_creator_map() {
    FileReader::ObjectCreatorMap ret_map;
    gen_creator_map_from_tag(ret_map, type_tag<T>());
    return ret_map;
}

void FileReader::initialize() {
    // Read include and exclude list
    if(config_.has("include") && config_.has("exclude")) {
        throw InvalidCombinationError(
            config_, {"exclude", "include"}, "include and exclude parameter are mutually exclusive");
    } else if(config_.has("include")) {
        auto inc_arr = config_.getArray<std::string>("include");
        include_.insert(inc_arr.begin(), inc_arr.end());
    } else if(config_.has("exclude")) {
        auto exc_arr = config_.getArray<std::string>("exclude");
        exclude_.insert(exc_arr.begin(), exc_arr.end());
    }

    // Initialize the call map from the tuple of available objects
    object_creator_map_ = gen_creator_map<corryvreckan::OBJECTS>();

    // Open the file with the objects
    input_file_ = std::make_unique<TFile>(config_.getPath("file_name", true).c_str());

    // Read all the trees in the file
    TList* keys = input_file_->GetListOfKeys();
    std::set<std::string> tree_names;

    for(auto&& object : *keys) {
        auto& key = dynamic_cast<TKey&>(*object);
        if(std::string(key.GetClassName()) == "TTree") {
            auto tree = static_cast<TTree*>(key.ReadObjectAny(nullptr));

            if(tree->GetName() == std::string("Event")) {
                LOG(DEBUG) << "Found Event object tree";
                event_tree_ = tree;
                continue;
            }

            // Check if a version of this tree has already been read
            if(tree_names.find(tree->GetName()) != tree_names.end()) {
                LOG(TRACE) << "Skipping copy of tree with name " << tree->GetName()
                           << " because one with identical name has already been processed";
                continue;
            }
            tree_names.insert(tree->GetName());

            // Check if this tree should be used
            if((!include_.empty() && include_.find(tree->GetName()) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(tree->GetName()) != exclude_.end())) {
                LOG(TRACE) << "Ignoring tree with " << tree->GetName()
                           << " objects because it has been excluded or not explicitly included";
                continue;
            }

            trees_.push_back(tree);
        }
    }

    if(trees_.empty()) {
        throw ModuleError("Provided ROOT file does not contain any trees, module cannot read any data");
    }

    // Prepare event branch:
    if(event_tree_ == nullptr) {
        throw ModuleError("Could not find \"Event\" tree to read event definitions from");
    }

    // Loop over the list of branches and create the set of receiver objects
    TObjArray* event_branches = event_tree_->GetListOfBranches();
    if(event_branches->GetEntries() != 1) {
        throw ModuleError("\"Event\" tree invalid, cannot read event data from file");
    }

    auto* event_branch = static_cast<TBranch*>(event_branches->At(0));
    event_ = new Event();
    event_branch->SetAddress(&event_);

    // Loop over all found trees
    for(auto& tree : trees_) {
        // Loop over the list of branches and create the set of receiver objects
        TObjArray* branches = tree->GetListOfBranches();
        LOG(TRACE) << "Tree \"" << tree->GetName() << "\" has " << branches->GetEntries() << " branches";
        for(int i = 0; i < branches->GetEntries(); i++) {
            auto* branch = static_cast<TBranch*>(branches->At(i));

            // Add a new vector of objects and bind it to the branch
            object_info object_inf;
            object_inf.objects = new std::vector<Object*>;
            object_info_array_.emplace_back(object_inf);
            branch->SetAddress(&(object_info_array_.back().objects));

            // Check tree structure and if object type matches name
            auto split_type = corryvreckan::split<std::string>(branch->GetClassName(), "<>");
            if(split_type.size() != 2 || split_type[1].size() <= 2) {
                throw ModuleError("Tree is malformed and cannot be used for creating objetcs");
            }
            std::string class_name = split_type[1].substr(0, split_type[1].size() - 1);
            std::string corry_namespace = "corryvreckan::";
            size_t corry_idx = class_name.find(corry_namespace);
            if(corry_idx != std::string::npos) {
                class_name.replace(corry_idx, corry_namespace.size(), "");
            }
            if(class_name != tree->GetName()) {
                throw ModuleError("Tree contains objects of the wrong type");
            }

            std::string branch_name = branch->GetName();
            if(branch_name != "global") {
                // Check if detector is registered by fetching it:
                auto detector = get_detector(branch_name);
                object_info_array_.back().detector = branch_name;
            }
        }
    }
}

StatusCode FileReader::run(const std::shared_ptr<Clipboard>& clipboard) {

    if(clipboard->isEventDefined()) {
        ModuleError("Clipboard event already defined, cannot continue");
    }

    // Read event object from tree and store it on the clipboard:
    event_tree_->GetEntry(event_num_);
    clipboard->putEvent(std::make_shared<Event>(*event_));
    read_cnt_++;

    for(auto& tree : trees_) {
        LOG(TRACE) << "Reading tree \"" << tree->GetName() << "\"";
        tree->GetEntry(event_num_);
    }

    LOG(TRACE) << "Putting stored objects on the clipboard";

    // Loop through all branches
    for(auto object_inf : object_info_array_) {
        auto objects = object_inf.objects;
        // Skip empty objects in current event
        if(objects->empty()) {
            continue;
        }

        // Check if a pointer to a dispatcher method exist
        auto first_object = (*objects)[0];
        auto iter = object_creator_map_.find(typeid(*first_object));
        if(iter == object_creator_map_.end()) {
            LOG(INFO) << "Cannot create object with type " << corryvreckan::demangle(typeid(*first_object).name())
                      << " because it not registered for clipboard storage";
            continue;
        }

        LOG(TRACE) << "- " << objects->size() << " " << corryvreckan::demangle(typeid(*first_object).name()) << ", detector "
                   << object_inf.detector;
        // Create the object vector and store it on the clipboard:
        iter->second(*objects, object_inf.detector, clipboard);

        // Update statistics
        read_cnt_ += objects->size();
    }

    event_num_++;

    if(event_num_ >= event_tree_->GetEntries()) {
        LOG(INFO) << "Requesting end of run because TTree only contains data for " << (event_num_ + 1) << " events";
        return StatusCode::EndRun;
    } else {
        return StatusCode::Success;
    }
}

void FileReader::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    int branch_count = 0;
    for(auto& tree : trees_) {
        branch_count += tree->GetListOfBranches()->GetEntries();
    }
    branch_count += event_tree_->GetListOfBranches()->GetEntries();

    // Print statistics
    LOG(INFO) << "Read " << read_cnt_ << " objects from " << branch_count << " branches";

    // Close the file
    input_file_->Close();
}
