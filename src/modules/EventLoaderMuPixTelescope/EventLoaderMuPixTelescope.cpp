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
    : Module(config, detector), removed_(0), detector_(detector), blockFile_(nullptr) {

    config_.setDefault<bool>("is_sorted", false);
    config_.setDefault<bool>("ts2_is_gray", false);
    config_.setDefault<unsigned>("buffer_depth", 1000);
    config_.setDefault<double>("time_offset", 0.0);

    inputDirectory_ = config_.getPath("input_directory");
    runNumber_ = config_.get<int>("Run");
    buffer_depth_ = config.get<unsigned>("buffer_depth");
    isSorted_ = config_.get<bool>("is_sorted");
    timeOffset_ = config_.get<double>("time_offset");
    if(config_.has("input_file"))
        input_file_ = config_.get<string>("input_file");
}

void EventLoaderMuPixTelescope::initialize() {
    // extract the tag from the detetcor name
    string tag = detector_->getName();
    if(tag.find("_") < tag.length())
        tag = tag.substr(tag.find("_") + 1);
    tag_ = uint(stoi(tag, nullptr, 16));
    LOG(DEBUG) << detector_->getName() << " is using the fpga link tag " << hex << tag_;
    if(typeString_to_typeID.find(detector_->getType()) == typeString_to_typeID.end()) {
        throw KeyValueParseError("tag " + std::to_string(tag_), "Sensor tag not supported");
    }
    type_ = typeString_to_typeID.at(detector_->getType());
    LOG(INFO) << "Detector " << detector_->getType() << "is assigned to type id " << type_;
    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << runNumber_;
    std::string s = ss.str();
    std::string fileName = "telescope_run_" + s + ".blck";
    // overwrite default file name in case of more exotic naming scheme
    if(input_file_.size() > 0)
        fileName = input_file_;

    // check the if folder and file do exist
    dirent* entry;
    bool foundFile = false;
    DIR* directory = opendir(inputDirectory_.c_str());
    if(directory == nullptr) {
        throw MissingDataError("Cannot open directory: " + inputDirectory_);
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
    string file = (inputDirectory_ + "/" + entry->d_name);
    LOG(INFO) << "reading " << file;
    blockFile_ = new BlockFile(file);
    if(!blockFile_->open_read()) {
        throw MissingDataError("Cannot read data file: " + fileName);
    }
    hHitMap = new TH2F("hitMap",
                       "hitMap; column; row",
                       detector_->nPixels().x(),
                       -.05,
                       detector_->nPixels().x() - .5,
                       detector_->nPixels().y(),
                       -.05,
                       detector_->nPixels().y() - .5);
    hdiscardedHitmap = new TH2F("discardedhitMap",
                                "hitMap of out of event hits; column; row",
                                detector_->nPixels().x(),
                                -.05,
                                detector_->nPixels().x() - .5,
                                detector_->nPixels().y(),
                                -.05,
                                detector_->nPixels().y() - .5);
    hPixelToT = new TH1F("pixelToT", "pixelToT; ToT in TS2 clock cycles.; ", 64, -0.5, 63.5);
    hTimeStamp = new TH1F("pixelTS", "pixelTS; TS in clock cycles; ", 1024, -0.5, 1023.5);
    hHitsEvent = new TH1F("hHitsEvent", "hHitsEvent; # hits per event; ", 300, -.5, 299.5);
    hitsPerkEvent = new TH1F("hHitsPerkEvent", "hitsper1kevents; corry events /1k; hits per 1k events", 1000, -.5, 999.5);
}

void EventLoaderMuPixTelescope::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    LOG(INFO) << "Number of hits put to clipboard: " << stored_
              << " and number of removed (not fitting in an event) hits: " << removed_;
    if(!isSorted_)
        LOG(INFO) << "Increasing the buffer depth might reduce this number.";
}

StatusCode EventLoaderMuPixTelescope::run(const std::shared_ptr<Clipboard>& clipboard) {
    eventNo_++;
    pixels_.clear();
    // get the hits
    StatusCode result = (isSorted_ ? read_sorted(clipboard) : read_unsorted(clipboard));
    hHitsEvent->Fill(double(pixels_.size()));
    counterHits_ += pixels_.size();
    if(eventNo_ % 1000 == 0) {
        int point = eventNo_ / 1000;
        hitsPerkEvent->Fill(point, double(counterHits_));
        counterHits_ = 0;
    }
    if(pixels_.size() > 0)
        clipboard->putData(pixels_, detector_->getName());
    stored_ += pixels_.size();
    return result;
}

StatusCode EventLoaderMuPixTelescope::read_sorted(const std::shared_ptr<Clipboard>& clipboard) {
    PixelVector hits;
    if(!blockFile_->read_next(tf_)) {
        return StatusCode::EndRun;
    }
    for(uint i = 0; i < tf_.num_hits(); ++i) {
        RawHit h = tf_.get_hit(i, type_);
        if(((h.tag() & uint(~0x3)) == tag_))
            continue;
        // move ts to ns - i'd like to do this already on the mupix8_DAQ side, but have not found the time yet, assuming
        // 10bit ts
        double px_timestamp = 8 * static_cast<double>(((tf_.timestamp() >> 2) & 0xFFFFFFFFFFC00) + h.timestamp_raw());
        // setting tot and charge to zero here - needs to be improved
        pixels_.push_back(std::make_shared<Pixel>(detector_->getName(), h.column(), h.row(), 0, 0, px_timestamp));
    }
    // If no event is defined create one
    if(clipboard->getEvent() == nullptr) {
        // The readout FPGA creates frames with a length of 128 time stamps, each 8ns. The int division cuts of lowest bits
        int begin = int(pixels_.front()->timestamp()) / 1024;
        clipboard->putEvent(std::make_shared<Event>(double(begin * 1024), double((begin + 1) * 1024)));
    }
    return StatusCode::Success;
}

StatusCode EventLoaderMuPixTelescope::read_unsorted(const std::shared_ptr<Clipboard>& clipboard) {
    if(!eof_)
        fillBuffer();
    while(true) {
        if(pixelbuffer_.size() == 0)
            break;
        auto pixel = pixelbuffer_.top();
        if((pixel->timestamp() < clipboard->getEvent()->start())) {
            LOG(DEBUG) << " Old hit found: " << Units::convert(pixel->timestamp(), "us") << " vs prev end (" << eventNo_ - 1
                       << ")\t" << Units::convert(prev_event_end_, "us") << " and current start \t"
                       << Units::display(clipboard->getEvent()->start(), "us")
                       << " and duration: " << clipboard->getEvent()->duration()
                       << "and number of triggers: " << clipboard->getEvent()->triggerList().size();
            removed_++;
            hdiscardedHitmap->Fill(pixel->column(), pixel->row());
            pixelbuffer_.pop(); // remove top element
            continue;
        }
        if(pixelbuffer_.size() && (pixel->timestamp() < clipboard->getEvent()->end()) &&
           (pixel->timestamp() > clipboard->getEvent()->start())) {
            LOG(DEBUG) << " Adding pixel hit: " << Units::convert(pixel->timestamp(), "us") << " vs prev end ("
                       << eventNo_ - 1 << ")\t" << Units::convert(prev_event_end_, "us") << " and current start \t"
                       << Units::display(clipboard->getEvent()->start(), "us")
                       << " and duration: " << Units::display(clipboard->getEvent()->duration(), "us");
            pixels_.push_back(pixel);
            hHitMap->Fill(pixel.get()->column(), pixel.get()->row());
            hPixelToT->Fill(pixel.get()->raw());
            // display the 10 bit timestamp distribution
            hTimeStamp->Fill(fmod((pixel.get()->timestamp() / 8.), pow(2, 10)));
            pixelbuffer_.pop();
        } else {
            break;
        }
    }
    if(pixelbuffer_.size() < buffer_depth_)
        fillBuffer();
    // Return value telling analysis to keep running
    if(pixelbuffer_.size() == 0)
        return StatusCode::NoData;
    prev_event_end_ = clipboard->getEvent()->end();
    return StatusCode::Success;
}

void EventLoaderMuPixTelescope::fillBuffer() {

    // here we need to check quite a number of cases
    while(pixelbuffer_.size() < buffer_depth_) {
        if(blockFile_->read_next(tf_)) {
            // no hits in data - can only happen if the zero suppression is switched off, skip the event
            if(tf_.num_hits() == 0) {
                continue;
            }
            // need to determine the sensor layer that is identified by the tag
            RawHit h = tf_.get_hit(0);
            // tag does not match - continue reading if data is not sorted
            if(((h.tag() & uint(~0x3)) != tag_)) {
                continue;
            }
            // all hits in one frame are from the same sensor. Copy them
            for(uint i = 0; i < tf_.num_hits(); ++i) {
                h = tf_.get_hit(i, type_);
                // move ts to ns - i'd like to do this already on the mupix8_DAQ side, but have not found the time yet,
                // assuming 10bit ts
                double px_timestamp =
                    8 * static_cast<double>(((tf_.timestamp() >> 2) & 0xFFFFFFFFFFC00) + h.timestamp_raw()) - timeOffset_;
                LOG(TRACE) << "Pixel timestamp " << px_timestamp;
                // setting tot and charge to zero here - needs to be improved
                pixelbuffer_.push(std::make_shared<Pixel>(detector_->getName(), h.column(), h.row(), 0, 0, px_timestamp));
            }
        } else {
            eof_ = true;
            break;
        }
    }
}
std::map<std::string, int> EventLoaderMuPixTelescope::typeString_to_typeID = {{"mupix8", MP8_SORTED_TS2},
                                                                              {"mupix9", MP10_SORTED_TS2},
                                                                              {"mupix10", MP10_UNSORTED_GS1_GS2},
                                                                              {"run2020v1", R20V1_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v2", R20V2_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v3", R20V3_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v4", R20V4_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v5", R20V5_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v6", R20V6_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v7", R20V7_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v8", R20V8_UNSORTED_GS1_GS2_GS3},
                                                                              {"run2020v9", R20V9_UNSORTED_GS1_GS2_GS3}};
