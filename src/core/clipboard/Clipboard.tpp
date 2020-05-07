/**
 * @file
 * @brief Template implementation of clipboard
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace corryvreckan {

    template <typename T> void Clipboard::putData(std::vector<std::shared_ptr<T>> objects, const std::string& key) {
        // Do not insert empty sets:
        if(objects.empty()) {
            return;
        }

        // Iterator for data type:
        ClipboardData::iterator type = data_.begin();

        /* If data type exists, returns iterator to offending key, if data type does not exist yet, creates new entry and
         * returns iterator to the newly created element.
         *
         * We use getBaseType here to always store objects as their base class types to be able to fetch them easily. E.g.
         * derived track classes will be stored as Track objects and can be fetched as such
         */
        type =
            data_.insert(type, ClipboardData::value_type(T::getBaseType(), std::map<std::string, std::shared_ptr<void>>()));

        // Insert data into data type element and print a warning if it exists already
        auto object_ptr = std::make_shared<std::vector<std::shared_ptr<T>>>(objects);
        auto test = type->second.insert(std::make_pair(key, std::static_pointer_cast<void>(object_ptr)));
        if(!test.second) {
            LOG(WARNING) << "Dataset of type " << corryvreckan::demangle(typeid(T).name()) << " already exists for key \""
                         << key << "\", ignoring new data";
        }
    }

    template <typename T> std::vector<std::shared_ptr<T>>& Clipboard::getData(const std::string& key) const {
        if(data_.count(typeid(T)) == 0 || data_.at(typeid(T)).count(key) == 0) {
            return *std::make_shared<std::vector<std::shared_ptr<T>>>();
        }
        return *std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(data_.at(typeid(T)).at(key));
    }

    template <typename T> size_t Clipboard::countObjects(const std::string& key) const {
        size_t number_of_objects = 0;

        // Check if we have anything of this type:
        if(data_.count(typeid(T)) != 0) {
            // Decide whether we should count all or just the ones identified by a key:
            if(key.empty()) {
                for(const auto& block : data_.at(typeid(T))) {
                    number_of_objects += std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(block.second)->size();
                }
            } else if(data_.at(typeid(T)).count(key) != 0) {
                number_of_objects =
                    std::static_pointer_cast<std::vector<std::shared_ptr<T>>>(data_.at(typeid(T)).at(key))->size();
            }
        }
        return number_of_objects;
    }

} // namespace corryvreckan
