/**
 * @file
 * @brief Implementation of [EventLoaderMuPixTelescope] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderMuPixTelescope.h"

#include "dirent.h"

using namespace corryvreckan;
using namespace std;

EventLoaderMuPixTelescope::EventLoaderMuPixTelescope(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)), m_blockFile(nullptr) {
    m_filePath = m_config.getPath("input_directory");
    m_runNumber = m_config.get<int>("Run", -1); // meaningless default runnumber
    m_isSorted = m_config.get<bool>("isSorted", false);
    m_ts2IsGray = m_config.get<bool>("ts2IsGray", false);
    // We need to check for the config files in case of scans... TBI
}

void EventLoaderMuPixTelescope::initialise() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->name();
    }
    // Need to check if the files do exist
    DIR* directory = opendir(m_filePath.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << m_filePath << " does not exist";
        return;
    }
    // check the entries and if the correct file exists - seems to be inefficient
    dirent* entry;
    bool foundFile = false;
    while((entry = readdir(directory))) {
        if(entry->d_name == string("telescope_run_000115.blck")) {
            foundFile = true;
            break;
        }
    }
    if(!foundFile) {
        LOG(ERROR) << "Requested run not existing ";
        return;
    }
    BlockFile bf("bf.blck");
    bf.open_read();
    //    m_blockFile = new BlockFile(to_string(m_filePath+"/"+entry->d_name));
    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode EventLoaderMuPixTelescope::run(std::shared_ptr<Clipboard>) {

    // Loop over all detectors
    for(auto& detector : get_detectors()) {
        // Get the detector name
        std::string detectorName = detector->name();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderMuPixTelescope::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
