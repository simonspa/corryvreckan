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

    m_spidr_t0 = 0;
    m_scope_t0 = 0;

    m_inputFile.read(reinterpret_cast<char*>(&m_blockSize), sizeof m_blockSize);
}

// number of events / block
// event timestamp
// seed time
// cfd time
// number of pixels / event
// column, row, ToT [xN]

bool EventLoaderFASTPIX::loadData(const std::shared_ptr<Clipboard>& clipboard, PixelVector &deviceData, double spidr_timestamp) {
    std::string detectorID = m_detector->getName();

    uint16_t event_size;
    double event_timestamp, seed_time, cfd_time;

    m_inputFile.read(reinterpret_cast<char*>(&event_timestamp), sizeof event_timestamp);
    m_inputFile.read(reinterpret_cast<char*>(&seed_time), sizeof seed_time);
    m_inputFile.read(reinterpret_cast<char*>(&cfd_time), sizeof cfd_time);
    m_inputFile.read(reinterpret_cast<char*>(&event_size), sizeof event_size);

    event_timestamp *= 1e9;

    // synchronise SPIDR and oscilloscope timestamps at start of each block
    if(m_triggerNumber % m_blockSize == 0) {
        //m_spidr_t0 = spidr_timestamp;
        //m_scope_t0 = event_timestamp;
    }

    m_triggerScopeTimestamps.emplace_back(event_timestamp-m_scope_t0);

    LOG(DEBUG) << "Event timestamp " << event_timestamp << " Seed time " << seed_time << " CFD time " << cfd_time << " event size " << event_size;

    double dt_scope = event_timestamp - m_prevScopeTriggerTime;
    double dt_spidr = spidr_timestamp - m_prevTriggerTime;

    LOG(DEBUG) << "dt scope: " << dt_scope;
    LOG(DEBUG) << "dt spidr: " << dt_spidr;
    LOG(DEBUG) << "dt_spidr/dt_scope: " << dt_spidr/dt_scope;


    /*if(std::abs((spidr_timestamp - m_spidr_t0) - (event_timestamp - m_scope_t0)) > 1) {
        LOG(DEBUG) << "Mismatch between SPIDR and oscilloscope timestamps. Discarding FASTPIX event. (Lost trigger?)";
        LOG(DEBUG) << "dt: " << (spidr_timestamp - m_spidr_t0) - (event_timestamp - m_scope_t0);
    }*/

    if(event_size == 0) {
      m_debugFile << "Empty event " << event_timestamp << '\n';
    }

    if((dt_spidr/dt_scope > 1.0001 || dt_spidr/dt_scope < 0.9999) && (m_triggerNumber % m_blockSize != 0)) {
        LOG(DEBUG) << "Mismatch between SPIDR and oscilloscope timestamps. Discarding FASTPIX event. (Lost trigger?)";
        m_inputFile.seekg(event_size * (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(double)), std::ios_base::cur);
    } else {
        for(uint16_t i = 0; i < event_size; i++) {
            uint16_t col, row;
            double tot;

            m_inputFile.read(reinterpret_cast<char*>(&col), sizeof col);
            m_inputFile.read(reinterpret_cast<char*>(&row), sizeof row);
            m_inputFile.read(reinterpret_cast<char*>(&tot), sizeof tot);

            LOG(DEBUG) << "Column " << col << " row " << row << " ToT " << tot;

            int idx = row*16+col+1;
            hitmap->SetBinContent(idx, hitmap->GetBinContent(idx)+1);

            m_debugFile << "Pixel " << idx << " col " << col << " row " << row << " tot " << tot << " timestamp " << event_timestamp << '\n';

            auto pixel = std::make_shared<Pixel>(detectorID, col, row, static_cast<int>(tot), tot, event_timestamp);
            deviceData.push_back(pixel);
        }
        m_prevScopeTriggerTime = event_timestamp;
    }

    return !deviceData.empty();
}

StatusCode EventLoaderFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto reference = get_reference();
    auto referenceSpidrSignals = clipboard->getData<SpidrSignal>(reference->getName());
    auto referencePixels = clipboard->getData<Pixel>(reference->getName());

    PixelVector deviceData;

    for(auto& refSpidrSignal : referenceSpidrSignals){
        //LOG(DEBUG) << reference->getName();
        //LOG(DEBUG) << refSpidrSignal->type();
        LOG(DEBUG) << "Loading data for event " << m_eventNumber << " trigger " << m_triggerNumber << " delta t " << (refSpidrSignal->timestamp() - m_prevTriggerTime);

        loadData(clipboard, deviceData, refSpidrSignal->timestamp());

        m_prevTriggerTime = refSpidrSignal->timestamp(); // - m_spidr_t0;

        m_triggerTimestamps.emplace_back(m_prevTriggerTime);
        m_triggerNumbers.emplace_back(m_triggerNumber);

        m_triggerNumber++;
    }

    if(!deviceData.empty()) {
        clipboard->putData(deviceData, m_detector->getName());
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

    trigger_scope_graph = new TGraph(m_triggerTimestamps.size(), &m_triggerScopeTimestamps[0], &m_triggerNumbers[0]);
    trigger_scope_graph->GetXaxis()->SetTitle("trigger timestamp");
    trigger_scope_graph->GetYaxis()->SetTitle("trigger number");
    trigger_scope_graph->Write("trigger_scope");

    TMultiGraph *mg = new TMultiGraph();
    mg->Add(trigger_graph);
    mg->Add(trigger_scope_graph);
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
}
