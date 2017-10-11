#include "DUTAnalysis.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"
#include "objects/Track.h"

using namespace corryvreckan;

DUTAnalysis::DUTAnalysis(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_digitalPowerPulsing = false;
}

void DUTAnalysis::initialise(Parameters* par) {

    // Pick up a copy of the parameters
    parameters = par;

    // Initialise single histograms
    tracksVersusTime = new TH1F("tracksVersusTime", "tracksVersusTime", 300000, 0, 300);
    associatedTracksVersusTime = new TH1F("associatedTracksVersusTime", "associatedTracksVersusTime", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "residualsX", 400, -0.2, 0.2);
    residualsY = new TH1F("residualsY", "residualsY", 400, -0.2, 0.2);
    residualsTime = new TH1F("residualsTime", "residualsTime", 2000, -0.000001, 0.000001);

    hTrackCorrelationX = new TH1F("hTrackCorrelationX", "hTrackCorrelationX", 4000, -10., 10.);
    hTrackCorrelationY = new TH1F("hTrackCorrelationY", "hTrackCorrelationY", 4000, -10., 10.);
    hTrackCorrelationTime = new TH1F("hTrackCorrelationTime", "hTrackCorrelationTime", 2000000, -0.005, 0.005);
    clusterToTVersusTime = new TH2F("clusterToTVersusTime", "clusterToTVersusTime", 300000, 0., 300., 200, 0, 1000);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime", "residualsTimeVsTime", 20000, 0, 200, 400, -0.0005, 0.0005);

    tracksVersusPowerOnTime = new TH1F("tracksVersusPowerOnTime", "tracksVersusPowerOnTime", 1200000, -0.01, 0.11);
    associatedTracksVersusPowerOnTime =
        new TH1F("associatedTracksVersusPowerOnTime", "associatedTracksVersusPowerOnTime", 1200000, -0.01, 0.11);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition", "hAssociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition", "hUnassociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);

    // Initialise member variables
    m_eventNumber = 0;
    m_nAlignmentClusters = 0;
    m_powerOnTime = 0;
    m_powerOffTime = 0;
    m_shutterOpenTime = 0;
    m_shutterCloseTime = 0;
}

StatusCode DUTAnalysis::run(Clipboard* clipboard) {

    LOG(TRACE) << "Power on time: " << m_powerOnTime / (4096. * 40000000.);
    LOG(TRACE) << "Power off time: " << m_powerOffTime / (4096. * 40000000.);

    if(parameters->currentTime < 13.5)
        return Success;

    // Timing cut for association
    double timingCut = 200. / 1000000000.; // 200 ns
    long long int timingCutInt = (timingCut * 4096. * 40000000.);

    // Spatial cut
    double spatialCut = 0.2; // 200 um

    // Track chi2/ndof cut
    double chi2ndofCut = 3.;

    // Power pulsing variable initialisation - get signals from SPIDR for this
    // device
    double timeSincePowerOn = 0.;

    // If the power was switched off/on in the last event we no longer have a
    // power on/off time
    if(m_shutterCloseTime != 0 && m_shutterCloseTime > m_shutterOpenTime)
        m_shutterOpenTime = 0;
    if(m_shutterOpenTime != 0 && m_shutterOpenTime > m_shutterCloseTime)
        m_shutterCloseTime = 0;

    // Now update the power pulsing with any new signals
    SpidrSignals* spidrData = (SpidrSignals*)clipboard->get(parameters->DUT, "SpidrSignals");
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
                if(fabs(double(signal->timestamp() - m_shutterOpenTime) / (4096. * 40000000.)) < 0.001)
                    continue;
                m_shutterOpenTime = signal->timestamp();
                LOG(TRACE) << "Shutter opened at " << double(m_shutterOpenTime) / (4096. * 40000000.);
            }
            if(signal->type() == "shutterClosed") {
                // There may be multiple power on/off in 1 time window. At the moment,
                // take earliest if within 1ms
                if(fabs(double(signal->timestamp() - m_shutterCloseTime) / (4096. * 40000000.)) < 0.001)
                    continue;
                m_shutterCloseTime = signal->timestamp();
                LOG(TRACE) << "Shutter closed at " << double(m_shutterCloseTime) / (4096. * 40000000.);
            }
        }
    }

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT clusters from the clipboard
    Clusters* clusters = (Clusters*)clipboard->get(parameters->DUT, "clusters");
    if(clusters == NULL) {
        LOG(DEBUG) << "No DUT clusters on the clipboard";
    }

    // Loop over all tracks
    bool first_track = true;
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut)
            continue;

        // Check if it intercepts the DUT
        PositionVector3D<Cartesian3D<double>> globalIntercept = parameters->detector[parameters->DUT]->getIntercept(track);
        if(!parameters->detector[parameters->DUT]->hasIntercept(track, 1.))
            continue;

        // Check that it doesn't go through/near a masked pixel
        if(parameters->detector[parameters->DUT]->hitMasked(track, 1.))
            continue;

        tracksVersusTime->Fill((double)track->timestamp() / (4096. * 40000000.));

        timeSincePowerOn = (double)(track->timestamp() - m_shutterOpenTime) / (4096. * 40000000.);
        if(timeSincePowerOn > 0. && timeSincePowerOn < 0.0002) {
            LOG(TRACE) << "Track at time " << double(track->timestamp()) / (4096. * 40000000.)
                       << " has time shutter open of " << timeSincePowerOn;
            LOG(TRACE) << "Shutter open time is " << double(m_shutterOpenTime) / (4096. * 40000000.)
                       << ", shutter close time is " << double(m_shutterCloseTime) / (4096. * 40000000.);
        }

        // Check time since power on (if power pulsing).
        // If power off time not known it will be 0. If it is known, then the track
        // should arrive before the power goes off
        if((m_shutterOpenTime != 0 && m_shutterCloseTime == 0) ||
           (m_shutterOpenTime != 0 &&
            ((m_shutterCloseTime > m_shutterOpenTime && m_shutterCloseTime - track->timestamp() > 0) ||
             (m_shutterOpenTime > m_shutterCloseTime && track->timestamp() - m_shutterOpenTime >= 0)))) {
            timeSincePowerOn = (double)(track->timestamp() - m_shutterOpenTime) / (4096. * 40000000.);
            tracksVersusPowerOnTime->Fill(timeSincePowerOn);
            //      if(timeSincePowerOn < (0.0002)){
            //        LOG(TRACE) <<"Track at time "<<parameters->currentTime<<" has
            //        time shutter open of "<<timeSincePowerOn;
            //        LOG(TRACE) <<"Shutter open time is
            //        "<<double(m_shutterOpenTime)/(4096.*40000000.)<<", shutter close
            //        time is "<<double(m_shutterCloseTime)/(4096.*40000000.);
            //      }
        }

        // If no DUT clusters then continue to the next track
        if(clusters == NULL)
            continue;
        /*
        // Correlation plot
        for(int itCluster=0;itCluster<clusters->size();itCluster++){

          // Get the cluster pointer
          Cluster* cluster = (*clusters)[itCluster];

          // Check if the cluster is close in time
    //      if( abs(cluster->timestamp() - track->timestamp()) > timingCutInt )
    continue;

          // Check distance between track and cluster
          ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());

          // Fill the correlation plot
          hTrackCorrelationX->Fill(intercept.X() - cluster->globalX());
          hTrackCorrelationY->Fill(intercept.Y() - cluster->globalY());
          hTrackCorrelationTime->Fill( (double)(track->timestamp() -
    cluster->timestamp()) / (4096.*40000000.));

          if( fabs(intercept.X() - cluster->globalX()) < 0.1 &&
              fabs(intercept.Y() - cluster->globalY()) < 0.1){
            residualsTime->Fill((double)(track->timestamp() - cluster->timestamp())
    / (4096.*40000000.));
            residualsTimeVsTime->Fill( (double)track->timestamp() /
    (4096.*40000000.), (double)(track->timestamp() - cluster->timestamp()) /
    (4096.*40000000.));
          }
        }
        */

        // Loop over all DUT clusters
        bool associated = false;
        for(auto& cluster : (*clusters)) {

            // Fill the tot histograms on the first run
            if(first_track == 0)
                clusterToTVersusTime->Fill((double)cluster->timestamp() / (4096. * 40000000.), cluster->tot());

            // Check if the cluster is close in time
            if(!m_digitalPowerPulsing && abs(cluster->timestamp() - track->timestamp()) > timingCutInt)
                continue;

            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
            double xdistance = intercept.X() - cluster->globalX();
            double ydistance = intercept.Y() - cluster->globalY();
            if(abs(xdistance) > spatialCut)
                continue;
            if(abs(ydistance) > spatialCut)
                continue;

            // We now have an associated cluster! Fill plots
            associated = true;
            LOG(TRACE) << "Found associated cluster";
            associatedTracksVersusTime->Fill((double)track->timestamp() / (4096. * 40000000.));
            residualsX->Fill(xdistance);
            residualsY->Fill(ydistance);
            track->addAssociatedCluster(cluster);
            m_nAlignmentClusters++;
            hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());

            // Fill power pulsing response
            if((m_shutterOpenTime != 0 && m_shutterCloseTime == 0) ||
               (m_shutterOpenTime != 0 &&
                ((m_shutterCloseTime > m_shutterOpenTime && m_shutterCloseTime - track->timestamp() > 0) ||
                 (m_shutterOpenTime > m_shutterCloseTime && track->timestamp() - m_shutterOpenTime >= 0)))) {
                associatedTracksVersusPowerOnTime->Fill(timeSincePowerOn);
            }

            // Only allow one associated cluster per track
            break;
        }

        if(!associated)
            hUnassociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());

        first_track = false;
    }

    // Increment event counter
    m_eventNumber++;

    //  if(m_nAlignmentClusters > 10000) return Failure;
    // Return value telling analysis to keep running
    return Success;
}

void DUTAnalysis::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}

// Function to check if a track goes through a given device
// bool DUTAnalysis::intercept(Track*, string device){
//
//  // Get the global intercept of the track and the device
//  ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
//
//  // Transform to the local co-ordinates
//
//  // Check if the row/column number is outside the acceptable range
//
//
//}
