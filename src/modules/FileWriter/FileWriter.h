/**
 * @file
 * @brief Definition of ROOT data file writer module
 * @copyright Copyright (c) 2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/module/Module.hpp"

namespace corryvreckan {
    /**
     * @ingroup Modules
     * @brief Module to write object data to ROOT trees in file for persistent storage
     *
     * Reads the whole clipboard. Creates a tree as soon as a new type of object is encountered and
     * saves the data in those objects to tree for every event. The tree name is the class name of the object. A separate
     * branch is created for every combination of detector name and message name that outputs this object.
     */
    class FileWriter : public Module {
    public:
        /**
         * @brief Constructor for this global module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors List of detectors to perform task on
         */
        FileWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

        /**
         * @brief Destructor deletes the internal objects used to build the ROOT Tree
         */
        ~FileWriter() override;

        /**
         * @brief Opens the file to write the objects to
         */
        void initialise() override;

        /**
         * @brief Writes the objects fetched to their specific tree, constructing trees on the fly for new objects.
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard) override;

        /**
         * @brief Finalize file writing, provide statistics information
         */
        void finalise() override;

    private:
        // Object names to include or exclude from writing
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // Output data file to write
        std::unique_ptr<TFile> output_file_;
        std::string output_file_name_{};

        // List of trees that are stored in data file
        std::map<std::string, std::unique_ptr<TTree>> trees_;
        std::unique_ptr<TTree> event_tree_;
        Event* event_{};

        // List of objects of a particular type, bound to a specific detector and having a particular name
        std::map<std::tuple<std::type_index, std::string>, std::vector<Object*>*> write_list_;

        // Statistical information about number of objects
        unsigned long write_cnt_{};
    };
} // namespace corryvreckan
