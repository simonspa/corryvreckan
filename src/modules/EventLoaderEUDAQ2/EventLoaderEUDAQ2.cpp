/**
 * @file
 * @brief Implementation of module EventLoaderEUDAQ2
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderEUDAQ2.h"
#include "eudaq/FileReader.hh"

#include "objects/Waveform.hpp"

#include <TDirectory.h>

using namespace corryvreckan;

EventLoaderEUDAQ2::EventLoaderEUDAQ2(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(detector) {

    // Backwards compatibility: also allow get_tag_vectors to be used for get_tag_profiles
    config_.setAlias("get_tag_profiles", "get_tag_vectors", true);

    config_.setDefault<bool>("get_time_residuals", false);
    config_.setDefault<bool>("get_tag_histograms", false);
    config_.setDefault<bool>("get_tag_profiles", false);
    config_.setDefault<bool>("ignore_bore", true);
    config_.setDefault<bool>("veto_triggers", false);
    config_.setDefault<bool>("sync_by_trigger", false);
    config_.setDefault<double>("skip_time", 0.);
    config_.setDefault<int>("buffer_depth", 0);
    config_.setDefault<int>("shift_triggers", 0);
    config_.setDefault<bool>("inclusive", true);
    config_.setDefault<std::string>("eudaq_loglevel", "ERROR");

    filename_ = config_.getPath("file_name", true);
    get_time_residuals_ = config_.get<bool>("get_time_residuals");

    get_tag_histograms_ = config_.get<bool>("get_tag_histograms");
    get_tag_profiles_ = config_.get<bool>("get_tag_profiles");
    ignore_bore_ = config_.get<bool>("ignore_bore");
    veto_triggers_ = config_.get<bool>("veto_triggers");
    skip_time_ = config_.get<double>("skip_time");
    adjust_event_times_ = config_.getMatrix<std::string>("adjust_event_times", {});
    buffer_depth_ = config_.get<int>("buffer_depth");
    shift_triggers_ = config_.get<int>("shift_triggers");
    inclusive_ = config_.get<bool>("inclusive");
    sync_by_trigger_ = config_.get<bool>("sync_by_trigger");

    // Set EUDAQ log level to desired value:
    EUDAQ_LOG_LEVEL(config_.get<std::string>("eudaq_loglevel"));
    LOG(INFO) << "Setting EUDAQ2 log level to \"" << config_.get<std::string>("eudaq_loglevel") << "\"";

    // Prepare EUDAQ2 config object
    eudaq::Configuration cfg;

    // Forward all settings to EUDAQ
    // WARNING: the EUDAQ Configuration class is not very flexible and e.g. booleans have to be passed as 1 and 0.
    auto configs = config_.getAll(true);
    for(const auto& key : configs) {
        LOG(DEBUG) << "Forwarding key \"" << key.first << " = " << key.second << "\" to EUDAQ converter";
        cfg.Set(key.first, key.second);
    }

    // Provide the calibration file specified in the detector geometry:
    // NOTE: This should go last to allow overwriting the calibration_file key in the module config with absolute path
    auto calibration_file =
        config_.has("calibration_file") ? config_.getPath("calibration_file", true) : detector_->calibrationFile();
    if(!calibration_file.empty()) {
        LOG(DEBUG) << "Forwarding detector calibration file: " << calibration_file;
        cfg.Set("calibration_file", calibration_file.string());
    }

    // Converting the newly built configuration to a shared pointer of a const configuration object
    // Unfortunately EUDAQ does not provide appropriate member functions for their configuration class to avoid this dance
    const eudaq::Configuration eu_cfg = cfg;
    eudaq_config_ = std::make_shared<const eudaq::Configuration>(eu_cfg);

    // Shift trigger ID of this device with respect to the IDs stored in the Corryrveckan Event
    // Note: Require shift_triggers >= 0
    if(shift_triggers_ < 0) {
        throw InvalidValueError(config_, "shift_triggers", "Trigger shift needs to be positive (or zero).");
    }
}

void EventLoaderEUDAQ2::initialize() {

    // Declare histograms
    std::string title = ";EUDAQ event start time[ms];# entries";
    hEudaqEventStart = new TH1D("eudaqEventStart", title.c_str(), 3e6, 0, 3e3);

    title = ";EUDAQ event start time[s];# entries";
    hEudaqEventStart_long = new TH1D("eudaqEventStart_long", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [ms];# entries";
    hClipboardEventStart = new TH1D("clipboardEventStart", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event start times (placed on clipboard); Corryvreckan event start time [s];# entries";
    hClipboardEventStart_long = new TH1D("clipboardEventStart_long", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event end times (placed on clipboard); Corryvreckan event end time [ms];# entries";
    hClipboardEventEnd = new TH1D("clipboardEventEnd", title.c_str(), 3e6, 0, 3e3);

    title = "Corryvreckan event end times (on clipboard); Corryvreckan event duration [ms];# entries";
    hClipboardEventDuration = new TH1D("clipboardEventDuration", title.c_str(), 3e6, 0, 3e3);

    hTriggersPerEvent =
        new TH1D("hTriggersPerEvent", "Number of triggers per event;triggers per event;entries", 20, -0.5, 19.5);

    title = " # events per corry event; number of events from " + detector_->getName() + " per corry event;# entries";
    hEudaqeventsPerCorry = new TH1D("hEudaqeventsPerCorryEvent", title.c_str(), 50, -.5, 49.5);

    title = "number of hits in corry frame vs number of eudaq frames;eudaq frames;# hits";
    hHitsVersusEUDAQ2Frames = new TH2D("hHitsVersusEUDAQ2Frames", title.c_str(), 15, -.5, 14.5, 200, -0.5, 199.5);
    // Create the following histograms only when detector is not auxiliary:
    if(!detector_->isAuxiliary()) {
        title = "hitmap;column;row;# events";
        hitmap = new TH2F("hitmap",
                          title.c_str(),
                          detector_->nPixels().X(),
                          -0.5,
                          detector_->nPixels().X() - 0.5,
                          detector_->nPixels().Y(),
                          -0.5,
                          detector_->nPixels().Y() - 0.5);

        title = "rawValues; column; row; raw values";
        hRawValuesMap = new TProfile2D("hRawValuesMap",
                                       title.c_str(),
                                       detector_->nPixels().X(),
                                       -0.5,
                                       detector_->nPixels().X() - 0.5,
                                       detector_->nPixels().Y(),
                                       -0.5,
                                       detector_->nPixels().Y() - 0.5);

        title = ";hit time [ms];# events";
        hPixelTimes = new TH1F("hPixelTimes", title.c_str(), 3e6, 0, 3e3);

        title = ";hit timestamp [s];# events";
        hPixelTimes_long = new TH1F("hPixelTimes_long", title.c_str(), 3e6, 0, 3e3);

        title = ";pixel raw values;# events";
        hPixelRawValues = new TH1F("hPixelRawValues", title.c_str(), 1024, -0.5, 1023.5);

        title = "Pixel Multiplicity per EUDAQ Event;# pixels;# events";
        hPixelMultiplicityPerEudaqEvent = new TH1F("hPixelMultiplicityPerEudaqEvent", title.c_str(), 1000, -0.5, 999.5);

        title = "Pixel Multiplicity per Corry Event;# pixels;# events";
        hPixelMultiplicityPerCorryEvent = new TH1F("hPixelMultiplicityPerCorryEvent", title.c_str(), 1000, -0.5, 999.5);

        if(get_time_residuals_) {
            hPixelTimeEventBeginResidual =
                new TH1F("hPixelTimeEventBeginResidual",
                         "hPixelTimeEventBeginResidual;pixel_ts - clipboard event begin [us]; # entries",
                         2.1e5,
                         -10,
                         200);

            hPixelTimeEventBeginResidual_wide =
                new TH1F("hPixelTimeEventBeginResidual_wide",
                         "hPixelTimeEventBeginResidual_wide;pixel_ts - clipboard event begin [us]; # entries",
                         1e5,
                         -5000,
                         5000);
            hPixelTimeEventBeginResidualOverTime =
                new TH2F("hPixelTimeEventBeginResidualOverTime",
                         "hPixelTimeEventBeginResidualOverTime; pixel time [s];pixel_ts - clipboard event begin [us]",
                         3e3,
                         0,
                         3e3,
                         2.1e4,
                         -10,
                         200);
            std::string histTitle = "hPixelTriggerTimeResidualOverTime_0;time [s];pixel_ts - trigger_ts [us];# entries";
            hPixelTriggerTimeResidualOverTime =
                new TH2D("hPixelTriggerTimeResidualOverTime_0", histTitle.c_str(), 3e3, 0, 3e3, 1e4, -50, 50);
        }
    }

    // open the input file with the eudaq reader
    try {
        reader_ = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), filename_);
    } catch(...) {
        LOG(ERROR) << "eudaq::FileReader could not read the input file ' " << filename_
                   << " '. Please verify that the path and file name are correct.";
        throw InvalidValueError(config_, "file_path", "Parsing error!");
    }

    // Check if all elements of m_adjust_event_times have a valid size of 3, if not throw error.
    for(auto& shift_times : adjust_event_times_) {
        if(shift_times.size() != 3) {
            throw InvalidValueError(
                config_,
                "adjust_event_times",
                "Parameter needs 3 values per row: [\"event type\", shift event start, shift event end]");
        }
    }
}

std::shared_ptr<eudaq::StandardEvent> EventLoaderEUDAQ2::get_next_sorted_std_event() {

    // Refill the event buffer if necessary:
    while(static_cast<int>(sorted_events_.size()) < buffer_depth_) {
        LOG(DEBUG) << "Filling buffer with new event.";
        // fill buffer with new std event:
        auto new_event = get_next_std_event();
        sorted_events_.push(new_event);
    }

    // get first element of queue and erase it
    auto stdevt = sorted_events_.top();
    sorted_events_.pop();
    return stdevt;
}

std::shared_ptr<eudaq::StandardEvent> EventLoaderEUDAQ2::get_next_std_event() {

    // Check if we still have a decoded event in the cache or if we need to read and decode new ones:
    while(events_decoded_.empty()) {

        // Check if we need a new raw event or if we still have some in the cache:
        if(events_raw_.empty()) {
            LOG(TRACE) << "Reading new EUDAQ event from file";
            auto new_event = reader_->GetNextEvent();
            if(!new_event) {
                LOG(DEBUG) << "Reached EOF";
                throw EndOfFile();
            }
            // Build buffer from all sub-events:
            auto subevents = new_event->GetSubEvents();
            events_raw_ = std::queue(std::deque(subevents.begin(), subevents.end()));
            // The main event might also contain data, so add it to the buffer:
            if(events_raw_.empty()) {
                events_raw_.push(new_event);
            }
        }
        LOG(TRACE) << "Buffer contains " << events_raw_.size() << " (sub-) events:";

        // Retrieve first and remove from raw event buffer:
        auto event = events_raw_.front();
        events_raw_.pop();

        // If this is a Begin-of-Run event and we should ignore it, please do so:
        if(event->IsBORE() && ignore_bore_) {
            LOG(DEBUG) << "Found EUDAQ2 BORE event, ignoring it";
            continue;
        }

        // Create new StandardEvent and attempt to decode the raw event
        auto decoded_event = eudaq::StandardEvent::MakeShared();
        if(eudaq::StdEventConverter::Convert(event, decoded_event, eudaq_config_)) {
            // Decoding succeeded, let's add it to the FIFO with all its subevents:
            for(const auto& subevent : decoded_event->GetSubEvents()) {
                // Make sure this is a decoded event:
                auto decoded_subevent = std::dynamic_pointer_cast<const eudaq::StandardEvent>(subevent);
                if(decoded_subevent == nullptr) {
                    LOG(WARNING) << "Decoded EUDAQ2 StandardEvent " << decoded_event->GetDescription()
                                 << " contained undecoded subevent - discarded";
                    continue;
                }

                // Remove const'ness - we might have to alter it later on:
                events_decoded_.push(std::const_pointer_cast<eudaq::StandardEvent>(decoded_subevent));
            }
            events_decoded_.push(decoded_event);
            LOG(DEBUG) << event->GetDescription() << ": decoding succeeded";
        } else {
            LOG(DEBUG) << event->GetDescription() << ": decoding failed";
        }
    }

    auto stdevt = events_decoded_.front();
    events_decoded_.pop();
    return stdevt;
}

void EventLoaderEUDAQ2::retrieve_event_tags(const eudaq::EventSPC evt) {
    auto tags = evt->GetTags();

    for(auto tag_pair : tags) {
        // Trying to convert tag value to double:
        try {
            double value = std::stod(tag_pair.second);

            // Check if histogram exists already, if not: create it
            if(get_tag_histograms_) {
                if(tagHist.find(tag_pair.first) == tagHist.end()) {
                    LOG(DEBUG) << "Found new event tag \"" << tag_pair.first << "\"";
                    std::string name = "tagHist_" + tag_pair.first;
                    std::string title = tag_pair.first + ";tag value;# entries";

                    auto* directory = getROOTDirectory();
                    auto* tagdir = directory->GetDirectory("tags");
                    if(!tagdir) {
                        tagdir = directory->mkdir("tags");
                    }
                    tagdir->cd();

                    tagHist[tag_pair.first] = new TH1D(name.c_str(), title.c_str(), 10240, -128.5, 127.5);
                    directory->cd();
                }
                tagHist[tag_pair.first]->Fill(value);
            }
            if(get_tag_profiles_) {
                if(tagProfile.find(tag_pair.first) == tagProfile.end()) {
                    LOG(DEBUG) << "Found new event tag \"" << tag_pair.first << "\"";
                    std::string name = "tagProfile_" + tag_pair.first;
                    std::string title = "tag_" + tag_pair.first + ";event / 1000;tag value";

                    TDirectory* directory = getROOTDirectory();
                    auto* tagdir = directory->GetDirectory("tags");
                    if(!tagdir) {
                        tagdir = directory->mkdir("tags");
                    }
                    tagdir->cd();

                    tagProfile[tag_pair.first] = new TProfile(name.c_str(), title.c_str(), 4e5, 0, 100);
                    directory->cd();
                }
                tagProfile[tag_pair.first]->Fill(evt->GetEventN() / 1000, value, 1);
            }
        } catch(std::exception& e) {
        }
    }
}
Event::Position EventLoaderEUDAQ2::is_within_event(const std::shared_ptr<Clipboard>& clipboard,
                                                   std::shared_ptr<eudaq::StandardEvent> evt) const {
    // Potentially shift the trigger IDs if requested
    auto trigger_after_shift = static_cast<uint32_t>(static_cast<int>(evt->GetTriggerN()) + shift_triggers_);

    // Check if this event has timestamps available:
    if((evt->GetTimeBegin() == 0 && evt->GetTimeEnd() == 0) || sync_by_trigger_) {
        LOG(DEBUG) << evt->GetDescription() << ": Event has no timestamp, comparing trigger IDs";

        // If there is no event defined yet, there is little we can do:
        if(!clipboard->isEventDefined()) {
            LOG(DEBUG) << "No Corryvreckan event defined - cannot define without timestamps.";
            return Event::Position::UNKNOWN;
        }

        // Get position of this time frame with respect to the defined event:
        // Shift trigger ID of this device with respect to the IDs stored in the Corryrveckan event.
        auto trigger_position = clipboard->getEvent()->getTriggerPosition(trigger_after_shift);
        if(trigger_position == Event::Position::BEFORE) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " before triggers registered in Corryvreckan event";
            LOG(DEBUG) << "(Shifted) Trigger ID " << trigger_after_shift
                       << " before triggers registered in Corryvreckan event";
        } else if(trigger_position == Event::Position::AFTER) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " after triggers registered in Corryvreckan event";
            LOG(DEBUG) << "(Shifted) Trigger ID " << trigger_after_shift
                       << " after triggers registered in Corryvreckan event";
        } else if(trigger_position == Event::Position::UNKNOWN) {
            LOG(DEBUG) << "Trigger ID " << evt->GetTriggerN() << " within Corryvreckan event range but not registered";
            LOG(DEBUG) << "(Shifted) Trigger ID " << trigger_after_shift
                       << " within Corryvreckan event range but not registered";
        } else {
            // Redefine EUDAQ2 event begin and end to trigger timestamp (both were zero):
            evt->SetTimeBegin(static_cast<uint64_t>(clipboard->getEvent()->getTriggerTime(trigger_after_shift) * 1000));
            evt->SetTimeEnd(static_cast<uint64_t>(clipboard->getEvent()->getTriggerTime(trigger_after_shift) * 1000));

            LOG(DEBUG) << "Shifted Trigger ID " << trigger_after_shift << " found in Corryvreckan event";
        }
        return trigger_position;
    }

    // Read time from EUDAQ2 event and convert from picoseconds to nanoseconds:
    double event_start = static_cast<double>(evt->GetTimeBegin()) / 1000 + detector_->timeOffset();
    double event_end = static_cast<double>(evt->GetTimeEnd()) / 1000 + detector_->timeOffset();
    // Store the original position of the event before adjusting its length:
    double event_timestamp = event_start;
    LOG(DEBUG) << "event_start = " << Units::display(event_start, "us")
               << ", event_end = " << Units::display(event_end, "us");

    // If adjustment of event start/end is required:
    const auto it = std::find_if(adjust_event_times_.begin(),
                                 adjust_event_times_.end(),
                                 [evt](const std::vector<std::string>& x) { return x.front() == evt->GetDescription(); });

    if(it != adjust_event_times_.end()) {
        event_start += corryvreckan::from_string<double>(it->at(1));
        event_end += corryvreckan::from_string<double>(it->at(2));
        LOG(DEBUG) << "Adjusting " << it->at(0) << " event_start by "
                   << Units::display(corryvreckan::from_string<double>(it->at(1)), {"us", "ns"}) << ", event_end by "
                   << Units::display(corryvreckan::from_string<double>(it->at(2)), {"us", "ns"});
        LOG(DEBUG) << "Adjusted event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
    }

    // Skip if later start is requested:
    if(event_start < skip_time_) {
        LOG(DEBUG) << "Event start before requested skip time: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(skip_time_, {"us", "ns"});
        return Event::Position::BEFORE;
    }

    // Check if an event is defined or if we need to create it:
    if(!clipboard->isEventDefined()) {
        LOG(DEBUG) << "Defining Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " - "
                   << Units::display(event_end, {"us", "ns"}) << ", length "
                   << Units::display(event_end - event_start, {"us", "ns"});
        clipboard->putEvent(std::make_shared<Event>(event_start, event_end));
        hClipboardEventStart->Fill(static_cast<double>(Units::convert(event_start, "ms")));
        hClipboardEventStart_long->Fill(static_cast<double>(Units::convert(event_start, "s")));
        hClipboardEventEnd->Fill(static_cast<double>(Units::convert(event_end, "ms")));
        hClipboardEventDuration->Fill(
            static_cast<double>(Units::convert(clipboard->getEvent()->end() - clipboard->getEvent()->start(), "ms")));
    } else {
        LOG(DEBUG) << "Corryvreckan event found on clipboard: "
                   << Units::display(clipboard->getEvent()->start(), {"us", "ns"}) << " - "
                   << Units::display(clipboard->getEvent()->end(), {"us", "ns"})
                   << ", length: " << Units::display(clipboard->getEvent()->duration(), {"us", "ns"});
    }

    // Get position of this time frame with respect to the defined event:
    auto position = clipboard->getEvent()->getFramePosition(event_start, event_end, inclusive_);
    if(position == Event::Position::BEFORE) {
        LOG(DEBUG) << "Event start before Corryvreckan event: " << Units::display(event_start, {"us", "ns"}) << " < "
                   << Units::display(clipboard->getEvent()->start(), {"us", "ns"});
    } else if(position == Event::Position::AFTER) {
        LOG(DEBUG) << "Event end after Corryvreckan event: " << Units::display(event_end, {"us", "ns"}) << " > "
                   << Units::display(clipboard->getEvent()->end(), {"us", "ns"});
    } else if(position == Event::Position::DURING) {
        // check if event has valid trigger ID (flag = 0x10):
        if(evt->IsFlagTrigger()) {
            if(veto_triggers_ && !clipboard->getEvent()->triggerList().empty()) {
                LOG(DEBUG) << "Not storing trigger ID " << trigger_after_shift
                           << " because of existing trigger and veto flag";
            } else {
                // Store potential trigger numbers, assign to center of event:
                clipboard->getEvent()->addTrigger(trigger_after_shift, event_timestamp);
                LOG(DEBUG) << "Stored trigger ID " << trigger_after_shift << " at "
                           << Units::display(event_timestamp, {"us", "ns"});
            }
        }
    }

    return position;
}

PixelVector EventLoaderEUDAQ2::get_pixel_data(std::shared_ptr<eudaq::StandardEvent> evt, int plane_id) const {

    PixelVector pixels;

    // No plane found:
    if(plane_id < 0) {
        return pixels;
    }

    auto plane = evt->GetPlane(static_cast<size_t>(plane_id));
    // Concatenate plane name according to naming convention: sensor_type + "_" + int
    auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
    auto detector_name = detector_->getName();
    // Convert to lower case before string comparison to avoid errors by the user:
    std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
    std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);
    LOG(TRACE) << plane_name << " (ID " << plane_id << ") with " << plane.HitPixels() << " pixel hits";

    // Loop over all hits and add to pixels vector:
    for(unsigned int i = 0; i < plane.HitPixels(); i++) {

        auto col = static_cast<int>(plane.GetX(i));
        auto row = static_cast<int>(plane.GetY(i));
        auto raw = static_cast<int>(plane.GetPixel(i)); // generic pixel raw value (could be ToT, ADC, ...)

        double ts;
        if(plane.GetTimestamp(i) == 0) {
            // If the plane timestamp is not defined, we place all pixels in the center of the EUDAQ2 event.
            // Note: If the EUDAQ2 event has zero timestamps (time begin/end == 0), then the event times are
            // redefined to time begin/end == trigger timestamp in get_trigger_position (see above), i.e. in
            // that case, all pixel timestamp will be set to the corresponding trigger timestamp.
            ts = static_cast<double>(evt->GetTimeBegin() + evt->GetTimeEnd()) / 2 / 1000 + detector_->timeOffset();
        } else {
            ts = static_cast<double>(plane.GetTimestamp(i)) / 1000 + detector_->timeOffset();
        }

        if(col >= detector_->nPixels().X() || row >= detector_->nPixels().Y()) {
            LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix with size "
                         << detector_->nPixels();
        }

        if(detector_->masked(col, row)) {
            LOG(TRACE) << "Masking pixel (col, row) = (" << col << ", " << row << ")";
            continue;
        } else {
            LOG(TRACE) << "Storing (col, row, timestamp) = (" << col << ", " << row << ", "
                       << Units::display(ts, {"ns", "us", "ms"}) << ") from EUDAQ2 event data";
        }

        // when calibration is not available, set charge = raw
        auto pixel = (plane.HasWaveform(i)
                          ? std::make_shared<Waveform>(
                                detector_->getName(),
                                col,
                                row,
                                raw,
                                raw,
                                ts,
                                Waveform::waveform_t{plane.GetWaveform(i), plane.GetWaveformX0(i), plane.GetWaveformDX(i)})
                          : std::make_shared<Pixel>(detector_->getName(), col, row, raw, raw, ts));

        hitmap->Fill(col, row);
        hPixelTimes->Fill(static_cast<double>(Units::convert(ts, "ms")));
        hPixelTimes_long->Fill(static_cast<double>(Units::convert(ts, "s")));
        hPixelRawValues->Fill(raw);
        hRawValuesMap->Fill(col, row, raw);

        pixels.push_back(pixel);
    }
    hPixelMultiplicityPerEudaqEvent->Fill(static_cast<int>(pixels.size()));
    LOG(DEBUG) << detector_->getName() << ": Plane contains " << pixels.size() << " pixels";

    return pixels;
}

bool EventLoaderEUDAQ2::filter_detectors(std::shared_ptr<eudaq::StandardEvent> evt, int& plane_id) const {
    // Check if the detector type matches the currently processed detector type:
    auto detector_type = evt->GetDetectorType();
    std::transform(detector_type.begin(), detector_type.end(), detector_type.begin(), ::tolower);
    // Fall back to parsing the description if not set:
    if(detector_type.empty()) {
        LOG(TRACE) << "Using fallback comparison with EUDAQ2 event description";
        auto description = evt->GetDescription();
        std::transform(description.begin(), description.end(), description.begin(), ::tolower);
        if(description.find(detector_->getType()) == std::string::npos) {
            LOG(DEBUG) << "Ignoring event because description doesn't match type " << detector_->getType() << ": "
                       << description;
            return false;
        }
    } else if(detector_type != detector_->getType()) {
        LOG(DEBUG) << "Ignoring event because detector type doesn't match: " << detector_type;
        return false;
    }

    // To the best of our knowledge, this is the detector we are looking for.
    LOG(DEBUG) << "Found matching event for detector type " << detector_->getType();
    if(evt->NumPlanes() == 0) {
        return true;
    }

    // Check if we can identify the detector itself among the planes:
    for(size_t i_plane = 0; i_plane < evt->NumPlanes(); i_plane++) {
        auto plane = evt->GetPlane(i_plane);

        // Concatenate plane name according to naming convention: sensor_type + "_" + int
        auto plane_name = plane.Sensor() + "_" + std::to_string(plane.ID());
        auto detector_name = detector_->getName();
        // Convert to lower case before string comparison to avoid errors by the user:
        std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);
        std::transform(detector_name.begin(), detector_name.end(), detector_name.begin(), ::tolower);

        if(detector_name == plane_name) {
            plane_id = static_cast<int>(i_plane);
            LOG(DEBUG) << "Found matching plane in event for detector " << detector_->getName();
            return true;
        } else {
            LOG(DEBUG) << "plane " << plane_name << "does not match " << detector_name;
        }
    }

    // Detector not found among planes of this event
    LOG(DEBUG) << "Ignoring event because no matching plane could be found for detector " << detector_->getName();
    return false;
}

StatusCode EventLoaderEUDAQ2::run(const std::shared_ptr<Clipboard>& clipboard) {
    size_t num_eudaq_events_per_corry = 0;

    PixelVector pixels;

    Event::Position current_position = Event::Position::UNKNOWN;
    while(1) {
        // Retrieve next event from file/buffer:
        if(!event_) {
            try {
                if(buffer_depth_ == 0) {
                    // simply get next decoded EUDAQ StandardEvent from buffer
                    event_ = get_next_std_event();
                } else {
                    // get next decoded EUDAQ StandardEvent from timesorted buffer
                    event_ = get_next_sorted_std_event();
                }
            } catch(EndOfFile&) {
                return StatusCode::EndRun;
#ifdef STDEVENTCONVERTER_EXCEPTIONS_
                // Only evaluate if defined by EUDAQ2
            } catch(eudaq::DataInvalid& e) {
                LOG(ERROR) << "EUDAQ2 reports invalid data: " << e.what() << std::endl << "Ending run.";
                return StatusCode::EndRun;
#endif
            }
        }

        // Filter out "wrong" detectors and store plane ID if found:
        int plane_id = -1;
        if(!filter_detectors(event_, plane_id)) {
            event_.reset();
            continue;
        }

        // Read and store tag information for this detector:
        if(get_tag_histograms_ || get_tag_profiles_) {
            retrieve_event_tags(event_);
        }

        // Check if this event is within the currently defined Corryvreckan event:
        current_position = is_within_event(clipboard, event_);

        if(current_position == Event::Position::DURING) {
            num_eudaq_events_per_corry++;
            LOG(DEBUG) << "Is within current Corryvreckan event, storing data";
            // Store data on the clipboard
            auto new_pixels = get_pixel_data(event_, plane_id);
            hits_ += new_pixels.size();
            pixels.insert(pixels.end(), new_pixels.begin(), new_pixels.end());

            // Add eudaq tags to the event
            auto eudaq_tags = event_->GetTags();
            clipboard->getEvent()->addTags(eudaq_tags);
        }

        // If this event was after the current event or if we have not enough information, stop reading:
        if(current_position == Event::Position::AFTER) {
            break;
        } else if(current_position == Event::Position::UNKNOWN) {
            event_.reset();
            break;
        }

        // Do not fill if current_position == Event::Position::AFTER to avoid double-counting!
        // Converting EUDAQ2 picoseconds into Corryvreckan nanoseconds:
        hEudaqEventStart->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e9);       // here convert from ps to ms
        hEudaqEventStart_long->Fill(static_cast<double>(event_->GetTimeBegin()) / 1e12); // here convert from ps to seconds

        // Reset this shared event pointer to get a new event from the stack:
        event_.reset();
    }

    auto event = clipboard->getEvent();
    hTriggersPerEvent->Fill(static_cast<double>(event->triggerList().size()));
    LOG(DEBUG) << "Triggers on clipboard event: " << event->triggerList().size();
    for(auto& trigger : event->triggerList()) {
        LOG(DEBUG) << "\t ID: " << trigger.first << ", time: " << Units::display(trigger.second, "us");
    }

    LOG(DEBUG) << "Tags on clipboard event: " << event->tagList().size();
    for(auto& tag : event->tagList()) {
        LOG(DEBUG) << "\t Key: " << tag.first << " -> " << tag.second;
    }

    // histogram only exists for non-auxiliary detectors:
    if(!detector_->isAuxiliary()) {
        hPixelMultiplicityPerCorryEvent->Fill(static_cast<int>(pixels.size()));
    }

    // Loop over pixels for plotting
    if(get_time_residuals_) {
        for(auto& pixel : pixels) {
            hPixelTimeEventBeginResidual->Fill(
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));
            hPixelTimeEventBeginResidual_wide->Fill(
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));
            hPixelTimeEventBeginResidualOverTime->Fill(
                static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                static_cast<double>(Units::convert(pixel->timestamp() - event->start(), "us")));

            size_t iTrigger = 0;
            for(auto& trigger : event->triggerList()) {
                // check if histogram exists already, if not: create it
                if(hPixelTriggerTimeResidual.find(iTrigger) == hPixelTriggerTimeResidual.end()) {
                    std::string histName = "hPixelTriggerTimeResidual_" + to_string(iTrigger);
                    std::string histTitle = histName + ";pixel_ts - trigger_ts [us];# entries";
                    hPixelTriggerTimeResidual[iTrigger] = new TH1D(histName.c_str(), histTitle.c_str(), 2e5, -100, 100);
                }
                // use iTrigger, not trigger ID (=trigger.first) (which is unique and continuously incrementing over the
                // runtime)
                hPixelTriggerTimeResidual[iTrigger]->Fill(
                    static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
                if(iTrigger == 0) { // fill only for 0th trigger
                    hPixelTriggerTimeResidualOverTime->Fill(
                        static_cast<double>(Units::convert(pixel->timestamp(), "s")),
                        static_cast<double>(Units::convert(pixel->timestamp() - trigger.second, "us")));
                }
                iTrigger++;
            }
        }
    }

    // Store the full event data on the clipboard
    hEudaqeventsPerCorry->Fill(static_cast<double>(num_eudaq_events_per_corry));
    hHitsVersusEUDAQ2Frames->Fill(static_cast<double>(num_eudaq_events_per_corry), static_cast<double>(pixels.size()));
    clipboard->putData(pixels, detector_->getName());

    LOG(DEBUG) << "Finished Corryvreckan event";
    return StatusCode::Success;
}

void EventLoaderEUDAQ2::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(INFO) << "Found " << hits_ << " hits in the data.";
}
