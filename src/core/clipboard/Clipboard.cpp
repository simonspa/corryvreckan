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

void Clipboard::putPersistentData(std::string name, double value) {
    m_persistent_data[name] = value;
}

double Clipboard::getPersistentData(std::string name) const {
    try {
        return m_persistent_data.at(name);
    } catch(std::out_of_range&) {
        throw MissingDataError(name);
    }
}

bool Clipboard::hasPersistentData(std::string name) const {
    return m_persistent_data.find(name) != m_persistent_data.end();
}

bool Clipboard::isEventDefined() const {
    return (m_event != nullptr);
}

void Clipboard::putEvent(std::shared_ptr<Event> event) {
    // Already defined:
    if(m_event) {
        throw InvalidDataError("Event already defined. Only one module can place an event definition");
    } else {
        m_event = event;
    }
}

std::shared_ptr<Event> Clipboard::getEvent() const {
    if(!m_event) {
        throw InvalidDataError("Event not defined. Add Metronome module or Event reader defining the event");
    }
    return m_event;
}

void Clipboard::clear() {
    // Loop over all data types
    for(auto block = m_data.cbegin(); block != m_data.cend();) {
        auto collections = block->second;

        // Loop over all stored collections of this type
        for(auto set = collections.cbegin(); set != collections.cend();) {
            std::shared_ptr<ObjectVector> collection = std::static_pointer_cast<ObjectVector>(set->second);
            // Loop over all objects and delete them
            for(ObjectVector::iterator it = collection->begin(); it != collection->end(); ++it) {
                // All objects are destroyed together in this clear function at the end of the event. To avoid costly
                // reverse-iterations through the TRef dependency hash lists, we just tell ROOT not to care about possible
                // TRef-dependants and to just destroy the object directly by resetting the `kMustCleanup` bit.
                (*it)->ResetBit(kMustCleanup);
                // Delete the object itself:
                delete(*it);
            }
            set = collections.erase(set);
        }
        block = m_data.erase(block);
    }

    // Resetting the event definition:
    m_event.reset();
}

std::vector<std::string> Clipboard::listCollections() const {
    std::vector<std::string> collections;

    for(const auto& block : m_data) {
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
    return m_data;
}
