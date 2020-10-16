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

EventLoaderMuPixTelescope::EventLoaderMuPixTelescope(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_removed(0), m_detector(detector), m_blockFile(nullptr) {

    config_.setDefault<bool>("is_sorted", false);
    config_.setDefault<bool>("ts2_is_gray", false);

    m_inputDirectory = config_.getPath("input_directory");
    m_runNumber = config_.get<int>("Run");
    m_isSorted = config_.get<bool>("is_sorted");
    m_ts2IsGray = config_.get<bool>("ts2_is_gray");
    // We need to check for the config files in case of scans... TBI
}

void EventLoaderMuPixTelescope::initialize() {

    // Need to check if the files do exist
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    if(m_detector->getName() == "mp10_0")
        m_tag = 0x0;
    else if(m_detector->getName() == "mp10_1")
        m_tag = 0x4;
    else if(m_detector->getName() == "mp10_2")
        m_tag = 0x8;
    else if(m_detector->getName() == "mp10_3")
        m_tag = 0xC;

    m_type = 107;
    if(m_detector->getName() == "mp10_1")
        m_type = 2027;
    // check the entries and if the correct file exists continue - seems to be inefficient
    dirent* entry;
    bool foundFile = false;
    while((entry = readdir(directory))) {
        if(entry->d_name == string("telescope_run_000015.blck")) {
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
    m_blockFile = new BlockFile(file);
    if(!m_blockFile->open_read()) {
        LOG(ERROR) << "File cannot be read" << endl;
        return;
    } else
        LOG(STATUS) << "Loaded Reader";
    hHitMap = new TH2F("hitMap", "hitMap; column; row", 50, -0.5, 49.5, 202, -0.5, 201.5);
    hPixelToT = new TH1F("pixelToT", "pixelToT; ToT in TS2 clock cycles.; ", 64, -0.5, 63.5);
    hTimeStamp = new TH1F("pixelTS", "pixelTS; TS in clock cycles; ", 1024, -0.5, 1023.5);
}

void EventLoaderMuPixTelescope::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    LOG(INFO) << "Removed " << m_removed << " hits that could not be properly sorted";
}

StatusCode EventLoaderMuPixTelescope::run(const std::shared_ptr<Clipboard>& clipboard) {

    // refill the buffer
    if(!m_eof)
        fillBuffer();
    // std::cout << clipboard->getEvent()->start() << "\t" << clipboard->getEvent()->end()<<std::endl;
    PixelVector hits;
    while((m_pixelbuffer.front()->timestamp() < clipboard->getEvent()->start())) {
        LOG(DEBUG) << " Old hit found: " << m_pixelbuffer.front()->timestamp();
        m_removed++;
        m_pixelbuffer.erase(m_pixelbuffer.begin());
    }
    while(m_pixelbuffer.size() && (m_pixelbuffer.front()->timestamp() < clipboard->getEvent()->end()) &&
          (m_pixelbuffer.front()->timestamp() > clipboard->getEvent()->start())) {
        //       m_pixelbuffer.front()->print(LOG(DEBUG));
        hits.push_back(m_pixelbuffer.front());
        m_pixelbuffer.erase(m_pixelbuffer.begin());
    };

    if(hits.size() > 0)
        clipboard->putData(hits, m_detector->getName());
    // Return value telling analysis to keep running
    if(m_pixelbuffer.size() == 0)
        return StatusCode::EndRun;
    return StatusCode::Success;
}

void EventLoaderMuPixTelescope::fillBuffer() {
    TelescopeFrame tf;
    while(m_pixelbuffer.size() < 100) {
        if(m_blockFile->read_next(tf)) {
            // no hits in data
            if(tf.num_hits() == 0)
                continue;

            RawHit h = tf.get_hit(0);
            // wrong tag
            if((h.tag() & uint(~0x3)) != m_tag)
                continue;
            for(uint i = 0; i < tf.num_hits(); ++i) {
                h = tf.get_hit(i, m_type);
                // move ts to ns
                double px_timestamp = 8 * static_cast<double>(((tf.timestamp() >> 2) & 0xFFFFFFFFFFC00) + h.timestamp_raw());
                m_pixelbuffer.emplace_back(
                    std::make_shared<Pixel>(m_detector->getName(), h.column(), h.row(), 0, 0, px_timestamp));
                // fill hit data
                hHitMap->Fill(h.column(), h.row());
                hTimeStamp->Fill(h.timestamp_raw());
            }
        } else {
            m_eof = true;
            break;
        }
    }
    // sort the hits by timestamp
    auto _sort = [](std::shared_ptr<Pixel> const a, std::shared_ptr<Pixel> const b) {
        return a->timestamp() < b->timestamp();
    };
    std::sort(m_pixelbuffer.begin(), m_pixelbuffer.end(), _sort);
}
