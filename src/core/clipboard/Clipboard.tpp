/**
 * @file
 * @brief Template implementation of clipboard
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "exceptions.h"

#include <algorithm>

namespace corryvreckan {

    template <typename T> void Clipboard::putData(std::vector<std::shared_ptr<T>> objects, const std::string& key) {
        put_data(data_, std::move(objects), key);
    }

    template <typename T> void Clipboard::removeData(std::shared_ptr<T> object, const std::string& key) {
        remove_data(data_, std::vector<std::shared_ptr<T>>{std::move(object)}, key);
    }

    template <typename T> void Clipboard::removeData(std::vector<std::shared_ptr<T>>& objects, const std::string& key) {
        remove_data(data_, std::move(objects), key);
    }

    template <typename T> std::vector<std::shared_ptr<T>>& Clipboard::getData(const std::string& key) const {
        return get_data<T>(data_, key);
    }

    template <typename T> size_t Clipboard::countObjects(const std::string& key) const {
        return count_objects<T>(data_, key);
    }

    template <typename T>
    void Clipboard::putPersistentData(std::vector<std::shared_ptr<T>> objects, const std::string& key) {
        put_data(persistent_data_, std::move(objects), key, true);
    }

    template <typename T>
    std::vector<std::shared_ptr<T>>& ReadonlyClipboard::getPersistentData(const std::string& key) const {
        return get_data<T>(persistent_data_, key);
    }

    template <typename T> size_t ReadonlyClipboard::countPersistentObjects(const std::string& key) const {
        return count_objects<T>(persistent_data_, key);
    }

    // Translate raw pointers to their shared pointers on storage. Fail if not found.
    template <typename T> void Clipboard::copyToPersistentData(std::vector<T*> references, const std::string& key) {
        auto from_volatile = get_data<T>(data_, key);
        std::vector<std::shared_ptr<T>> to_persistent;

        // Clear vector of duplicates:
        std::sort(references.begin(), references.end());
        references.erase(std::unique(references.begin(), references.end()), references.end());

        for(auto& ref : references) {
            auto it = std::find_if(
                from_volatile.begin(), from_volatile.end(), [=](const std::shared_ptr<T>& x) { return x.get() == ref; });
            if(it == from_volatile.end()) {
                throw MissingDataError(key);
            } else {
                to_persistent.push_back(*it);
            }
        }

        // Ship off to persistent storage
        put_data(persistent_data_, std::move(to_persistent), key, true);
    }

    template <typename T>
    void Clipboard::put_data(ClipboardData& storage_element,
                             std::vector<std::shared_ptr<T>> objects,
                             const std::string& key,
                             bool append) {
        // Do not insert empty sets:
        if(objects.empty()) {
            return;
        }

        // Iterator for data type:
        auto type = storage_element.begin();

        /* If data type exists, returns iterator to offending key, if data type does not exist yet, creates new entry and
         * returns iterator to the newly created element.
         *
         * We use getBaseType here to always store objects as their base class types to be able to fetch them easily.
         * E.g. derived track classes will be stored as Track objects and can be fetched as such
         */
        type = storage_element.insert(
            type, ClipboardData::value_type(T::getBaseType(), std::map<std::string, std::shared_ptr<void>>()));

        // Insert data into data type element and print a warning if it exists already
        auto object_ptr = std::make_shared<std::vector<std::shared_ptr<T>>>(objects);
        auto element = type->second.insert(std::make_pair(key, std::static_pointer_cast<void>(object_ptr)));
        if(!element.second) {
            if(append) {
                // Get the pointer to the existing element vector
                auto existing_elements = std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(element.first->second);
                // Append new elements to the end
                existing_elements->insert(existing_elements->end(), objects.begin(), objects.end());
            } else {
                LOG(WARNING) << "Dataset of type " << corryvreckan::demangle(typeid(T).name())
                             << " already exists for key \"" << key << "\", ignoring new data";
            }
        }
    }

    template <typename T>
    void Clipboard::remove_data(ClipboardData& storage_element,
                                const std::vector<std::shared_ptr<T>>& objects,
                                const std::string& key) {

        // Try to get existing objects:
        if(storage_element.count(typeid(T)) == 0 || storage_element.at(typeid(T)).count(key) == 0) {
            return;
        }
        auto data = std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(storage_element.at(typeid(T)).at(key));
        for(const auto& object : objects) {
            auto object_iterator = std::find(data->begin(), data->end(), object);
            if(object_iterator != data->end()) {
                data->erase(object_iterator);
            }
        }

        // If we removed all objects, drop the key:
        if(data->empty()) {
            storage_element.at(typeid(T)).erase(key);
        }
    }

    template <typename T>
    std::vector<std::shared_ptr<T>>& ReadonlyClipboard::get_data(const ClipboardData& storage_element,
                                                                 const std::string& key) const {
        if(storage_element.count(typeid(T)) == 0 || storage_element.at(typeid(T)).count(key) == 0) {
            return *std::make_shared<std::vector<std::shared_ptr<T>>>();
        }
        return *std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(storage_element.at(typeid(T)).at(key));
    }

    template <typename T>
    size_t ReadonlyClipboard::count_objects(const ClipboardData& storage_element, const std::string& key) const {
        size_t number_of_objects = 0;

        // Check if we have anything of this type:
        if(storage_element.count(typeid(T)) != 0) {
            // Decide whether we should count all or just the ones identified by a key:
            if(key.empty()) {
                for(const auto& block : storage_element.at(typeid(T))) {
                    number_of_objects += std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(block.second)->size();
                }
            } else if(storage_element.at(typeid(T)).count(key) != 0) {
                number_of_objects =
                    std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(storage_element.at(typeid(T)).at(key))->size();
            }
        }
        return number_of_objects;
    }

} // namespace corryvreckan
