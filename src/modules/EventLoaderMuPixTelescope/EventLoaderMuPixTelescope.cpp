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
    config_.setDefault<int>("buffer_depth", 1000);

    m_inputDirectory = config_.getPath("input_directory");
    m_runNumber = config_.get<int>("Run");
    m_buffer_depth = config.get<int>("buffer_depth");
    m_isSorted = config_.get<bool>("is_sorted");
    m_ts2IsGray = config_.get<bool>("ts2_is_gray");
    // We need to check for the config files in case of scans... TBI
}

void EventLoaderMuPixTelescope::initialize() {

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
    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << m_runNumber;
    std::string s = ss.str();
    std::string fileName = "telescope_run_" + s + ".blck";

    // Need to check if the files do exist
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        throw MissingDataError("Cannot open directory: " + m_inputDirectory);
    }
    while((entry = readdir(directory))) {
        if(entry->d_name == fileName) {
            foundFile = true;
            break;
        }
    }
    if(!foundFile) {
        throw MissingDataError("Cannot open data file: " + fileName);
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
    hHitMap = new TH2F("hitMap",
                       "hitMap; column; row",
                       m_detector->nPixels().x(),
                       -.05,
                       m_detector->nPixels().x() - .5,
                       m_detector->nPixels().y(),
                       -.05,
                       m_detector->nPixels().y() - .5);
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
    PixelVector hits;
    while(true) {
        if(m_pixelbuffer.size() == 0)
            break;
        auto pixel = m_pixelbuffer.top();
        if((pixel->timestamp() < clipboard->getEvent()->start())) {
            LOG(DEBUG) << " Old hit found: " << pixel->timestamp();
            m_removed++;
            m_pixelbuffer.pop(); // remove top element
            continue;
        }
        if(m_pixelbuffer.size() && (pixel->timestamp() < clipboard->getEvent()->end()) &&
           (pixel->timestamp() > clipboard->getEvent()->start())) {
            hits.push_back(pixel);
            hHitMap->Fill(pixel.get()->column(), pixel.get()->row());
            hPixelToT->Fill(pixel.get()->raw());
            // igitt
            hTimeStamp->Fill(fmod((pixel.get()->timestamp() / 8.), pow(2, 10)));
            m_pixelbuffer.pop();
        } else {
            break;
        }
        if(signed(m_pixelbuffer.size()) < m_buffer_depth)
            fillBuffer();
    }
    if(hits.size() > 0)
        clipboard->putData(hits, m_detector->getName());
    // Return value telling analysis to keep running
    if(m_pixelbuffer.size() == 0)
        return StatusCode::EndRun;

    return StatusCode::Success;
}

void EventLoaderMuPixTelescope::fillBuffer() {
    TelescopeFrame tf;
    while(m_pixelbuffer.size() < unsigned(m_buffer_depth)) {
        if(m_blockFile->read_next(tf)) {
            // no hits in data - can only happen if the zero suppression is switched off
            if(tf.num_hits() == 0)
                continue;
            // need to determine the sensor layer that is identified by the tag
            RawHit h = tf.get_hit(0);
            // tag does not match - continue reading
            if((h.tag() & uint(~0x3)) != m_tag)
                continue;
            // all hits in one frame are from the same sensor. Copy them
            for(uint i = 0; i < tf.num_hits(); ++i) {
                h = tf.get_hit(i, m_type);
                // move ts to ns
                double px_timestamp = 8 * static_cast<double>(((tf.timestamp() >> 2) & 0xFFFFFFFFFFC00) + h.timestamp_raw());
                // setting tot and charge to zero here - needs to be improved
                m_pixelbuffer.push(std::make_shared<Pixel>(m_detector->getName(), h.column(), h.row(), 0, 0, px_timestamp));
            }
        } else {
            m_eof = true;
            break;
        }
    }
}
