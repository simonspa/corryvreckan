/**
 * @file
 * @brief Implementation of module EventCreatorEUDAQ2
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventDefinitionM26.h"

using namespace corryvreckan;

EventDefinitionM26::EventDefinitionM26(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {
    detector_time_ = config_.get<std::string>("detector_event_time");
    detector_duration_ = config_.get<std::string>("detector_event_duration");
    duration_ = config_.get<std::string>("file_duration");
    timestamp_ = config_.get<std::string>("file_timestamp");
    timeshift_ = config_.get<double>("time_shift");
    shift_triggers_ = config_.get<int>("shift_triggers");
}

void EventDefinitionM26::initialize() {
    timebetweenMimosaEvents_ =
        new TH1F("htimebetweenTimes", "time between two mimosa frames; time /us; #entries", 1000, -0.5, 995.5);
    timebetweenTLUEvents_ =
        new TH1F("htimebetweenTrigger", "time between two triggers frames; time /us; #entries", 1000, -0.5, 995.5);

    // open the input file with the eudaq reader
    try {
        readerDuration_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), duration_);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << duration_
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(config_, "file_path", "Parsing error!");
    }
    try {
        readerTime_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), timestamp_);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << timestamp_
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(config_, "file_path", "Parsing error!");
    }
    // get the first event each
    timestampTrig_ =
        get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_) + shift_triggers_;
    durationTrig_ = get_next_event_with_det(readerDuration_, detector_duration_, time_before_, time_after_);
}

unsigned EventDefinitionM26::get_next_event_with_det(eudaq::FileReaderUP& filereader,
                                                     std::string& det,
                                                     long double& begin,
                                                     long double& end) {
    do {
        auto evt = filereader->GetNextEvent();
        if(evt == nullptr) {
            throw EndOfFile();
        }
        std::vector<eudaq::EventSPC> events_ = evt->GetSubEvents();
        if(events_.empty()) {
            events_.push_back(evt);
        }
        for(const auto& e : events_) {
            auto stdevt = eudaq::StandardEvent::MakeShared();
            if(!eudaq::StdEventConverter::Convert(e, stdevt, nullptr)) {
                LOG(ERROR) << "Failed to convert event";
                continue;
            }
            auto detector = stdevt->GetDetectorType();
            if(det == detector) {
                begin = Units::get(static_cast<double>(stdevt->GetTimeBegin()), "ps");
                end = Units::get(static_cast<double>(stdevt->GetTimeEnd()), "ps");
                // MIMOSA
                if(det == "MIMOSA26") {
                    // pivot magic - see readme
                    double piv = stdevt->GetPlane(0).PivotPixel() / 16.;
                    begin = Units::get(piv * (115.2 / 576), "us") + timeshift_;
                    end = Units::get(230.4, "us") - begin;
                }
                return static_cast<int>(e->GetTriggerN());
            }
        }

    } while(true);
}
StatusCode EventDefinitionM26::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all detectors
    if(clipboard->isEventDefined()) {
        throw ModuleError(
            "Event already defined - cannot crate a new event. This module needs to be placed before the first EventLoader");
    }
    // read events until we have a common tag:
    do {
        LOG(DEBUG) << "Trigger of timestamp defining event: " << timestampTrig_ << std::endl
                   << " Trigger of duration defining event: " << durationTrig_;
        if(timestampTrig_ < durationTrig_) {
            timestampTrig_ =
                get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_) + shift_triggers_;
            timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
            trig_prev_ = time_trig_start_;
        } else if(timestampTrig_ > durationTrig_) {
            durationTrig_ = get_next_event_with_det(readerDuration_, detector_duration_, time_before_, time_after_);
        }
        if(timestampTrig_ == durationTrig_) {
            auto time_trig = (time_trig_start_ + time_trig_stop_) / 2.;
            if(time_trig - time_prev_ > 0) {
                timebetweenMimosaEvents_->Fill(static_cast<double>(Units::convert(time_trig - time_prev_, "us")));
                time_prev_ = time_trig;
                long double evtstart = time_trig - time_before_;
                long double evtEnd = time_trig + time_after_;
                clipboard->putEvent(std::make_shared<Event>(evtstart, evtEnd));
                LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(evtstart, {"us", "ns"}) << " - "
                           << Units::display(evtEnd, {"us", "ns"}) << ", length "
                           << Units::display(evtEnd - evtstart, {"us", "ns"});
            } else {
                LOG(WARNING) << "Current trigger time smaller than previous: " << time_trig << " vs " << time_prev_;
            }
            timestampTrig_ =
                get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_) + shift_triggers_;
            timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
            trig_prev_ = time_trig_start_;
            durationTrig_ = get_next_event_with_det(readerDuration_, detector_duration_, time_before_, time_after_);

        } else if(timestampTrig_ > durationTrig_) {
            LOG(DEBUG) << "No TLU time stamp for trigger ID " << timestampTrig_;
        } else if(timestampTrig_ < durationTrig_) {
            LOG(DEBUG) << "No Mimosa data for trigger ID " << durationTrig_;
        }

    } while(!clipboard->isEventDefined());
    // Return value telling analysis to keep running
    return StatusCode::Success;
}
