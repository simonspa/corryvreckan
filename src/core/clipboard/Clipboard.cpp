/**
 * @file
 * @brief Implementation of clipboard storage
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Clipboard.hpp"
#include "exceptions.h"
#include "objects/Object.hpp"

using namespace corryvreckan;

bool Clipboard::isEventDefined() const {
    return (event_ != nullptr);
}

void Clipboard::putEvent(std::shared_ptr<Event> event) {
    // Already defined:
    if(event_) {
        throw InvalidDataError("Event already defined. Only one module can place an event definition");
    } else {
        event_ = event;
    }
}

std::shared_ptr<Event> Clipboard::getEvent() const {
    if(!event_) {
        throw InvalidDataError("Event not defined. Add Metronome module or Event reader defining the event");
    }
    return event_;
}

void Clipboard::clear() {
    // Loop over all data types
    for(auto& block : data_) {
        // Loop over all stored collections of this type
        for(auto& set : block.second) {
            for(auto& obj : (*std::static_pointer_cast<ObjectVector>(set.second))) {
                // All objects are destroyed together in this clear function at the end of the event. To avoid costly
                // reverse-iterations through the TRef dependency hash lists, we just tell ROOT not to care about possible
                // TRef-dependants and to just destroy the object directly by resetting the `kMustCleanup` bit.
                obj->ResetBit(kMustCleanup);
            }
        }
    }

    // Clear the data
    data_.clear();

    // Resetting the event definition:
    event_.reset();
}

std::vector<std::string> Clipboard::listCollections() const {
    std::vector<std::string> collections;

    for(const auto& block : data_) {
        std::string line(corryvreckan::demangle(block.first.name()));
        line += ": ";
        for(const auto& set : block.second) {
            std::shared_ptr<ObjectVector> collection = std::static_pointer_cast<ObjectVector>(set.second);
            line += set.first + " (" + collection->size() + ") ";
        }
        line += "\n";
        collections.push_back(line);
    }
    return collections;
}

const ClipboardData& Clipboard::getAll() const {
    return data_;
}
