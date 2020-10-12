/**
 * @file
 * @brief Implementation of module EventDefinitionM26
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

    config_.setDefault<int>("time_shift", 0);
    config_.setDefault<int>("shift_triggers", 0);
    config_.setDefault<std::string>("eudaq_loglevel", "ERROR");
    config_.setDefault<double>("response_time_m26", Units::get<double>(0, "us"));
    config_.setDefault<double>("skip_time", 0.);

    detector_time_ = config_.get<std::string>("detector_event_time");
    detector_duration_ = config_.get<std::string>("detector_event_duration");
    duration_ = config_.get<std::string>("file_duration");
    timestamp_ = config_.get<std::string>("file_timestamp");
    timeshift_ = config_.get<double>("time_shift");
    shift_triggers_ = config_.get<int>("shift_triggers");
    response_time_m26_ = config_.get<double>("response_time_m26");
    skip_time_ = config_.get<double>("skip_time");

    config_.setDefault<std::string>("eudaq_loglevel", "ERROR");

    // Set EUDAQ log level to desired value:
    EUDAQ_LOG_LEVEL(config_.get<std::string>("eudaq_loglevel"));
    LOG(INFO) << "Setting EUDAQ2 log level to \"" << config_.get<std::string>("eudaq_loglevel") << "\"";

    // Prepare EUDAQ2 config object
    eudaq::Configuration cfg;

    // Forward all settings to EUDAQ
    // WARNING: the EUDAQ Configuration class is not very flexible and e.g. booleans have to be passed as 1 and 0.
    auto configs = config_.getAll();
    for(const auto& key : configs) {
        LOG(DEBUG) << "Forwarding key \"" << key.first << " = " << key.second << "\" to EUDAQ converter";
        cfg.Set(key.first, key.second);
    }

    // Converting the newly built configuration to a shared pointer of a const configuration object
    // Unfortunately, EUDAQ does not provide appropriate member functions for their configuration class to avoid this dance
    const eudaq::Configuration eu_cfg = cfg;
    eudaq_config_ = std::make_shared<const eudaq::Configuration>(eu_cfg);
}

void EventDefinitionM26::initialize() {
    timebetweenMimosaEvents_ =
        new TH1F("htimebetweenTimes", "time between two mimosa frames; time /us; #entries", 1000, -0.5, 995.5);
    timebetweenTLUEvents_ =
        new TH1F("htimebetweenTrigger", "time between two triggers frames; time /us; #entries", 1000, -0.5, 995.5);

    timeBeforeTrigger_ = new TH1F("timeBeforeTrigger", "time in frame before trigger; time /us; #entries", 2320, -231, 1);
    timeAfterTrigger_ = new TH1F("timeAfterTrigger", "time in frame after trigger; time /us; #entries", 2320, -1, 231);

    std::string title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [ms];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [s];# entries";
    hClipboardEventStart_long = new TH1D("clipboardEventStart_long", title.c_str(), 3e6, 0, 3e3);

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
}

unsigned EventDefinitionM26::get_next_event_with_det(eudaq::FileReaderUP& filereader,
                                                     std::string& det,
                                                     long double& begin,
                                                     long double& end) {
    do {
        LOG(DEBUG) << "Get next event.";
        auto evt = filereader->GetNextEvent();
        if(!evt) {
            LOG(DEBUG) << "Reached end-of-file.";
            throw EndOfFile();
        }
        LOG(DEBUG) << "Get subevents.";
        std::vector<eudaq::EventSPC> events_ = evt->GetSubEvents();
        if(events_.empty()) {
            events_.push_back(evt);
        }
        for(const auto& e : events_) {
            auto stdevt = eudaq::StandardEvent::MakeShared();
            if(!eudaq::StdEventConverter::Convert(e, stdevt, eudaq_config_)) {
                continue;
            }

            auto detector = stdevt->GetDetectorType();
            LOG(DEBUG) << "det = " << det << ", detector = " << detector;
            if(det == detector) {
                begin = Units::get(static_cast<double>(stdevt->GetTimeBegin()), "ps");
                end = Units::get(static_cast<double>(stdevt->GetTimeEnd()), "ps");

                LOG(DEBUG) << "Set begin/end, begin: " << Units::display(begin, {"ns", "us"})
                           << ", end: " << Units::display(end, {"ns", "us"});
                // MIMOSA
                if(det == "MIMOSA26") {
                    // pivot magic - see readme
                    double piv = stdevt->GetPlane(0).PivotPixel() / 16.;
                    begin = Units::get(piv * (115.2 / 576), "us") + timeshift_;
                    end = Units::get(230.4, "us") - begin;
                    LOG(DEBUG) << "Pivot magic, begin: " << Units::display(begin, {"ns", "us", "ms"})
                               << ", end: " << Units::display(end, {"ns", "us", "ms"})
                               << ", duration = " << Units::display(begin + end, {"ns", "us"});
                }
                return e->GetTriggerN();
            }
        }

    } while(true);
}
StatusCode EventDefinitionM26::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Loop over all detectors
    if(clipboard->isEventDefined()) {
        throw ModuleError("Event already defined - cannot create a new event. This module needs to be placed before the "
                          "first EventLoader");
    }
    // read events until we have a common tag:
    do {
        try {
            triggerTLU_ = static_cast<unsigned>(
                static_cast<int>(get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_)) +
                shift_triggers_);
            timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
            trig_prev_ = time_trig_start_;
            triggerM26_ = get_next_event_with_det(readerDuration_, detector_duration_, time_before_, time_after_);

            if(triggerTLU_ < triggerM26_) {
                LOG(DEBUG) << "TLU trigger smaller than Mimosa26 trigger, get next TLU trigger";
                triggerTLU_ = static_cast<unsigned>(static_cast<int>(get_next_event_with_det(
                                                        readerTime_, detector_time_, time_trig_start_, time_trig_stop_)) +
                                                    shift_triggers_);
                timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
                trig_prev_ = time_trig_start_;
            } else if(triggerTLU_ > triggerM26_) {
                LOG(DEBUG) << "Mimosa26 trigger smaller than TLU trigger, get next Mimosa26 trigger";
                triggerM26_ = get_next_event_with_det(readerDuration_, detector_duration_, time_before_, time_after_);
            }

        } catch(EndOfFile&) {
            return StatusCode::EndRun;
        }

        LOG(DEBUG) << "TLU trigger defining event: " << triggerTLU_ << std::endl
                   << "Mimosa26 trigger defining event: " << triggerM26_;

        if(triggerTLU_ == triggerM26_) {
            auto time_trig = time_trig_start_ - response_time_m26_;
            if(time_trig - time_prev_ > 0) {
                // if(time_trig_start_ - time_trig_stop_prev_ > 0) {
                timebetweenMimosaEvents_->Fill(static_cast<double>(Units::convert(time_trig - time_prev_, "us")));
                timeBeforeTrigger_->Fill(static_cast<double>(Units::convert(-1.0 * time_before_, "us")));
                timeAfterTrigger_->Fill(static_cast<double>(Units::convert(time_after_, "us")));
                long double evtStart = time_trig - time_before_;
                long double evtEnd = time_trig + time_after_;

                if(evtStart < skip_time_) {
                    LOG(ERROR) << "Event start before requested skip time: " << Units::display(evtStart, {"us", "ns"})
                               << " < " << Units::display(skip_time_, {"us", "ns"});
                    continue;
                }

                LOG(DEBUG) << "time to previous trigger = " << Units::display(time_trig - time_prev_, "us");
                time_prev_ = time_trig;
                LOG(DEBUG) << "before/after/duration = " << Units::display(time_before_, "us") << ", "
                           << Units::display(time_after_, "us") << ", " << Units::display(time_after_ + time_before_, "us");
                LOG(DEBUG) << "evtStart/evtEnd/duration = " << Units::display(evtStart, "us") << ", "
                           << Units::display(evtEnd, "us") << ", " << Units::display(evtEnd - evtStart, "us");
                clipboard->putEvent(std::make_shared<Event>(evtStart, evtEnd));
                LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(evtStart, {"us", "ns"}) << " - "
                           << Units::display(evtEnd, {"us", "ns"}) << ", length "
                           << Units::display(evtEnd - evtStart, {"us", "ns"});
                hClipboardEventStart->Fill(static_cast<double>(Units::convert(evtStart, "ms")));
                hClipboardEventStart_long->Fill(static_cast<double>(Units::convert(evtStart, "s")));
            } else {
                LOG(WARNING) << "Current trigger time smaller than previous: " << time_trig << " vs " << time_prev_;
            }

        } else if(triggerTLU_ > triggerM26_) {
            LOG(DEBUG) << "No TLU time stamp for trigger ID " << triggerTLU_;
        } else if(triggerTLU_ < triggerM26_) {
            LOG(DEBUG) << "No Mimosa data for trigger ID " << triggerM26_;
        }

    } while(!clipboard->isEventDefined());
    // Return value telling analysis to keep running
    return StatusCode::Success;
}
