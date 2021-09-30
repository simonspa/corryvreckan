/**
 * @file
 * @brief EUDAQ event-based reader to allow sequential reading of multiple runs
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "SequentialReader.h"

using namespace corryvreckan;

void SequentialReader::addFile(const std::string& filename) {
    // Files are read sequentially, and FIFO
    file_readers_.emplace(std::make_unique<eudaq::FileReader>(filename, ""));
}

bool SequentialReader::NextEvent() {
    if(!file_readers_.front()->NextEvent()) {
        // Remove the all one, assign the event, and ask again
        // if still are readers
        file_readers_.pop();
        if(file_readers_.size() == 0) {
            return false;
        };
        // and call it again
        NextEvent();
    }

    return true;
}

const eudaq::DetectorEvent& SequentialReader::GetDetectorEvent() const {
    return file_readers_.front()->GetDetectorEvent();
}
