#include "Clipboard.hpp"
#include "exceptions.h"
#include "objects/Object.hpp"

using namespace corryvreckan;

void Clipboard::put(std::string name, Objects* objects) {
    m_dataID.push_back(name);
    m_data[name] = objects;
}

void Clipboard::put(std::string name, std::string type, Objects* objects) {
    m_dataID.push_back(name + type);
    m_data[name + type] = objects;
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

double Clipboard::get_persistent(std::string name) {
    try {
        return m_persistent_data.at(name);
    } catch(std::out_of_range&) {
        throw MissingDataError(name);
    }
}

bool Clipboard::has_persistent(std::string name) {
    return m_persistent_data.find(name) != m_persistent_data.end();
}

void Clipboard::clear() {
    for(auto& id : m_dataID) {
        Objects* collection = m_data[id];
        for(Objects::iterator it = collection->begin(); it != collection->end(); it++)
            delete(*it);
        delete m_data[id];
        m_data.erase(id);
    }
    m_dataID.clear();
}

std::vector<std::string> Clipboard::listCollections() {
    std::vector<std::string> collections;
    for(auto& name : m_dataID) {
        collections.push_back(name);
    }
    return collections;
}
