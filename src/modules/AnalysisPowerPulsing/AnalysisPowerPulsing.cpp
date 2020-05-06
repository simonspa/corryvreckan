/**
 * @file
 * @brief Implementation of module AnalysisPowerPulsing
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisPowerPulsing.h"
#include "objects/Cluster.hpp"
#include "objects/SpidrSignal.hpp"

using namespace corryvreckan;
using namespace std;

AnalysisPowerPulsing::AnalysisPowerPulsing(Configuration config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {}

void AnalysisPowerPulsing::initialise() {

    // int timeN = 1500000;
    // int timeN = 3000000;
    int timeN = 1100000;
    // int timeMin = 13;
    // int timeMax = 14;
    float timeMin = 0;
    float timeMax = 110;
    openSignalVersusTime = new TH1D("openSignalVersusTime", "openSignalVersusTime", timeN, timeMin, timeMax);
    closeSignalVersusTime = new TH1D("closeSignalVersusTime", "closeSignalVersusTime", timeN, timeMin, timeMax);
    clustersVersusTime = new TH1D("clustersVersusTime", "clustersVersusTime", timeN, timeMin, timeMax);
    clustersVersusPowerOnTime = new TH1D("clustersVersusPowerOnTime", "clustersVersusPowerOnTime", 1200000, -0.01, 0.11);
    timeSincePowerOnVersusClusterTime = new TH2D("timeSincePowerOnVersusClusterTime",
                                                 "timeSincePowerOnVersusClusterTime",
                                                 10000,
                                                 timeMin,
                                                 timeMax,
                                                 1200,
                                                 0.009,
                                                 0.0101);
    minTimeSincePowerOnVersusClusterTime =
        new TH1D("minTimeSincePowerOnVersusClusterTime", "minTimeSincePowerOnVersusClusterTime", 10000, timeMin, timeMax);
    diffMinTimeSincePowerOnVersusClusterTime = new TH1D(
        "diffMinTimeSincePowerOnVersusClusterTime", "diffMinTimeSincePowerOnVersusClusterTime", 10000, timeMin, timeMax);

    m_powerOnTime = 0;
    m_powerOffTime = 0;
    m_shutterOpenTime = 0;
    m_shutterCloseTime = 0;

    v_minTime.clear();
}

StatusCode AnalysisPowerPulsing::run(std::shared_ptr<Clipboard> clipboard) {

    LOG(TRACE) << "Power on time: " << static_cast<double>(m_powerOnTime) / (4096. * 40000000.);
    LOG(TRACE) << "Power off time: " << static_cast<double>(m_powerOffTime) / (4096. * 40000000.);
    // Power pulsing variable initialisation - get signals from SPIDR for this device
    double timeSincePowerOn = 0.;
    double timeSinceShutterClosed = 0.;

    if(m_shutterCloseTime != 0 && m_shutterCloseTime > m_shutterOpenTime)
        m_shutterOpenTime = 0;
    if(m_shutterOpenTime != 0 && m_shutterOpenTime > m_shutterCloseTime)
        m_shutterCloseTime = 0;

    /*
        // Now update the power pulsing with any new signals
        SpidrSignal* spidrData = (SpidrSignals*)clipboard->get(m_DUT, "SpidrSignals");
        // If there are new signals
        if(spidrData != NULL) {
            // Loop over all signals registered
            int nSignals = spidrData->size();
            for(int iSig = 0; iSig < nSignals; iSig++) {
                // Get the signal
                SpidrSignal* signal = (*spidrData)[iSig];
                // Register the power on or power off time, and whether the shutter is
                // open or not
                if(signal->type() == "shutterOpen") {
                    // There may be multiple power on/off in 1 time window. At the moment,
                    // take earliest if within 1ms
                    if(abs(Units::convert(signal->timestamp() - m_shutterOpenTime, "s")) < 0.001) {
                        continue;
                    }
                    m_shutterOpenTime = signal->timestamp();
                    LOG(TRACE) << "Shutter opened at " << Units::display(m_shutterOpenTime, {"ns", "us", "s"});
                }
                if(signal->type() == "shutterClosed") {
                    // There may be multiple power on/off in 1 time window. At the moment,
                    // take earliest if within 1ms
                    if(abs(Units::convert(signal->timestamp() - m_shutterCloseTime, "s")) < 0.001) {
                        continue;
                    }
                    m_shutterCloseTime = signal->timestamp();
                    LOG(TRACE) << "Shutter closed at " << Units::display(m_shutterCloseTime, {"ns", "us", "s"});
                }
            }
        }

    */

    // Now update the power pulsing with any new signals
    auto spidrData = clipboard->getData<SpidrSignal>(m_detector->getName());

    // Loop over all signals registered
    for(auto& signal : spidrData) {
        // Register the power on or power off time, and whether the shutter is
        // open or not
        if(signal->type() == "shutterOpen") {
            // There may be multiple power on/off in 1 time window. At the moment,
            // take earliest if within 1ms -> 10us
            //     if(abs(Units::convert(signal->timestamp() - m_shutterOpenTime, "s")) < 0.00001) {
            //     //if(fabs(double(signal->timestamp() - m_shutterOpenTime) / (4096. * 40000000.)) < 0.001){
            //         //LOG(WARNING) << "Removed a shutterOpen signal "<<hex<< signal <<dec<<" , too close to the
            //         previous one: "<<double(signal->timestamp() - m_shutterOpenTime) / (4096. * 40000000.);
            //         LOG(WARNING) << "Removed a shutterOpen signal "<<hex<< signal <<dec<<" , too close to the previous
            //         one: "<<Units::display(signal->timestamp() - m_shutterOpenTime,  {"ns", "us", "s"}); continue;
            //     }
            openSignalVersusTime->Fill(static_cast<double>(Units::convert(signal->timestamp(), "s")));
            // openSignalVersusTime->Fill( (double)(signal->timestamp()) / (4096. * 40000000.));
            // openSignalVersusTime->Fill( (double)(signal->timestamp() - oldOpen) / (4096. * 40000000.));
            m_shutterOpenTime = signal->timestamp();
            // LOG(DEBUG) << "Shutter opened at " << double(m_shutterOpenTime) / (4096. * 40000000.);
            LOG(DEBUG) << "Shutter opened at " << Units::display(m_shutterOpenTime, {"ns", "us"}); //, "s"});
        }
        if(signal->type() == "shutterClosed") {
            // There may be multiple power on/off in 1 time window. At the moment,
            // take earliest if within 1ms
            // if(fabs(double(signal->timestamp() - m_shutterCloseTime) / (4096. * 40000000.)) < 0.001){
            //     if(abs(Units::convert(signal->timestamp() - m_shutterCloseTime, "s")) < 0.00001){
            //         LOG(WARNING) << "Removed a shutterClose signal "<<hex<< signal <<dec<<" , too close to the
            //         previous one: "<<Units::display(signal->timestamp() - m_shutterCloseTime,  {"ns", "us", "s"});
            //         continue;
            //     }
            closeSignalVersusTime->Fill(static_cast<double>(Units::convert(signal->timestamp(), "s")));
            // closeSignalVersusTime->Fill((double)(signal->timestamp() - oldClose ) / (4096. * 40000000.));
            m_shutterCloseTime = signal->timestamp();
            // LOG(DEBUG) << "Shutter closed at " << double(m_shutterCloseTime) / (4096. * 40000000.);
            LOG(DEBUG) << "Shutter closed at " << Units::display(m_shutterCloseTime, {"ns", "us"}); //, "s"});
        }
    }

    // Get the DUT clusters from the clipboard
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());

    double minTimePerEvent = 99999.;
    double minCluster = 99999.;
    for(auto& cluster : clusters) {

        // Fill the tot histograms on the first run
        // if(first_track == 0){
        clustersVersusTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")));
        //}
        if((m_shutterOpenTime != 0 && m_shutterCloseTime == 0) ||
           (m_shutterOpenTime != 0 &&
            ((m_shutterCloseTime > m_shutterOpenTime && m_shutterCloseTime - cluster->timestamp() > 0) ||
             (m_shutterOpenTime > m_shutterCloseTime && cluster->timestamp() - m_shutterCloseTime >= 0)))) {
            //(m_shutterOpenTime > m_shutterCloseTime && cluster->timestamp() - m_shutterOpenTime >= 0)))) {
            timeSincePowerOn = static_cast<double>(Units::convert(cluster->timestamp() - m_shutterOpenTime, "s"));
            clustersVersusPowerOnTime->Fill(timeSincePowerOn);
        }
        // if (m_shutterCloseTime>m_shutterOpenTime && cluster->timestamp()>m_shutterCloseTime){ // means shutter is closed?
        if(cluster->timestamp() < m_shutterOpenTime) { // means shutter is closed?
            timeSinceShutterClosed = static_cast<double>(Units::convert(cluster->timestamp() - m_shutterCloseTime, "s"));
            timeSincePowerOnVersusClusterTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "s")),
                                                    timeSinceShutterClosed);
            //     std::cout<<" m_shutterCloseTime >? m_shutterOpenTime "<<m_shutterCloseTime<<"  "<<m_shutterOpenTime<<"
            //     cluster= "<<Units::display(cluster->timestamp(), "s")<<"  timeSinceShutterClosed=
            //     "<<timeSinceShutterClosed<<std::endl;
            if(timeSinceShutterClosed < minTimePerEvent) {
                minTimePerEvent = timeSinceShutterClosed;
                minCluster = static_cast<double>(Units::convert(cluster->timestamp(), "s"));
            }
        }
    }
    if(minTimePerEvent < 10) {
        minTimeSincePowerOnVersusClusterTime->Fill(minCluster, minTimePerEvent);
        if(v_minTime.size() > 0)
            diffMinTimeSincePowerOnVersusClusterTime->Fill(minCluster, minTimePerEvent - v_minTime.back());
        v_minTime.push_back(minTimePerEvent);
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
