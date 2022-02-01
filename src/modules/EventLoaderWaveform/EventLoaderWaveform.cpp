/**
 * @file
 * @brief Implementation of module EventLoaderWaveform
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderWaveform.h"
#include "objects/SpidrSignal.hpp"
#include "objects/Waveform.hpp"

using namespace corryvreckan;

EventLoaderWaveform::EventLoaderWaveform(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    m_inputDirectory = config_.getPath("input_directory");
    // m_channels = m_detector->getConfiguration().getArray<std::string>("channels");
    m_channels = config_.getArray<std::string>("channels");

    LOG(DEBUG) << "Directory " << m_inputDirectory;
}

void EventLoaderWaveform::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Initialise member variables
    m_eventNumber = 0;
    m_triggerNumber = 1;
    m_loader = std::make_unique<DirectoryLoader>(m_inputDirectory, m_channels);
}

StatusCode EventLoaderWaveform::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto reference = get_reference();
    auto referenceSpidrSignals = clipboard->getData<SpidrSignal>(reference->getName());

    WaveformVector deviceData;

    for(const auto& spidr : referenceSpidrSignals) {
        if(spidr->trigger() == m_triggerNumber) {
            auto waveform = m_loader->read();
            m_triggerNumber++;

            for(const auto& w : waveform) {
                deviceData.emplace_back(std::make_shared<Waveform>("", spidr->timestamp(), spidr->trigger(), w));
                LOG(DEBUG) << "Loading waveform for trigger " << spidr->trigger();
            }
        } else if(spidr->trigger() > m_triggerNumber) {
            while(spidr->trigger() > m_triggerNumber) {
                LOG(DEBUG) << "Skipping waveform for trigger " << m_triggerNumber;
                auto w = m_loader->read();
                m_triggerNumber++;
            }
            auto waveform = m_loader->read();
            m_triggerNumber++;

            for(const auto& w : waveform) {
                deviceData.emplace_back(std::make_shared<Waveform>("", spidr->timestamp(), spidr->trigger(), w));
                LOG(DEBUG) << "Loading waveform for trigger " << spidr->trigger();
            }
        }
    }

    if(!deviceData.empty()) {
        clipboard->putData(deviceData, m_detector->getName());
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderWaveform::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
