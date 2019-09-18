/**
 * @file
 * @brief Definition of ROOT data file reader module
 * @copyright Copyright (c) 2017-2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <functional>
#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/module/Module.hpp"

namespace corryvreckan {
    /**
     * @ingroup Modules
     * @brief Module to read data stored in ROOT file back to the Corryvreckan clipboard
     * @remarks The implementation of this module is based on the ROOTObjectReader module of the Allpix Squared project
     *
     * Reads the tree of objects in the data format of the \ref FileWriter module. Copies all stored objects that are
     * supported back to the clipboard.
     */
    class FileReader : public Module {
    public:
        using ObjectCreatorMap =
            std::map<std::type_index,
                     std::function<void(std::vector<Object*>, std::string detector, std::shared_ptr<Clipboard> clipboard)>>;

        /**
         * @brief Constructor for this global module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors List of detectors to perform task on
         */
        FileReader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        /**
         * @brief Destructor deletes the internal objects read from ROOT Tree
         */
        ~FileReader() override;

        /**
         * @brief Open the ROOT file containing the stored output data
         */
        void initialise() override;

        /**
         * @brief Move the objects stored for the current event to the clipboard
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard) override;

        /**
         * @brief Output summary and close the ROOT file
         */
        void finalise() override;

    private:
        /**
         * @brief Internal object storing objects and information to construct a message from tree
         */
        struct object_info {
            std::vector<Object*>* objects;
            std::string detector;
        };

        // Object names to include or exclude from reading
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // File containing the objects
        std::unique_ptr<TFile> input_file_;

        // Object trees in the file
        std::vector<TTree*> trees_;
        TTree* event_tree_{nullptr};
        Event* event_{};

        // List of objects and detector information converted from the trees
        std::list<object_info> object_info_array_;

        // Statistics for total amount of objects stored
        unsigned long read_cnt_{};

        // Counter for number of events read:
        int event_num_{};

        // Internal map to construct an object from it's type index
        ObjectCreatorMap object_creator_map_;
    };
} // namespace corryvreckan
