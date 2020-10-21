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
#include <string>
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
    if(config_.has("input_file"))
        m_input_file = config_.get<string>("input_file");
    // We need to check for the config files in case of scans... TBI
}

void EventLoaderMuPixTelescope::initialize() {
    // extract the tag from the detetcor name
    string tag = m_detector->getName();
    if(tag.find("_") < tag.length())
        tag = tag.substr(tag.find("_") + 1);
    m_tag = uint(stoi(tag, nullptr, 16));
    LOG(DEBUG) << m_detector->getName() << " is using the fpga link tag " << hex << m_tag;
    m_type = typeString_to_typeID(m_detector->getType());

    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << m_runNumber;
    std::string s = ss.str();
    std::string fileName = "telescope_run_" + s + ".blck";
    // overwrite default file name in case of more exotic naming scheme
    if(m_input_file.size() > 0)
        fileName = m_input_file;

    // check the if folder and file do exist
    dirent* entry;
    bool foundFile = false;
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
        throw MissingDataError("Cannot read data file: " + fileName);
    }
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

int EventLoaderMuPixTelescope::typeString_to_typeID(string typeString) {
    // This stuff is required to take large number of different hardware conversions into account...
    // Might be replaced if a better way is implemented on the DAQ side
    if(typeString == "mupix8")
        return MP8_SORTED_TS2;
    else if(typeString == "mupix9")
        return MP10_SORTED_TS2;
    else if(typeString == "mupix10")
        return MP10_UNSORTED_GS1_GS2;
    else if(typeString == "run2020v1")
        return R20V1_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v2")
        return R20V2_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v3")
        return R20V3_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v4")
        return R20V4_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v5")
        return R20V5_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v6")
        return R20V6_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v7")
        return R20V7_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v8")
        return R20V8_UNSORTED_GS1_GS2_GS3;
    else if(typeString == "run2020v9")
        return R20V9_UNSORTED_GS1_GS2_GS3;
    else
        throw InvalidModuleActionException(typeString + " is an invalid mupix styled sensor");
}

void EventLoaderMuPixTelescope::fillBuffer() {
    // here we need to check quite a number of cases
    TelescopeFrame tf;
    while(m_pixelbuffer.size() < unsigned(m_buffer_depth)) {
        if(m_blockFile->read_next(tf)) {
            // no hits in data - can only happen if the zero suppression is switched off
            if(tf.num_hits() == 0)
                continue;
            // need to determine the sensor layer that is identified by the tag
            RawHit h = tf.get_hit(0);
            // tag does not match - continue reading if data is not sorted
            if(((h.tag() & uint(~0x3)) != m_tag) && !m_isSorted)
                continue;
            // all hits in one frame are from the same sensor. Copy them
            for(uint i = 0; i < tf.num_hits(); ++i) {
                h = tf.get_hit(i, m_type);
                // move ts to ns - i'd like to do this already on the mupix8_DAQ side, but have not found the time yet
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
