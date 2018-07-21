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
#include <string>
#include <unordered_map>

#include "core/utils/log.h"
#include "objects/Object.hpp"

namespace corryvreckan {

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
         * @param objects vector of Objects to store
         */
        void put(std::string name, Objects* objects);

        /**
         * @brief Add object to the clipboard
         * @param name Name of the collection to be stored
         * @param type Type of the object collection to be stored
         * @param Objects vector of Objects to store
         */
        void put(std::string name, std::string type, Objects* objects);

        /**
         * @brief Retrieve objects from the clipboard
         * @param name Name of the object collection to fetch
         * @param type Type of objects to be retrieved
         * @return Vector of Object pointers
         */
        Objects* get(std::string name, std::string type = "");

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
         * @throws MissingKeyError in case the key is not found.
         */
        double get_persistent(std::string name);

        /**
         * @brief Check if variable exists on the persistent clipboard storage
         * @param name Name of the variable
         * @return True if value exists, false if it does not exist.
         */
        bool has_persistent(std::string name);

        /**
         * @brief Clear the event storage of the clipboard
         */
        void clear();

        /**
         * @brief Get a list of currently held collections on the clipboard event storage
         * @return Vector of collections names currently stored on the clipboard
         */
        std::vector<std::string> listCollections();

    private:
        // Container for data, list of all data held
        std::map<std::string, Objects*> m_data;

        // List of available data collections
        std::vector<std::string> m_dataID;

        // Persistent clipboard storage
        std::unordered_map<std::string, double> m_persistent_data;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_CLIPBOARD_H
