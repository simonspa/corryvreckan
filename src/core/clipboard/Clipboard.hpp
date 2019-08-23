/**
 * @file
 * @brief Store objects for exachange between modules on the clipboard
 * @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
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
#include "objects/Event.hpp"
#include "objects/Object.hpp"

namespace corryvreckan {
    typedef std::map<std::type_index, std::map<std::string, std::shared_ptr<void>>> ClipboardData;

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
    class Clipboard {

    public:
        /**
         * @brief Construct the clipboard
         */
        Clipboard() {}
        /**
         * @brief Required virtual destructor
         */
        virtual ~Clipboard() {}

        /**
         * @brief Add object to the clipboard
         * @param name Name of the collection to be stored
         * @param type Type of the object collection to be stored
         * @param Objects vector of Objects to store
         */
        template <typename T> void put(std::shared_ptr<std::vector<T*>> objects, const std::string& key = "");

        /**
         * @brief Retrieve objects from the clipboard
         * @param name Name of the object collection to fetch
         * @param type Type of objects to be retrieved
         * @return Vector of Object pointers
         */
        template <typename T> std::shared_ptr<std::vector<T*>> get(const std::string& key = "") const;

        /**
         * @brief Check whether an event has been defined
         * @return true if an event has been defined, false otherwise
         */
        bool event_defined() const;

        /**
         * @brief Store the event object
         * @param event The event object to be stored
         * @thorws InvalidDataError in case an event exist already
         */
        void put_event(std::shared_ptr<Event> event);

        /**
         * @brief Retrieve the event object
         * @returnShared pointer to the event
         * @throws MissingDataError in case no event is available.
         */
        std::shared_ptr<Event> get_event() const;

        /**
         * @brief Store or update variable on the persistent clipboard storage
         * @param name Name of the variable
         * @param value Value to be stored
         */
        void put_persistent(std::string name, double value);

        /**
         * @brief Retrieve variable from the persistent clipboard storage
         * @param name Name of the variable
         * @return Stored value from the persistent clipboard storage
         * @throws MissingDataError in case the key is not found.
         */
        double get_persistent(std::string name) const;

        /**
         * @brief Check if variable exists on the persistent clipboard storage
         * @param name Name of the variable
         * @return True if value exists, false if it does not exist.
         */
        bool has_persistent(std::string name) const;

        /**
         * @brief Clear the event storage of the clipboard
         */
        void clear();

        /**
         * @brief Get a list of currently held collections on the clipboard event storage
         * @return Vector of collections names currently stored on the clipboard
         */
        std::vector<std::string> list_collections() const;

        /**
         * @brief Retrieve all currently stored clipboard data
         * @return All clipboard data
         */
        const ClipboardData& get_all() const;

    private:
        // Container for data, list of all data held
        ClipboardData m_data;

        // Persistent clipboard storage
        std::unordered_map<std::string, double> m_persistent_data;

        // Store the current time slice:
        std::shared_ptr<Event> m_event{};
    };
} // namespace corryvreckan

// Include template members
#include "Clipboard.tpp"

#endif // CORRYVRECKAN_CLIPBOARD_H
