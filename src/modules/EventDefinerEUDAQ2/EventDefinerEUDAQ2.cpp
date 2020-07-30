/**
 * @file
 * @brief Implementation of module EventCreatorEUDAQ2
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventDefinerEUDAQ2.h"

using namespace corryvreckan;

EventDefinerEUDAQ2::EventDefinerEUDAQ2(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {
    detector_time_ = config_.get<std::string>("detector_event_time");
    detector_duration_ = config_.get<std::string>("detector_event_duration");
    duration_ = config_.get<std::string>("file_duration");
    timestamp_ = config_.get<std::string>("file_timestamp");
    timeshift_ = config_.get<double>("time_shift");
    shift_triggers_ = config_.get<int>("shift_triggers");
    // Prepare EUDAQ2 config object - copy from the EventLoader
    eudaq::Configuration cfg;
    // WARNING: the EUDAQ Configuration class is not very flexible and e.g. booleans have to be passed as 1 and 0.
    auto configs = config_.getAll();
    for(const auto& key : configs) {
        LOG(DEBUG) << "Forwarding key \"" << key.first << " = " << key.second << "\" to EUDAQ converter";
        cfg.Set(key.first, key.second);
    }
    // Converting the newly built configuration to a shared pointer of a const configuration object
    // Unfortunbately EUDAQ does not provide appropriate member functions for their configuration class to avoid this dance
    const eudaq::Configuration eu_cfg = cfg;
    eudaq_config_ = std::make_shared<const eudaq::Configuration>(eu_cfg);
}

void EventDefinerEUDAQ2::initialize() {

    std::vector<std::string> det;
    for(auto& detector : get_detectors()) {
        det.push_back(detector->getName());
    }

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

    timestampTrig_ = -1;
    durationTrig_ = 0;
    triggerIDs_.resize(2, 0);
    // Initialise member variables
    eventNumber_ = 0;
    time_prev_ = 0;
    trig_prev_ = 0;
}

int EventDefinerEUDAQ2::get_next_event_with_det(eudaq::FileReaderUP& filereader,
                                                std::string& det,
                                                long double& begin,
                                                long double& end) {
    do {
        auto evt = filereader->GetNextEvent();
        if(evt == nullptr)
            return -5;
        std::vector<eudaq::EventSPC> events_ = evt->GetSubEvents();
        if(events_.empty())
            events_.push_back(evt);
        for(auto e : events_) {
            auto stdevt = eudaq::StandardEvent::MakeShared();
            if(!eudaq::StdEventConverter::Convert(e, stdevt, eudaq_config_)) {
                LOG(ERROR) << "Failed to convert event";
                continue;
            }
            auto detector = stdevt->GetDetectorType();
            if(det == detector) {
                begin = static_cast<double>(stdevt->GetTimeBegin());
                begin = Units::get(begin, "ps");
                end = static_cast<double>(stdevt->GetTimeEnd());
                end = Units::get(end, "ps");
                // MIMOSA
                if(det == "MIMOSA26") {
                    double piv = stdevt->GetPlane(0).PivotPixel() / 16.;
                    begin =
                        piv * (115.2 / 576) + timeshift_; // 100 seems to be the delay between trigger and good to go sign
                    end = 230.4 - begin;
                    begin = Units::get(begin, "us");
                    end = Units::get(end, "us");
                }
                return static_cast<int>(e->GetTriggerN());
            }
        }

    } while(true);
}
StatusCode EventDefinerEUDAQ2::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all detectors
    if(clipboard->isEventDefined()) {
        LOG(ERROR) << "Event already defined - cannot crate a new event";
    }
    // read events until we have a common tag:
    do {
        LOG(DEBUG) << "Trigger of timestamp defining event: " << timestampTrig_
                   << "\t Trigger of duration defining event: " << durationTrig_;
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
                LOG(WARNING) << "inverted time structure";
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

    // Increment event counter
    eventNumber_++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventDefinerEUDAQ2::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << eventNumber_ << " events";
}
