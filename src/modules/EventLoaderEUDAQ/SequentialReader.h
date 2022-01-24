/**
 * @file
 * @brief EUDAQ event-based reader to allow sequential reading of multiple runs
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef SequentialREADER_H__
#define SequentialREADER_H__

#include "eudaq/DetectorEvent.hh"
#include "eudaq/Event.hh"
#include "eudaq/FileReader.hh"

#include <memory>
#include <queue>
#include <string>

namespace corryvreckan {

    class SequentialReader {

    public:
        /*
         * @brief Constructor and destructor
         */
        SequentialReader() = default;
        ~SequentialReader() = default;

        /*
         * @brief Add a file to read
         * @param filename: the name of the raw file
         */
        void addFile(const std::string& filename);

        /*
         * @brief Process the next event
         */
        bool NextEvent();
        /*
         * @brief Get the current detector event
         */
        const eudaq::DetectorEvent& GetDetectorEvent() const;

    private:
        // the container of all file readers
        std::queue<std::unique_ptr<eudaq::FileReader>> file_readers_;
    };
} // namespace corryvreckan
#endif // SequentialREADER_H__
