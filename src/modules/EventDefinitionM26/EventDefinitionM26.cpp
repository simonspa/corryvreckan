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

    config_.setAlias("file_duration", "file_m26", true);

    config_.setDefault<int>("time_shift", 0);
    config_.setDefault<int>("shift_triggers", 0);
    config_.setDefault<double>("skip_time", 0.);
    config_.setDefault<double>("add_begin", 0.);
    config_.setDefault<double>("add_end", 0.);
    config_.setDefault<int>("plane_pivot", 0.);
    config_.setDefault<int>("pivot_min", 0.);
    config_.setDefault<int>("pivot_max", 576.);
    config_.setDefault<bool>("add_trigger", false);
    config_.setDefault<bool>("use_all_mimosa_hits", false);
    config_.setDefault<std::string>("eudaq_loglevel", "ERROR");
    config_.setDefault<bool>("pixelated_timing_layer", true);
    detector_time_ = config_.get<std::string>("detector_event_time");
    // Convert to lower case before string comparison to avoid errors by the user:
    std::transform(detector_time_.begin(), detector_time_.end(), detector_time_.begin(), ::tolower);

    timestamp_ = config_.getPath("file_timestamp");
    duration_ = config_.getPath("file_duration");
    timeshift_ = config_.get<double>("time_shift");
    shift_triggers_ = config_.get<int>("shift_triggers");
    skip_time_ = config_.get<double>("skip_time");
    add_begin_ = config_.get<double>("add_begin");
    add_end_ = config_.get<double>("add_end");
    pivot_min_ = config_.get<int>("pivot_min");
    pivot_max_ = config_.get<int>("pivot_max");
    use_all_mimosa_hits_ = config_.get<bool>("use_all_mimosa_hits");
    pixelated_timing_layer_ = config_.get<bool>("pixelated_timing_layer");
    LOG(WARNING) << " Setting shift to " << Units::get(timeshift_, "us") << ": accepting pivots from  " << pivot_min_
                 << " to " << pivot_max_;
    add_trigger_ = config_.get<bool>("add_trigger");
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

    // Shift trigger ID of this device with respect to the IDs stored in the Corryrveckan Event
    // Note: Require shift_triggers >= 0
    if(shift_triggers_ < 0) {
        throw InvalidValueError(config_, "shift_triggers", "Trigger shift needs to be positive (or zero).");
    }
    // define the framelength once, since unit conversions are slow
    framelength_ = Units::get(115.2, "us");
    ;
}

void EventDefinitionM26::initialize() {
    timebetweenMimosaEvents_ =
        new TH1F("htimebetweenTimes", "time between two mimosa frames; time /us; #entries", 1000, -0.5, 995.5);
    timebetweenTLUEvents_ =
        new TH1F("htimebetweenTrigger", "time between two triggers frames; time /us; #entries", 1000, -0.5, 995.5);
    eventDuration_ =
        new TH1F("durationCorryEvent", "Event duration as defined on clipboard; time [#mus]; #entries", 240, 115.2, 360.4);

    timeBeforeTrigger_ = new TH1F("timeBeforeTrigger", "time in frame before trigger; time /us; #entries", 2320, -231, 1);
    timeAfterTrigger_ = new TH1F("timeAfterTrigger", "time in frame after trigger; time /us; #entries", 2320, -1, 231);

    std::string title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [ms];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [s];# entries";
    hClipboardEventStart_long = new TH1D("clipboardEventStart_long", title.c_str(), 3e6, 0, 3e3);

    pivotPixel_ = new TH1F("pivot_pixel", "pivot pixel; pivot; entries", 580, -.5, 579.5);
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

void EventDefinitionM26::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    LOG(INFO) << "We have to skip " << skipped_events_ << "events which have an overlap with the previous events.";
}

unsigned EventDefinitionM26::get_next_event_with_det(const eudaq::FileReaderUP& filereader,
                                                     const std::string& det,
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
            // Convert to lower case before string comparison to avoid errors by the user:
            std::transform(detector.begin(), detector.end(), detector.begin(), ::tolower);

            LOG(DEBUG) << "det = " << det << ", detector = " << detector;
            if(det == detector) {
                // MIMOSA
                begin = Units::get(static_cast<double>(stdevt->GetTimeBegin()), "ps");
                end = Units::get(static_cast<double>(stdevt->GetTimeEnd()), "ps");

                if(det == "mimosa26") {
                    // pivot magic - see readme
                    double piv = stdevt->GetPlane(0).PivotPixel() / 16.;
                    // we can here discard events with `bad` pivot-pixels
                    if(piv > pivot_max_ || piv < pivot_min_) {
                        LOG(DEBUG) << "Skipping mimosa event with pivot " << piv;
                        continue;
                    }
                    pivotPixel_->Fill(piv);
                    begin = framelength_ * piv / 576.;

                    // end should be after second frame, sharp (variable durationn, not variable length)
                    end = Units::get(230.4, "us");
                    LOG(DEBUG) << "Pivot magic, begin: " << Units::display(begin, {"ns", "us", "ms"})
                               << ", end: " << Units::display(end, {"ns", "us", "ms"})
                               << ", duration = " << Units::display(begin + end, {"ns", "us"});
                } else if(det == "tlu" && begin < skip_time_) {

                    LOG(TRACE) << "Event time below skip time: " << Units::display(begin, {"ns", "us", "ms", "s"}) << "vs. "
                               << Units::display(skip_time_, {"ns", "us", "ms", "s"});

                    continue;
                } else {

                    LOG(DEBUG) << "Set begin/end, begin: " << Units::display(begin, {"ns", "us"})
                               << ", end: " << Units::display(end, {"ns", "us"});
                }
                return e->GetTriggerN();
            }
        }

    } while(true);
}
StatusCode EventDefinitionM26::run(const std::shared_ptr<Clipboard>& clipboard) {

    if(clipboard->isEventDefined()) {
        throw ModuleError("Event already defined - cannot create a new event. This module needs to be placed before the "
                          "first EventLoader");
    }
    // read events until we have a common tag:
    try {
        triggerTLU_ = get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_);
        timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
        trig_prev_ = time_trig_start_;
        triggerM26_ = static_cast<unsigned>(
            static_cast<int>(get_next_event_with_det(readerDuration_, "mimosa26", time_before_, time_after_)) +
            shift_triggers_);
    } catch(EndOfFile&) {
        return StatusCode::EndRun;
    }

    do {
        try {
            if(triggerTLU_ < triggerM26_) {
                LOG(DEBUG) << "TLU trigger smaller than Mimosa26 trigger, get next TLU trigger";
                triggerTLU_ = get_next_event_with_det(readerTime_, detector_time_, time_trig_start_, time_trig_stop_);
                timebetweenTLUEvents_->Fill(static_cast<double>(Units::convert(time_trig_start_ - trig_prev_, "us")));
                trig_prev_ = time_trig_start_;
            } else if(triggerTLU_ > triggerM26_) {
                LOG(DEBUG) << "Mimosa26 trigger smaller than TLU trigger, get next Mimosa26 trigger";
                triggerM26_ = static_cast<unsigned>(
                    static_cast<int>(get_next_event_with_det(readerDuration_, "mimosa26", time_before_, time_after_)) +
                    shift_triggers_);
            }

        } catch(EndOfFile&) {
            return StatusCode::EndRun;
        }

        LOG(DEBUG) << "TLU trigger defining event: " << triggerTLU_ << std::endl
                   << "Mimosa26 trigger defining event: " << triggerM26_;

        if(triggerTLU_ == triggerM26_) {
            auto time_trig = time_trig_start_;
            // we only need to apply the shifts once we have a matching event:
            // Artificially enlarge the events if a timing layer is provided
            if(pixelated_timing_layer_) {
                time_before_ += add_begin_;
                time_after_ -= add_begin_;
                time_after_ += add_end_;
                // If no timing layer is configured, we need to stretch over all 3 frames
            } else if(use_all_mimosa_hits_) {
                time_before_ += framelength_;
            } else {
                time_before_ = framelength_;
            }
            time_trig = time_trig - timeshift_;

            // Define the begin and end of the event
            long double evtBegin = time_trig - time_before_;
            long double evtEnd = time_trig + time_after_;

            // Skip events while requested
            if(evtBegin < skip_time_) {
                LOG(DEBUG) << "Event start before requested skip time: " << Units::display(evtBegin, {"us", "ns"}) << " < "
                           << Units::display(skip_time_, {"us", "ns"});
                triggerTLU_--;
                continue;
            }

            // An earlier trigger ts points towards an error during data taking. Skip this event.
            if(time_trig - time_prev_ < 0) {
                LOG(ERROR) << "Current trigger time smaller than previous: " << time_trig << " vs " << time_prev_
                           << ". Skipping Event";
                triggerTLU_--;
                continue;
            }
            // M26 frames should never overlap. A bug in the DAQ causes a readout of the wrong frames occasionally
            if(evtBegin < time_trig_stop_prev_) {
                triggerTLU_--;
                LOG(DEBUG) << "Skipping event that would overlap previous event: start current: " << evtBegin
                           << " vs end previous: " << time_trig_stop_prev_;
                skipped_events_++;
                continue;
            }

            timebetweenMimosaEvents_->Fill(static_cast<double>(Units::convert(time_trig - time_prev_, "us")));
            timeBeforeTrigger_->Fill(static_cast<double>(Units::convert(-1.0 * time_before_, "us")));
            timeAfterTrigger_->Fill(static_cast<double>(Units::convert(time_after_, "us")));

            LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(evtBegin, {"us", "ns"}) << " - "
                       << Units::display(evtEnd, {"us", "ns"}) << ", length "
                       << Units::display(evtEnd - evtBegin, {"us", "ns"});

            LOG(DEBUG) << "time to previous trigger = " << Units::display(time_trig - time_prev_, "us");
            time_prev_ = time_trig;
            LOG(DEBUG) << "before/after/duration = " << Units::display(time_before_, "us") << ", "
                       << Units::display(time_after_, "us") << ", " << Units::display(time_after_ + time_before_, "us");
            LOG(DEBUG) << "evtBegin/evtEnd/duration = " << Units::display(evtBegin, "us") << ", "
                       << Units::display(evtEnd, "us") << ", " << Units::display(evtEnd - evtBegin, "us");

            time_trig_stop_prev_ = evtEnd;
            auto event = std::make_shared<Event>(evtBegin, evtEnd);
            clipboard->putEvent(event);

            if(add_trigger_) {
                clipboard->getEvent()->addTrigger(triggerTLU_, static_cast<double>(time_trig));
            }
            eventDuration_->Fill(static_cast<double>(Units::convert(event->duration(), "us")));
            hClipboardEventStart->Fill(static_cast<double>(Units::convert(evtBegin, "ms")));
            hClipboardEventStart_long->Fill(static_cast<double>(Units::convert(evtBegin, "s")));

        } else if(triggerTLU_ > triggerM26_) {
            LOG(DEBUG) << "No TLU time stamp for trigger ID " << triggerTLU_;
        } else if(triggerTLU_ < triggerM26_) {
            LOG(DEBUG) << "No Mimosa data for trigger ID " << triggerM26_;
        }

    } while(!clipboard->isEventDefined());
    // Return value telling analysis to keep running
    return StatusCode::Success;
}
