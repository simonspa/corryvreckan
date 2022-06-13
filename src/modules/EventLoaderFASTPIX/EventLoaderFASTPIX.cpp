/**
 * @file
 * @brief Implementation of module EventLoaderFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderFASTPIX.h"
#include <TMath.h>
#include <TMultiGraph.h>
#include "objects/SpidrSignal.hpp"

using namespace corryvreckan;

EventLoaderFASTPIX::EventLoaderFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    m_inputFile.open(config_.getPath("input_file"), std::ifstream::binary);

    m_timeScaler = config_.get("time_scaler", 0.999994);
}

void Honeycomb(TH2Poly* h, Double_t xstart, Double_t ystart, Int_t k, Int_t s);

// Adapted from ROOT Honeycomb function
void Honeycomb(TH2Poly* h, Double_t xstart, Double_t ystart, Int_t k, Int_t s) {
    // Add the bins
    Double_t sc = 1 / 1.5;

    Double_t x[6], y[6];
    Double_t xloop, yloop, xtemp;
    ystart *= sc;
    xloop = xstart;
    yloop = ystart + sc / 2.0;
    for(int sCounter = 0; sCounter < s; sCounter++) {
        xtemp = xloop; // Resets the temp variable

        for(int kCounter = 0; kCounter < k; kCounter++) {
            // Go around the hexagon
            x[0] = xtemp;
            y[0] = yloop;
            x[1] = x[0];
            y[1] = y[0] + sc;
            x[2] = x[1] + 1 / 2.0;
            y[2] = y[1] + sc / 2.0;
            x[3] = x[2] + 1 / 2.0;
            y[3] = y[1];
            x[4] = x[3];
            y[4] = y[0];
            x[5] = x[2];
            y[5] = y[4] - sc / 2.0;

            h->AddBin(6, x, y);

            // Go right
            xtemp += 1;
        }

        // Increment the starting position
        if(sCounter % 2 == 0)
            xloop += 1 / 2.0;
        else
            xloop -= 1 / 2.0;
        yloop += 1.5 * sc;
    }
}

void EventLoaderFASTPIX::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    hitmap = new TH2Poly();
    hitmap->SetName("hitmap");
    hitmap->SetTitle("hitmap;Pixel x;Pixel y");
    Honeycomb(hitmap, 0.5, 0.5, 16, 4);

    seed_tot_inner = new TH1F("seed_tot_inner", "ToT inner seed pixels;ToT [ns];# pixels", 350, 0.5, 350.5);
    seed_tot_outer = new TH1F("seed_tot_outer", "ToT outer seed pixels;ToT [ns];# pixels", 350, 0.5, 350.5);
    pixels_per_event = new TH1F("pixels_per_event", "Pixels per event", 10, 0.5, 10.5);
    pixel_timestamps = new TH1F("pixel_timestamps", "Pixel timestamps", 200, -0.5, 99.5);
    pixel_distance = new TH1F("pixel_distance", "Distance to seed pixel", 20, -0.5, 19.5);
    pixel_distance_min = new TH1F("pixel_distance_min", "Minimum distance to seed pixel", 20, -0.5, 19.5);
    pixel_distance_max = new TH1F("pixel_distance_max", "Maximum distance to seed pixel", 20, -0.5, 19.5);
    pixel_distance_row = new TH1F("pixel_distance_row", "Distance to seed pixel (row)", 20, -0.5, 19.5);
    pixel_distance_col = new TH1F("pixel_distance_col", "Distance to seed pixel (column)", 20, -0.5, 19.5);
    trigger_dt = new TH1F("trigger_dt", "trigger_dt;Trigger #Deltat [#mus];# entries", 1000, -0.5, 200.5);

    event_status = new TH1F("event_status", "event status", 5, -0.5, 4.5);
    missing_peaks = new TH1F("missing_peaks", "missing peaks", 20, -0.5, 19.5);
    missing_peaks_vs_size = new TH2F("missing_peaks_vs_size",
                                     "Decoded pixels vs missing peaks;## decoded pixels;## missing peaks",
                                     15,
                                     -0.5,
                                     14.5,
                                     14,
                                     0.5,
                                     14.5);

    // Initialise member variables
    m_eventNumber = 0;
    m_triggerNumber = 0;
    m_prevTriggerTime = 0;
    m_prevScopeTriggerTime = 0;
    m_missingTriggers = 0;
    m_discardedEvents = 0;

    m_loadedEvents = 0;
    m_incompleteEvents = 0;
    m_noiseEvents = 0;

    m_spidr_t0 = 0;
    m_scope_t0 = 0;

    m_triggerSync = false;

    uint16_t version = 0;

    if(!m_inputFile) {
        throw ModuleError("Could not open file");
    }

    m_inputFile.read(reinterpret_cast<char*>(&version), sizeof version);
    LOG(INFO) << "Reading file format version " << version;

    if(version != 0) {
        throw ModuleError("Invalid file format version " + std::to_string(static_cast<unsigned>(version)));
    }

    m_inputFile.read(reinterpret_cast<char*>(&m_blockSize), sizeof m_blockSize);

    if(m_blockSize == 0) {
        throw ModuleError("Invalid data block size");
    }
}

size_t hex_distance(double x1, double y1, double x2, double y2);

size_t hex_distance(double x1, double y1, double x2, double y2) {
    return static_cast<size_t>(std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(-x1 - y1 + x2 + y2)) / 2;
}

// File format version
// number of events / block
// event timestamp
// event flags & metadata
// number of pixels / event
// pixel ID, ToT, px timestamp [xN]

bool EventLoaderFASTPIX::loadEvent(PixelVector& deviceData,
                                   std::map<std::string, std::string>& eventTags,
                                   double spidr_timestamp,
                                   bool discard) {
    std::string detectorID = m_detector->getName();

    uint16_t event_size;
    double event_timestamp;
    uint16_t event_flags;
    uint16_t event_meta;

    m_prevEvent = m_inputFile.tellg();

    m_inputFile.read(reinterpret_cast<char*>(&event_timestamp), sizeof event_timestamp);
    m_inputFile.read(reinterpret_cast<char*>(&event_flags), sizeof event_flags);
    m_inputFile.read(reinterpret_cast<char*>(&event_meta), sizeof event_meta);
    m_inputFile.read(reinterpret_cast<char*>(&event_size), sizeof event_size);

    LOG(DEBUG) << "Event timestamp: " << event_timestamp;
    LOG(DEBUG) << "Event size: " << event_size;
    LOG(DEBUG) << "Previous event: " << m_prevEvent;

    eventTags.emplace(std::string("fp_flags:") + std::to_string(m_triggerNumber + 1),
                      std::to_string(static_cast<unsigned>(event_flags)));
    eventTags.emplace(std::string("fp_meta:") + std::to_string(m_triggerNumber + 1),
                      std::to_string(static_cast<unsigned>(event_meta)));

    if(!discard) {
        pixels_per_event->Fill(event_size);
        event_status->Fill(event_flags);

        if(event_meta > 0 && event_flags == 1) {
            missing_peaks->Fill(event_meta);
            missing_peaks_vs_size->Fill(event_size, event_meta);
        }

        if(event_flags == 1) {
            m_incompleteEvents++;
        }

        if(event_flags == 2) {
            m_noiseEvents++;
        }

        m_loadedEvents++;
    }

    int seed_col = 0;
    int seed_row = 0;

    for(uint16_t i = 0; i < event_size; i++) {
        uint16_t idx;
        double tot, px_timestamp;

        m_inputFile.read(reinterpret_cast<char*>(&idx), sizeof idx);
        m_inputFile.read(reinterpret_cast<char*>(&tot), sizeof tot);
        m_inputFile.read(reinterpret_cast<char*>(&px_timestamp), sizeof px_timestamp);

        hitmap->SetBinContent(idx + 1, hitmap->GetBinContent(idx + 1) + 1);
        pixel_timestamps->Fill(px_timestamp);

        int row = idx / 16;
        int col = idx % 16;

        if(i == 0) {
            seed_col = col;
            seed_row = row;

            if(col == 0 || col == 15 || row == 0 || row == 3) {
                seed_tot_outer->Fill(tot);
            } else {
                seed_tot_inner->Fill(tot);
            }
        } else {
            pixel_distance->Fill(static_cast<double>(hex_distance(col, row, seed_col, seed_row)));
            pixel_distance_row->Fill(std::abs(row - seed_row));
            pixel_distance_col->Fill(std::abs(col - seed_col));
        }

        col = col - (row - (row & 1)) / 2;

        auto pixel = std::make_shared<Pixel>(
            detectorID, col, row, static_cast<int>(tot), tot, spidr_timestamp + px_timestamp + m_detector->timeOffset());
        deviceData.push_back(pixel);
    }

    m_scopeTriggerNumbers.emplace_back(m_triggerNumber + 1);
    m_scopeTriggerTimestamps.emplace_back(spidr_timestamp);

    m_triggerNumber++;

    // re-sync oscilloscope and SPIDR triggers at start of every data block
    if(m_triggerNumber % m_blockSize == 0) {
        m_triggerSync = false;
    }

    return !deviceData.empty();
}

double EventLoaderFASTPIX::getRawTimestamp() {
    double timestamp;

    m_inputFile.read(reinterpret_cast<char*>(&timestamp), sizeof timestamp);
    m_inputFile.seekg(-static_cast<int>(sizeof timestamp), std::ios_base::cur);

    return timestamp * 1e9;
}

double EventLoaderFASTPIX::getTimestamp() {
    double timestamp = getRawTimestamp();

    return (timestamp - m_scope_t0) * m_timeScaler + m_spidr_t0;
}

StatusCode EventLoaderFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto event = clipboard->getEvent();
    auto t = event->triggerList();
    std::vector<std::pair<uint32_t, double>> triggers(t.begin(), t.end());

    for(const auto& trigger : triggers) {
        m_spidrTriggerNumbers.emplace_back(trigger.first);
        m_spidrTriggerTimestamps.emplace_back(trigger.second);
    }

    size_t prev_trigger = m_triggerNumber;
    size_t trigger_index = 0;
    bool dead_time = false;

    PixelVector deviceData;
    PixelVector discardData;

    std::map<std::string, std::string> eventTags;
    std::map<std::string, std::string> discardTags;

    for(;;) {
        // Oscilloscope triggers are aligned to SPIDR timestamps/triggers. Time offsets should be compensated in the
        // EventLoaderTimestamp module and not in this event loader to assign both triggers and pixel data to the correct
        // event.
        LOG(DEBUG) << "Raw timestamp: " << getRawTimestamp();
        double timestamp = getTimestamp();
        auto position = event->getTimestampPosition(timestamp);

        if(m_triggerSync) {
            if(trigger_index < triggers.size()) {                          // match events by trigger number
                if(triggers[trigger_index].first == m_triggerNumber + 1) { // trigger numbers match
                    LOG(DEBUG) << "Loading event for trigger " << m_triggerNumber + 1;

                    // re-sync timestamps after each event
                    m_spidr_t0 = triggers[trigger_index].second;
                    m_scope_t0 = getRawTimestamp();

                    m_ratioTriggerNumbers.emplace_back(m_triggerNumber + 1);
                    m_dtRatio.emplace_back((m_spidr_t0 - m_prevTriggerTime) / (m_scope_t0 - m_prevScopeTriggerTime));
                    trigger_dt->Fill((m_spidr_t0 - m_prevTriggerTime) / 1000.0);

                    m_prevScopeTriggerTime = m_scope_t0;
                    m_prevTriggerTime = m_spidr_t0;

                    loadEvent(deviceData, eventTags, triggers[trigger_index].second);

                    trigger_index++;
                } else if(triggers[trigger_index].first >
                          m_triggerNumber + 1) { // SPIDR trigger is after Fastpix trigger (no earlier event with matching
                                                 // timestamp?). discard Fastpix data.
                    LOG(DEBUG) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger "
                               << triggers[trigger_index].first << ".";
                    LOG(DEBUG) << "Discarding Fastpix event";
                    m_discardedEvents++;

                    loadEvent(discardData, discardTags, triggers[trigger_index].second, true);
                } else { // SPIDR trigger is before Fastpix trigger. Previous Fastpix trigger was assigned to wrong event?
                    LOG(INFO) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger "
                              << triggers[trigger_index].first << ". Previous Fastpix event assigned to wrong event?";
                    m_discardedEvents++;

                    if(triggers[trigger_index].first ==
                       m_triggerNumber) { // SPIDR trigger belongs to previous Fastpix trigger
                        LOG(INFO) << "Rewinding Fastpix trigger";
                        m_triggerNumber--;
                        m_inputFile.seekg(m_prevEvent, std::ios_base::beg);

                        loadEvent(deviceData, eventTags, triggers[trigger_index].second);
                    } else {
                        LOG(INFO) << "Discarding SPIDR trigger";
                    }

                    trigger_index++;
                }
            } else { // no more SPIDR triggers in current event. Try matching timestamps instead
                if(position == Event::Position::DURING) { // Fastpix trigger belongs to this event
                    LOG(INFO) << "Event for trigger " << m_triggerNumber + 1 << " without matching SPIDR trigger";
                    LOG(INFO) << "Discarding Fastpix event";
                    loadEvent(discardData, discardTags, timestamp, true);
                    m_discardedEvents++;
                    dead_time = true; // Current event is likely to be incomplete (lost data packets in SPIDR readout?)
                                      // discard entire event to prevent issues with efficiency measurements

                } else if(position == Event::Position::BEFORE) { // Fastpix trigger belongs to an earlier event (no earlier
                                                                 // event with matching timestamp?)
                    LOG(INFO) << "Event for trigger " << m_triggerNumber + 1
                              << " without matching SPIDR trigger or timestamp";
                    LOG(INFO) << "Discarding Fastpix event";
                    loadEvent(discardData, discardTags, timestamp, true);
                    m_discardedEvents++;
                } else if(position == Event::Position::AFTER) { // Fastpix trigger belongs to a later event. Stop processing.
                    break;
                }
            }

        } else { // wait for event with SPIDR trigger to synchronise timestamps
            if(trigger_index < triggers.size()) {
                if(triggers[trigger_index].first == m_triggerNumber + 1) {
                    LOG(INFO) << "Synchronising events for trigger " << m_triggerNumber + 1;

                    m_spidr_t0 = triggers[trigger_index].second;
                    m_scope_t0 = getRawTimestamp();
                    m_prevScopeTriggerTime = m_scope_t0;
                    m_prevTriggerTime = m_spidr_t0;

                    m_triggerSync = true;

                    loadEvent(deviceData, eventTags, triggers[trigger_index].second);

                } else if(triggers[trigger_index].first < m_triggerNumber + 1) {
                    LOG(DEBUG) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger "
                               << triggers[trigger_index].first << " and triggers are not yet in sync";
                    LOG(DEBUG) << "Skipping SPIDR trigger";
                } else {
                    LOG(DEBUG) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger "
                               << triggers[trigger_index].first << " and triggers are not yet in sync";
                    size_t discard = triggers[trigger_index].first - (m_triggerNumber + 1);
                    LOG(DEBUG) << "Discarding " << discard << " Fastpix events";
                    for(size_t i = 0; i < discard; i++) {
                        loadEvent(discardData, discardTags, timestamp, true);
                    }
                    m_discardedEvents += discard;
                    continue;
                }

                trigger_index++;
            } else {
                break;
            }
        }
    }

    if(!deviceData.empty()) {
        clipboard->putData(deviceData, m_detector->getName());
    }

    if(!eventTags.empty()) {
        event->addTags(eventTags);
    }

    // Increment event counter
    m_eventNumber++;

    // Oscilloscope is copying data or might join the run a few seconds late, or missing packets in SPIDR
    // Discard events between last trigger in a block and first trigger in the next block
    if(prev_trigger % m_blockSize == 0 || m_triggerNumber % m_blockSize == 0 || dead_time) {
        return StatusCode::DeadTime;
    } else {
        // Return value telling analysis to keep running
        return StatusCode::Success;
    }
}

void EventLoaderFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    hitmap->Write();

    TGraph scope_trigger_graph(
        static_cast<int>(m_scopeTriggerTimestamps.size()), &m_scopeTriggerTimestamps[0], &m_scopeTriggerNumbers[0]);
    scope_trigger_graph.Write("scope_triggers");

    TGraph spidr_trigger_graph(
        static_cast<int>(m_spidrTriggerTimestamps.size()), &m_spidrTriggerTimestamps[0], &m_spidrTriggerNumbers[0]);
    spidr_trigger_graph.Write("spidr_triggers");

    TMultiGraph trigger_graph;
    trigger_graph.Add(&scope_trigger_graph);
    trigger_graph.Add(&spidr_trigger_graph);
    trigger_graph.Write("triggers");

    TGraph dt_ratio(static_cast<int>(m_ratioTriggerNumbers.size()), &m_ratioTriggerNumbers[0], &m_dtRatio[0]);
    dt_ratio.Write("trigger_dt_ratio");

    LOG(INFO) << "Analysed " << m_eventNumber << " events";
    LOG(INFO) << "Loaded " << m_loadedEvents << " events";
    LOG(INFO) << "Discarded " << m_discardedEvents << " events";
    LOG(INFO) << "Missing " << m_missingTriggers << " triggers";

    LOG(INFO) << "Complete events: " << m_loadedEvents - m_noiseEvents - m_incompleteEvents << " ("
              << 100.0 * static_cast<double>((m_loadedEvents - m_noiseEvents - m_incompleteEvents)) /
                     static_cast<double>(m_loadedEvents)
              << "%)";
    LOG(INFO) << "Incomplete events: " << m_incompleteEvents << " ("
              << 100.0 * static_cast<double>(m_incompleteEvents) / static_cast<double>(m_loadedEvents) << "%)";
    LOG(INFO) << "Noise events: " << m_noiseEvents << " ("
              << 100.0 * static_cast<double>(m_noiseEvents) / static_cast<double>(m_loadedEvents) << "%)";
}
