/**
 * @file
 * @brief Implementation of module EventLoaderMuPixTelescope
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderMuPixTelescope.h"
#include "dirent.h"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;
// using namespace std;

EventLoaderMuPixTelescope::EventLoaderMuPixTelescope(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)), m_blockFile(nullptr) {
    m_inputDirectory = m_config.getPath("input_directory");
    m_runNumber = m_config.get<int>("Run", -1); // meaningless default runnumber
    m_isSorted = m_config.get<bool>("is_sorted", false);
    m_ts2IsGray = m_config.get<bool>("ts2_is_gray", false);
    // We need to check for the config files in case of scans... TBI
}

void EventLoaderMuPixTelescope::initialise() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Need to check if the files do exist
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    // check the entries and if the correct file exists continue - seems to be inefficient
    dirent* entry;
    bool foundFile = false;
    while((entry = readdir(directory))) {
        if(entry->d_name == string("telescope_run_001020_mergedFrames.blck")) {
            foundFile = true;
            break;
        }
    }
    if(!foundFile) {
        LOG(ERROR) << "Requested run not existing ";
        return;
    } else
        LOG(INFO) << "File found" << endl;
    string file = (m_inputDirectory + "/" + entry->d_name);
    LOG(INFO) << "reading " << file;
    m_blockFile = new mudaq::BlockFile(file);
    if(!m_blockFile->open_read()) {
        LOG(ERROR) << "File cannot be read" << endl;
        return;
    } else
        LOG(STATUS) << "Loaded Reader";
    hHitMap = new TH2F("hitMap", "hitMap; column; row", 50, -0.5, 49.5, 202, -0.5, 201.5);
    hPixelToT = new TH1F("pixelToT", "pixelToT; ToT in TS2 clock cycles.; ", 64, -0.5, 63.5);
    hTimeStamp = new TH1F("pixelTS", "pixelTS; TS in clock cycles; ", 1024, -0.5, 1023.5);
}

StatusCode EventLoaderMuPixTelescope::run(std::shared_ptr<Clipboard> clipboard) {

    // Loop over all detectors
    vector<string> detectors;
    for(auto& detector : get_detectors()) {
        // Get the detector name
        std::string detectorName = detector->getName();
        detectors.push_back(detectorName);
        LOG(DEBUG) << "Detector with name " << detectorName;
    }
    map<string, std::shared_ptr<PixelVector>> dataContainers;
    mudaq::TelescopeFrame tf;
    double frame_start = std::numeric_limits<double>::max();
    double frame_end = std::numeric_limits<double>::min();

    if(!m_blockFile->read_next(tf))
        return StatusCode::EndRun;
    else {
        LOG(DEBUG) << "Found " << tf.num_hits() << " in event";
        for(uint i = 0; i < tf.num_hits(); ++i) {
            mudaq::RawHit h = tf.get_hit(i);
            if(h.tag() == 0x4)
                h = tf.get_hit(i, 66);
            double px_timestamp = 8 * static_cast<double>(((tf.timestamp() >> 2) & 0xFFFFF700) + h.timestamp_raw());
            Pixel* p = new Pixel(detectors.at(h.tag() / 4), h.column(), h.row(), 0, 0, px_timestamp);

            // Select earlies and latest pixel:
            frame_start = (px_timestamp < frame_start ? px_timestamp : frame_start);
            frame_end = (px_timestamp > frame_end ? px_timestamp : frame_end);

            if(!dataContainers.count(detectors.at(h.tag() / 4)))
                dataContainers[detectors.at(h.tag() / 4)] = std::make_shared<PixelVector>();
            dataContainers.at(detectors.at(h.tag() / 4))->push_back(p);
            hHitMap->Fill(h.column(), h.row());
            hTimeStamp->Fill(h.timestamp_raw());
        }
    }

    for(auto d : detectors) {
        if(!dataContainers.count(d))
            continue;
        try {
            clipboard->putData(dataContainers[d], d);
        } catch(ModuleError& e) {
            LOG(ERROR) << "Unknown detector ";
        }
    }

    // Store current frame time and the length of the event:
    LOG(DEBUG) << "Frame with " << tf.num_hits() << " hits, time: " << Units::display(frame_start, {"ns", "us", "s"})
               << ", length: " << Units::display((frame_end - frame_start), {"ns", "us", "s"});
    clipboard->putEvent(std::make_shared<Event>(frame_start, frame_end));

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
