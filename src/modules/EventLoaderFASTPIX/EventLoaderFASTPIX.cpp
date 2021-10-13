/**
 * @file
 * @brief Implementation of module EventLoaderFASTPIX
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderFASTPIX.h"
#include "objects/SpidrSignal.hpp"
#include <TMath.h>
#include <TMultiGraph.h>

using namespace corryvreckan;

EventLoaderFASTPIX::EventLoaderFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    m_inputFile.open(config_.getPath("input_file"), std::ifstream::binary);
    m_debugFile.open("debug.txt");
}

void Honeycomb(TH2Poly *h, Double_t xstart, Double_t ystart, Double_t a, Int_t k, Int_t s) {
   // Add the bins
   Double_t sc = 1/1.5;

   Double_t x[6], y[6];
   Double_t xloop, yloop, xtemp;
   xloop = xstart; yloop = ystart + sc*a/2.0;
   for (int sCounter = 0; sCounter < s; sCounter++) {
      xtemp = xloop; // Resets the temp variable

      for (int kCounter = 0; kCounter <  k; kCounter++) {
         // Go around the hexagon
         x[0] = xtemp;
         y[0] = yloop;
         x[1] = x[0];
         y[1] = y[0] + a*sc;
         x[2] = x[1] + a/2.0;
         y[2] = y[1] + a*sc/2.0;
         x[3] = x[2] + a/2.0;
         y[3] = y[1];
         x[4] = x[3];
         y[4] = y[0];
         x[5] = x[2];
         y[5] = y[4] - a*sc/2.0;

         h->AddBin(6, x, y);

         // Go right
         xtemp += a;
      }

      // Increment the starting position
      if (sCounter%2 == 0) xloop += a/2.0;
      else                 xloop -= a/2.0;
      yloop += 1.5*a*sc;
   }
}

void EventLoaderFASTPIX::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    hitmap = new TH2Poly();
    hitmap->SetName("hitmap");
    hitmap->SetTitle("hitmap");
    Honeycomb(hitmap,0.5,0.5/1.5,1,16,4);

    test = new TH1F("hPixelEfficiency", "hPixelEfficiency; single pixel efficiency; # entries", 201, 0, 1.005);
    trigger_delta = new TH1F("hTriggerDelta", "hPixelEfficiency; single pixel efficiency; # entries", 1001, 0, 1000);

    // Initialise member variables
    m_eventNumber = 0;
    m_triggerNumber = 0;
    m_prevTriggerTime = 0;
    m_prevScopeTriggerTime = 0;
    m_lostTriggers = 0;
    m_discardedEvents = 0;

    m_spidr_t0 = 0;
    m_scope_t0 = 0;

    m_triggerSync = false;
    m_timeScaler = 0.99993;

    m_inputFile.read(reinterpret_cast<char*>(&m_blockSize), sizeof m_blockSize);
}

// number of events / block
// event timestamp
// seed time
// cfd time
// number of pixels / event
// column, row, ToT [xN]

bool EventLoaderFASTPIX::loadEvent(const std::shared_ptr<Clipboard>& clipboard, PixelVector &deviceData, double spidr_timestamp) {
    std::string detectorID = m_detector->getName();

    uint16_t event_size;
    double event_timestamp, seed_time, cfd_time;

    m_inputFile.read(reinterpret_cast<char*>(&event_timestamp), sizeof event_timestamp);
    m_inputFile.read(reinterpret_cast<char*>(&seed_time), sizeof seed_time);
    m_inputFile.read(reinterpret_cast<char*>(&cfd_time), sizeof cfd_time);
    m_inputFile.read(reinterpret_cast<char*>(&event_size), sizeof event_size);

    for(uint16_t i = 0; i < event_size; i++) {
        uint16_t col, row;
        double tot;

        m_inputFile.read(reinterpret_cast<char*>(&col), sizeof col);
        m_inputFile.read(reinterpret_cast<char*>(&row), sizeof row);
        m_inputFile.read(reinterpret_cast<char*>(&tot), sizeof tot);

        LOG(DEBUG) << "Column " << col << " row " << row << " ToT " << tot;

        int idx = row*16+col+1;
        hitmap->SetBinContent(idx, hitmap->GetBinContent(idx)+1);

        auto pixel = std::make_shared<Pixel>(detectorID, col, row, static_cast<int>(tot), tot, spidr_timestamp);
        deviceData.push_back(pixel);
    }

    m_triggerNumber++;

    //re-sync oscilloscope and SPIDR triggers at start of every data block
    if(m_triggerNumber % m_blockSize == 0) {
        m_triggerSync = false;
    }

    return !deviceData.empty();
}

double EventLoaderFASTPIX::getRawTimestamp() {
    double timestamp;

    m_inputFile.read(reinterpret_cast<char*>(&timestamp), sizeof timestamp);
    m_inputFile.seekg(-sizeof timestamp, std::ios_base::cur);

    return timestamp * 1e9;
}

double EventLoaderFASTPIX::getTimestamp() {
    double timestamp = getRawTimestamp();

    return (timestamp - m_scope_t0) * m_timeScaler + m_spidr_t0;
}

StatusCode EventLoaderFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto reference = get_reference();
    auto referenceSpidrSignals = clipboard->getData<SpidrSignal>(reference->getName());
    auto event = clipboard->getEvent();

    size_t spidr_index = 0;

    for(;;) {
        PixelVector deviceData;
        double timestamp = getTimestamp();
        auto position = event->getTimestampPosition(timestamp);

        if(m_triggerSync) {
            if(spidr_index < referenceSpidrSignals.size()) { // match events by trigger number
                loadEvent(clipboard, deviceData, referenceSpidrSignals[spidr_index]->timestamp());
                if(referenceSpidrSignals[spidr_index]->trigger() == m_triggerNumber + 1) {
                    spidr_index++;
                    LOG(INFO) << "Loading event for trigger " << m_triggerNumber + 1;

                    // re-sync timestamps after each event
                    m_spidr_t0 = referenceSpidrSignals[spidr_index]->timestamp();
                    m_scope_t0 = getRawTimestamp();

                    if(!deviceData.empty()) {
                        clipboard->putData(deviceData, m_detector->getName());
                    }
                } else {
                    LOG(INFO) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger " << referenceSpidrSignals[spidr_index]->trigger() << ". Previous Fastpix event assigned to wrong event?";
                    LOG(INFO) << "Discarding Fastpix event";
                    LOG(INFO) << "Timestamp " << Units::display(timestamp, {"s", "us", "ns"});
                    LOG(INFO) << "Raw timestamp " << Units::display(getRawTimestamp(), {"s", "us", "ns"});
                    LOG(INFO) << "SPIDR timestamp " << Units::display(referenceSpidrSignals[spidr_index]->timestamp(), {"s", "us", "ns"});
                    LOG(INFO) << "Event " << Units::display(event->start(), {"s", "us", "ns"}) << " " << Units::display(event->end(), {"s", "us", "ns"}); 
                    m_discardedEvents++;
                    for(;;);
                }
            } else { // no more SPIDR triggers in current event. Try matching timestamps instead
                if(position == Event::Position::DURING) {
                    LOG(INFO) << "Loading event for trigger " << m_triggerNumber + 1 << " without matching SPIDR trigger";
                    LOG(INFO) << "Timestamp " << Units::display(timestamp, {"s", "us", "ns"});
                    LOG(INFO) << "Raw timestamp " << Units::display(getRawTimestamp(), {"s", "us", "ns"});
                    LOG(INFO) << "Event " << Units::display(event->start(), {"s", "us", "ns"}) << " " << Units::display(event->end(), {"s", "us", "ns"}); 
                    loadEvent(clipboard, deviceData, timestamp);
                
                    if(!deviceData.empty()) {
                        clipboard->putData(deviceData, m_detector->getName());
                    }
                } else if(position == Event::Position::BEFORE) {
                    LOG(INFO) << "Event for trigger " << m_triggerNumber + 1 << " without matching SPIDR trigger or timestamp";
                    LOG(INFO) << "Discarding Fastpix event";
                    loadEvent(clipboard, deviceData, timestamp);
                    m_discardedEvents++;
                } else if(position == Event::Position::AFTER) {
                    break;
                }
            }

        } else { // wait for event with SPIDR trigger
            if(spidr_index < referenceSpidrSignals.size()) {
                if(referenceSpidrSignals[spidr_index]->trigger() == m_triggerNumber + 1) {
                    LOG(INFO) << "Synchronising events for trigger " << m_triggerNumber + 1;
                    LOG(INFO) << "Timestamp " << Units::display(timestamp, {"s", "us", "ns"});
                    LOG(INFO) << "Raw timestamp " << Units::display(getRawTimestamp(), {"s", "us", "ns"});
                    LOG(INFO) << "SPIDR timestamp " << Units::display(referenceSpidrSignals[spidr_index]->timestamp(), {"s", "us", "ns"});
                    LOG(INFO) << "Event " << Units::display(event->start(), {"s", "us", "ns"}) << " " << Units::display(event->end(), {"s", "us", "ns"}); 
                    loadEvent(clipboard, deviceData, referenceSpidrSignals[spidr_index]->timestamp());

                    m_spidr_t0 = referenceSpidrSignals[spidr_index]->timestamp();
                    m_scope_t0 = getRawTimestamp();
                    m_triggerSync = true;

                    if(!deviceData.empty()) {
                        clipboard->putData(deviceData, m_detector->getName());
                    }
                } else {
                    LOG(INFO) << "Expected SPIDR trigger " << m_triggerNumber + 1 << " but got trigger " << referenceSpidrSignals[spidr_index]->trigger();
                    LOG(INFO) << "Discarding Fastpix event";
                    loadEvent(clipboard, deviceData, timestamp);
                    m_discardedEvents++;
                }
                
                spidr_index++;
            } else {
                break;
            }
        }
    }


    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    hitmap->Write();

    trigger_graph = new TGraph(m_triggerTimestamps.size(), &m_triggerTimestamps[0], &m_triggerNumbers[0]);
    trigger_graph->GetXaxis()->SetTitle("trigger timestamp");
    trigger_graph->GetYaxis()->SetTitle("trigger number");
    trigger_graph->Write("trigger");

    trigger_scope_graph = new TGraph(m_triggerScopeTimestamps.size(), &m_triggerScopeTimestamps[0], &m_triggerNumbers[0]);
    trigger_scope_graph->GetXaxis()->SetTitle("trigger timestamp");
    trigger_scope_graph->GetYaxis()->SetTitle("trigger number");
    trigger_scope_graph->Write("trigger_scope");

    TGraph *trigger_scope_graph_corrected = new TGraph(m_triggerScopeTimestampsCorrected.size(), &m_triggerScopeTimestampsCorrected[0], &m_triggerNumbers[0]);
    trigger_scope_graph_corrected->GetXaxis()->SetTitle("trigger timestamp");
    trigger_scope_graph_corrected->GetYaxis()->SetTitle("trigger number");
    trigger_scope_graph_corrected->Write("trigger_scope_corrected");

    TMultiGraph *mg = new TMultiGraph();
    mg->Add(trigger_graph);
    mg->Add(trigger_scope_graph);
    mg->Add(trigger_scope_graph_corrected);
    mg->Write("triggers");

    std::vector<double> dt_ratio;

    for(size_t i = 1; i < m_triggerNumbers.size(); i++) {
        double dt_spidr = m_triggerTimestamps[i] - m_triggerTimestamps[i-1];
        double dt_scope = m_triggerScopeTimestamps[i] - m_triggerScopeTimestamps[i-1];

        dt_ratio.emplace_back(dt_spidr/dt_scope);
    }

    trigger_dt_ratio = new TGraph(dt_ratio.size(), &m_triggerNumbers[1], &dt_ratio[0]);
    trigger_dt_ratio->GetXaxis()->SetTitle("trigger number");
    trigger_dt_ratio->GetYaxis()->SetTitle("trigger dt_spidr/dt_scope");
    trigger_dt_ratio->Write("trigger_dt_ratio");

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
    LOG(DEBUG) << "Lost " << m_lostTriggers << " triggers";
}
