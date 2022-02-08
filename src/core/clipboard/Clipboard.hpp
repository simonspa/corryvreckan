/**
 * @file
 * @brief Store objects for exachange between modules on the clipboard
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_CLIPBOARD_H
#define CORRYVRECKAN_CLIPBOARD_H

#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "core/utils/log.h"
#include "core/utils/type.h"
#include "objects/Event.hpp"
#include "objects/Object.hpp"

namespace corryvreckan {
    using ClipboardData = std::map<std::type_index, std::map<std::string, std::shared_ptr<void>>>;

    class ReadonlyClipboard {
    public:
        /**
         * @brief Construct the clipboard
         */
        ReadonlyClipboard() = default;
        /**
         * @brief Required virtual destructor
         */
        virtual ~ReadonlyClipboard() = default;

        /**
         * @brief Method to retrieve objects from the clipboard
         * @param key Identifying key of objects to be fetched. Defaults to empty key
         */
        template <typename T> std::vector<std::shared_ptr<T>>& getPersistentData(const std::string& key = "") const;

        /**
         * @brief Method to count the number of objects of a given type on the clipboard
         * @param key Identifying key of objects to be counted. An empty key will count all objects available.
         */
        template <typename T> size_t countPersistentObjects(const std::string& key = "") const;

    protected:
        /**
         * @brief Method to retrieve data from a clipboard storage element depending on the template type
         * @param storage_element  Clipboard storage element to retrieve data from
         * @param key              Data key to search for
         * @return Vector of shared pointers to the data blocks found or empty vector if none were found
         */
        template <typename T>
        std::vector<std::shared_ptr<T>>& get_data(const ClipboardData& storage_element, const std::string& key) const;

        /**
         * @brief Count the number of objects of a given type and key on a storage element
         * @param storage_element  Clipboard storage element to retrieve data from
         * @param key              Data key to search for
         * @return Number of objects of given type and key found on the storage element
         */
        template <typename T> size_t count_objects(const ClipboardData& storage_element, const std::string& key) const;

        // Persistent clipboard storage
        ClipboardData persistent_data_;
    };

    /**
     * @brief Class for temporary data storage for exachange between modules
     *
     * The Clipboard class is used to transfer information between modules during the event processing. \ref Objects can be
     * placed on the clipboard, and retrieved by their name. At the end of each event, the clipboard is
     * wiped clean.
     *
     * In addition, a permanent clipboard storage area for variables of type double is provided, which allow to exchange
     * information which should outlast a single event. This is dubbed the "persistent storage"
     */
    class Clipboard : public ReadonlyClipboard {
        friend class ModuleManager;

    public:
        /**
         * @brief Method to add a vector of objects to the clipboard
         * @param objects Shared pointer to vector of objects to be stored
         * @param key     Identifying key for this set of objects. Defaults to empty key
         */
        template <typename T> void putData(std::vector<std::shared_ptr<T>> objects, const std::string& key = "");

        /**
         * @brief Method to retrieve objects from the clipboard
         * @param key Identifying key of objects to be fetched. Defaults to empty key
         */
        template <typename T> std::vector<std::shared_ptr<T>>& getData(const std::string& key = "") const;

        /**
         * @brief Method to remove an existing object from the clipboard
         * @param object  Shared pointer of the object to be removed
         * @param key     Identifying key for the set this object belongs to. Defaults to empty key
         */
        template <typename T> void removeData(std::shared_ptr<T> object, const std::string& key = "");

        /**
         * @brief Method to remove existing objects from the clipboard
         * @param objects List of objects to be removed
         * @param key     Identifying key for the set these objects belongs to. Defaults to empty key
         */
        template <typename T> void removeData(std::vector<std::shared_ptr<T>>& objects, const std::string& key = "");

        /**
         * @brief Method to count the number of objects of a given type on the clipboard
         * @param key Identifying key of objects to be counted. An empty key will count all objects available.
         */
        template <typename T> size_t countObjects(const std::string& key = "") const;

        /**
         * @brief Check whether an event has been defined
         * @return true if an event has been defined, false otherwise
         */
        bool isEventDefined() const;

        /**
         * @brief Store the event object
         * @param event The event object to be stored
         * @throws InvalidDataError in case an event exist already
         */
        void putEvent(std::shared_ptr<Event> event);

        /**
         * @brief Retrieve the event object
         * @return Shared pointer to the event
         * @throws MissingDataError in case no event is available.
         */
        std::shared_ptr<Event> getEvent() const;

        /**
         * @brief Method to add a vector of objects to the clipboard
         * @param objects Shared pointer to vector of objects to be stored
         * @param key     Identifying key for this set of objects. Defaults to empty key
         */
        template <typename T> void putPersistentData(std::vector<std::shared_ptr<T>> objects, const std::string& key = "");

        /**
         * @brief Method to find objects in the event storage and copy the to the persistent storage of the clipboard.
         *
         * This method is useful to copy objects to persistent storage from which only the references, i.e. raw pointers are
         * available. This method looks up the relevant volatile storage element and compares the stored objects with the
         * pointers provided. It then stores the matched objects on permanent storage for later reference.
         *
         * The vector delivered at the input is cleared of duplicates.
         *
         * @param objects Vector of raw pointers of data elements already stored on the event storage element
         * @param key     Identifying key for this set of objects. Defaults to empty key
         * @throws MissingDataError if the related object could not be found on the storage
         */
        template <typename T> void copyToPersistentData(std::vector<T*> objects, const std::string& key = "");

        /**
         * @brief Get a list of currently held collections on the clipboard event storage
         * @return Vector of collections names currently stored on the clipboard
         */
        std::vector<std::string> listCollections() const;

        /**
         * @brief Retrieve all currently stored clipboard data
         * @return All clipboard data
         */
        const ClipboardData& getAll() const;

    private:
        /**
         * @brief Clear the event storage of the clipboard
         */
        void clear();

        /**
         * Helper to put new data onto clipboard
         * @param storage_element The storage element of the clipboard to store data in
         * @param objects         Data to be stored
         * @param key             Key under which data will be stored
         * @param append          Flag whether data should be appended to existing key or not
         */
        template <typename T>
        void put_data(ClipboardData& storage_element,
                      std::vector<std::shared_ptr<T>> objects,
                      const std::string& key,
                      bool append = false);

        /**
         * Helper to remove a set of objects from the clipboard
         * @param  storage_element Storage element concerned
         * @param  objects         Objects to be removed
         * @param  key             Key under which objects should be searched
         */
        template <typename T>
        void
        remove_data(ClipboardData& storage_element, const std::vector<std::shared_ptr<T>>& objects, const std::string& key);

        // Container for data, list of all data held
        ClipboardData data_;

        // Store the current time slice:
        std::shared_ptr<Event> event_{};
    };
} // namespace corryvreckan

// Include template members
#include "Clipboard.tpp"

#endif // CORRYVRECKAN_CLIPBOARD_H
