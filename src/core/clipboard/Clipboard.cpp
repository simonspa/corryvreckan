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

bool Clipboard::has_persistent(std::string name) const {
    return m_persistent_data.find(name) != m_persistent_data.end();
}

void Clipboard::clear() {
    // Loop over all data types
    for(auto block = m_data.cbegin(); block != m_data.cend();) {
        auto collections = block->second;

        // Loop over all stored collections of this type
        for(auto set = collections.cbegin(); set != collections.cend();) {
            Objects* collection = set->second;
            // Loop over all objects and delete them
            for(Objects::iterator it = collection->begin(); it != collection->end(); ++it) {
                delete(*it);
            }
            delete collection;
            set = collections.erase(set);
        }
        block = m_data.erase(block);
    }
}

std::vector<std::string> Clipboard::listCollections() {
    std::vector<std::string> collections;

    for(const auto& block : m_data) {
        std::string line(corryvreckan::demangle(block.first.name()));
        line += ": ";
        for(const auto& set : block.second) {
            line += set.first + " (" + set.second->size() + ") ";
        }
        line += "\n";
        collections.push_back(line);
    }
    return collections;
}
