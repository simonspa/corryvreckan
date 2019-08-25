#include "Clipboard.hpp"
#include "exceptions.h"
#include "objects/Object.hpp"

using namespace corryvreckan;

void Clipboard::put_persistent(std::string name, double value) {
    m_persistent_data[name] = value;
}

double Clipboard::get_persistent(std::string name) const {
    try {
        return m_persistent_data.at(name);
    } catch(std::out_of_range&) {
        throw MissingDataError(name);
    }
}

bool Clipboard::event_defined() const {
    return (m_event != nullptr);
}

void Clipboard::put_event(std::shared_ptr<Event> event) {
    // Already defined:
    if(m_event) {
        throw InvalidDataError("Event already defined. Only one module can place an event definition");
    } else {
        m_event = event;
    }
}

std::shared_ptr<Event> Clipboard::get_event() const {
    if(!m_event) {
        throw InvalidDataError("Event not defined. Add Metronome module or Event reader defining the event");
    }
    return m_event;
}

bool Clipboard::has_persistent(std::string name) const {
    return m_persistent_data.find(name) != m_persistent_data.end();
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
                delete(*it);
            }
            set = collections.erase(set);
        }
        block = m_data.erase(block);
    }

    // Resetting the event definition:
    m_event.reset();
}

std::vector<std::string> Clipboard::list_collections() const {
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

const ClipboardData& Clipboard::get_all() const {
    return m_data;
}
