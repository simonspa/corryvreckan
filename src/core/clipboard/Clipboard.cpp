#include "Clipboard.hpp"
#include "core/utils/log.h"
#include "exceptions.h"
#include "objects/Object.hpp"

using namespace corryvreckan;

void Clipboard::put(std::string name, Objects* objects) {
    m_data.insert(ClipboardData::value_type(name, objects));
}

void Clipboard::put(std::string name, std::string type, Objects* objects) {
    m_data.insert(ClipboardData::value_type(name + type, objects));
}

void Clipboard::put_persistent(std::string name, double value) {
    m_persistent_data[name] = value;
}

Objects* Clipboard::get(std::string name, std::string type) {
    if(m_data.count(name + type) == 0) {
        return nullptr;
    }
    return m_data[name + type];
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
    for(auto set = m_data.cbegin(); set != m_data.cend();) {
        Objects* collection = set->second;
        for(Objects::iterator it = collection->begin(); it != collection->end(); ++it) {
            delete(*it);
        }
        delete collection;
        set = m_data.erase(set);
    }

    // Resetting the event definition:
    m_event.reset();
}

std::vector<std::string> Clipboard::listCollections() const {
    std::vector<std::string> collections;
    for(auto& dataset : m_data) {
        collections.push_back(dataset.first);
    }
    return collections;
}
