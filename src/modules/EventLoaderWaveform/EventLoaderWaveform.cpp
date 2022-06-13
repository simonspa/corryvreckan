/**
 * @file
 * @brief Implementation of module EventLoaderWaveform
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
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
    m_columns = config_.getArray<int>("columns");
    m_rows = config_.getArray<int>("rows");

    if(m_channels.size() != m_columns.size() || m_columns.size() != m_rows.size()) {
        throw InvalidValueError(config, "channels", "Invalid number of channels or pixels.");
    }

    LOG(DEBUG) << "Directory " << m_inputDirectory;
}

void EventLoaderWaveform::initialize() {

    for(auto& detector : get_detectors()) {
        LOG(DEBUG) << "Initialise for detector " + detector->getName();
    }

    // Initialise member variables
    m_eventNumber = 0;
    m_triggerNumber = 1;
    m_loader = std::make_unique<DirectoryLoader>(m_inputDirectory, m_channels, m_columns, m_rows, m_detector->getName());
}

StatusCode EventLoaderWaveform::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto event = clipboard->getEvent();
    auto triggers = event->triggerList();

    WaveformVector deviceData;

    for(const auto& trigger : triggers) {
        if(trigger.first == m_triggerNumber) {
            auto waveform = m_loader->read(trigger);
            m_triggerNumber++;

            LOG(DEBUG) << "Loading waveforms for trigger " << trigger.first;
            deviceData.insert(deviceData.end(), waveform.begin(), waveform.end());
        } else if(trigger.first > m_triggerNumber) {
            while(trigger.first > m_triggerNumber) {
                LOG(DEBUG) << "Skipping waveform for trigger " << m_triggerNumber;
                auto w = m_loader->read(trigger);
                m_triggerNumber++;
            }
            auto waveform = m_loader->read(trigger);
            m_triggerNumber++;

            LOG(DEBUG) << "Loading waveforms for trigger " << trigger.first;
            deviceData.insert(deviceData.end(), waveform.begin(), waveform.end());
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
