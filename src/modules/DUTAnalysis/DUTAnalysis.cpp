#include "DUTAnalysis.h"
#include "objects/Cluster.h"
#include "objects/MCParticle.h"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"
#include "objects/Track.h"

using namespace corryvreckan;

DUTAnalysis::DUTAnalysis(Configuration config, Detector* detector) : Module(std::move(config), std::move(detector)) {
    m_digitalPowerPulsing = m_config.get<bool>("digitalPowerPulsing", false);
    m_useMCtruth = m_config.get<bool>("useMCtruth", false);
    chi2ndofCut = m_config.get<double>("chi2ndofCut", 3.);
}

void DUTAnalysis::initialise() {

    // Initialise single histograms
    tracksVersusTime = new TH1F("tracksVersusTime", "tracksVersusTime", 300000, 0, 300);
    associatedTracksVersusTime = new TH1F("associatedTracksVersusTime", "associatedTracksVersusTime", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "residualsX", 400, -0.2, 0.2);
    residualsXfine = new TH1F("residualsXfine", "residualsXfine", 400, -0.05, 0.05);
    residualsX1pix = new TH1F("residualsX1pix", "residualsX1pix", 400, -0.2, 0.2);
    residualsX2pix = new TH1F("residualsX2pix", "residualsX2pix", 400, -0.2, 0.2);
    residualsY = new TH1F("residualsY", "residualsY", 400, -0.2, 0.2);

    clusterTotAssociated = new TH1F("clusterTotAssociated", "clusterTotAssociated", 10000, 0, 10000);
    clusterSizeAssociated = new TH1F("clusterSizeAssociated", "clusterSizeAssociated", 30, 0, 30);
    clusterSizeAssociated_X = new TH1F("clusterSizeAssociated_X", "clusterSizeAssociated_X", 30, 0, 30);
    clusterSizeAssociated_Y = new TH1F("clusterSizeAssociated_Y", "clusterSizeAssociated_Y", 30, 0, 30);
    residualsTime = new TH1F("residualsTime", "residualsTime", 20000, -1000, +1000);

    hTrackCorrelationX = new TH1F("hTrackCorrelationX", "hTrackCorrelationX", 4000, -10., 10.);
    hTrackCorrelationY = new TH1F("hTrackCorrelationY", "hTrackCorrelationY", 4000, -10., 10.);
    hTrackCorrelationTime = new TH1F("hTrackCorrelationTime", "hTrackCorrelationTime", 2000000, -5000, 5000);
    clusterToTVersusTime = new TH2F("clusterToTVersusTime", "clusterToTVersusTime", 300000, 0., 300., 200, 0, 1000);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime", "residualsTimeVsTime", 20000, 0, 200, 1000, -1000, +1000);
    residualsTimeVsSignal = new TH2F("residualsTimeVsSignal", "residualsTimeVsSignal", 20000, 0, 100000, 1000, -1000, +1000);

    tracksVersusPowerOnTime = new TH1F("tracksVersusPowerOnTime", "tracksVersusPowerOnTime", 1200000, -0.01, 0.11);
    associatedTracksVersusPowerOnTime =
        new TH1F("associatedTracksVersusPowerOnTime", "associatedTracksVersusPowerOnTime", 1200000, -0.01, 0.11);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition", "hAssociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition", "hUnassociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);

    if(m_useMCtruth) {
        residualsXMCtruth = new TH1F("residualsXMCtruth", "residualsXMCtruth", 400, -0.2, 0.2);
        telescopeResolution = new TH1F("telescopeResolution", "telescopeResolution", 400, -0.2, 0.2);
    }

    // Initialise member variables
    m_eventNumber = 0;
    m_nAlignmentClusters = 0;
    m_powerOnTime = 0;
    m_powerOffTime = 0;
    m_shutterOpenTime = 0;
    m_shutterCloseTime = 0;
}

StatusCode DUTAnalysis::run(Clipboard* clipboard) {

    LOG(TRACE) << "Power on time: " << Units::display(m_powerOnTime, {"ns", "us", "s"});
    LOG(TRACE) << "Power off time: " << Units::display(m_powerOffTime, {"ns", "us", "s"});

    // Get the DUT detector:
    auto detector = get_dut();
    auto dutname = detector->name();

    //    if(clipboard->get_persistent("eventStart") < 13.5)
    //        return Success;

    // Track chi2/ndof cut
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
    SpidrSignals* spidrData = reinterpret_cast<SpidrSignals*>(clipboard->get(dutname, "SpidrSignals"));
    // If there are new signals
    if(spidrData != nullptr) {
        // Loop over all signals registered
        for(auto& signal : (*spidrData)) {
            // Register the power on or power off time, and whether the shutter is
            // open or not
            if(signal->type() == "shutterOpen") {
                // There may be multiple power on/off in 1 time window. At the moment,
                // take earliest if within 1ms
                if(std::abs(Units::convert(signal->timestamp() - m_shutterOpenTime, "s")) < 0.001) {
                    continue;
                }
                m_shutterOpenTime = signal->timestamp();
                LOG(TRACE) << "Shutter opened at " << Units::display(m_shutterOpenTime, {"ns", "us", "s"});
            }
            if(signal->type() == "shutterClosed") {
                // There may be multiple power on/off in 1 time window. At the moment,
                // take earliest if within 1ms
                if(std::abs(Units::convert(signal->timestamp() - m_shutterCloseTime, "s")) < 0.001) {
                    continue;
                }
                m_shutterCloseTime = signal->timestamp();
                LOG(TRACE) << "Shutter closed at " << Units::display(m_shutterCloseTime, {"ns", "us", "s"});
            }
        }
    }

    // Get the tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT clusters from the clipboard
    Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(dutname, "clusters"));
    if(clusters == nullptr) {
        LOG(DEBUG) << "No DUT clusters on the clipboard";
    }

    // Get the MC particles from the clipboard
    MCParticles* mcParticles = reinterpret_cast<MCParticles*>(clipboard->get(dutname, "mcparticles"));
    if(mcParticles == nullptr) {
        LOG(DEBUG) << "No DUT MC particles on the clipboard";
    }

    // Loop over all tracks
    bool first_track = true;
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            continue;
        }

        // Check if it intercepts the DUT
        PositionVector3D<Cartesian3D<double>> globalIntercept = detector->getIntercept(track);
        if(!detector->hasIntercept(track, 1.)) {
            continue;
        }

        // Check that it doesn't go through/near a masked pixel
        if(detector->hitMasked(track, 1.)) {
            continue;
        }

        tracksVersusTime->Fill(static_cast<double>(Units::convert(track->timestamp(), "s")));

        timeSincePowerOn = track->timestamp() - m_shutterOpenTime;
        if(timeSincePowerOn > 0. && timeSincePowerOn < Units::convert(200, "us")) {
            LOG(TRACE) << "Track at time " << Units::display(track->timestamp(), {"ns", "us", "s"})
                       << " has time shutter open of " << Units::display(timeSincePowerOn, {"ns", "us", "s"});
            LOG(TRACE) << "Shutter open time is " << Units::display(m_shutterOpenTime, {"ns", "us", "s"})
                       << ", shutter close time is " << Units::display(m_shutterCloseTime, {"ns", "us", "s"});
        }

        // Check time since power on (if power pulsing).
        // If power off time not known it will be 0. If it is known, then the track
        // should arrive before the power goes off
        if((m_shutterOpenTime != 0 && m_shutterCloseTime == 0) ||
           (m_shutterOpenTime != 0 &&
            ((m_shutterCloseTime > m_shutterOpenTime && m_shutterCloseTime - track->timestamp() > 0) ||
             (m_shutterOpenTime > m_shutterCloseTime && track->timestamp() - m_shutterOpenTime >= 0)))) {
            timeSincePowerOn = track->timestamp() - m_shutterOpenTime;
            tracksVersusPowerOnTime->Fill(timeSincePowerOn);

            if(timeSincePowerOn < 200000) {
                LOG(TRACE) << "Track at time " << Units::display(clipboard->get_persistent("eventStart"), {"ns", "us", "s"})
                           << " has time shutter open of " << Units::display(timeSincePowerOn, {"ns", "us", "s"});
                LOG(TRACE) << "Shutter open time is " << Units::display(m_shutterOpenTime, {"ns", "us", "s"})
                           << ", shutter close time is " << Units::display(m_shutterCloseTime, {"ns", "us", "s"});
            }
        }

        // If no DUT clusters then continue to the next track
        if(clusters == nullptr)
            continue;

        // Correlation plot
        for(auto& cluster : (*clusters)) {
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());

            // Fill the correlation plot
            hTrackCorrelationX->Fill(intercept.X() - cluster->globalX());
            hTrackCorrelationY->Fill(intercept.Y() - cluster->globalY());
            hTrackCorrelationTime->Fill(
                static_cast<double>(Units::convert(track->timestamp() - cluster->timestamp(), "ns")));

            if(fabs(intercept.X() - cluster->globalX()) < 0.1 && fabs(intercept.Y() - cluster->globalY()) < 0.1) {
                residualsTime->Fill(static_cast<double>(Units::convert(track->timestamp() - cluster->timestamp(), "ns")));
                residualsTimeVsTime->Fill(
                    static_cast<double>(Units::convert(track->timestamp(), "ns")),
                    static_cast<double>(Units::convert(track->timestamp() - cluster->timestamp(), "ns")));
                residualsTimeVsSignal->Fill(
                    cluster->tot(), static_cast<double>(Units::convert(track->timestamp() - cluster->timestamp(), "ns")));
            }
        }

        // Loop over all DUT clusters
        bool associated = false;
        for(auto& cluster : (*clusters)) {

            // Fill the tot histograms on the first run
            if(first_track == 0) {
                clusterToTVersusTime->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "ns")), cluster->tot());
            }

            if(!track->isAssociated(cluster)) {
                LOG(DEBUG) << "No associated cluster found";
                continue;
            }

            LOG(DEBUG) << "Found associated cluster";
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
            double xdistance = intercept.X() - cluster->globalX();
            double ydistance = intercept.Y() - cluster->globalY();

            // We now have an associated cluster! Fill plots
            associated = true;
            associatedTracksVersusTime->Fill(static_cast<double>(Units::convert(track->timestamp(), "s")));
            residualsX->Fill(xdistance);
            residualsXfine->Fill(xdistance);
            if(cluster->size() == 1)
                residualsX1pix->Fill(xdistance);
            if(cluster->size() == 2)
                residualsX2pix->Fill(xdistance);
            residualsY->Fill(ydistance);
            clusterTotAssociated->Fill(cluster->tot());
            clusterSizeAssociated->Fill(static_cast<double>(cluster->size()));
            clusterSizeAssociated_X->Fill(cluster->columnWidth());
            clusterSizeAssociated_Y->Fill(cluster->rowWidth());
            track->addAssociatedCluster(cluster);
            m_nAlignmentClusters++;
            hAssociatedTracksGlobalPosition->Fill(globalIntercept.X(), globalIntercept.Y());

            // Get associated MC particle info and plot
            if(m_useMCtruth) {
                if(mcParticles == nullptr)
                    continue;
                // Find the closest MC particle
                double smallestDistance(DBL_MAX);
                ROOT::Math::XYZPoint particlePosition;
                for(auto& particle : (*mcParticles)) {
                    ROOT::Math::XYZPoint entry = particle->getLocalStart();
                    ROOT::Math::XYZPoint exit = particle->getLocalEnd();
                    ROOT::Math::XYZPoint centre(
                        (entry.X() + exit.X()) / 2., (entry.Y() + exit.Y()) / 2., (entry.Z() + exit.Z()) / 2.);
                    double distance = sqrt((centre.X() - cluster->localX()) * (centre.X() - cluster->localX()) +
                                           (centre.Y() - cluster->localY()) * (centre.Y() - cluster->localY()));
                    if(distance < smallestDistance) {
                        particlePosition.SetXYZ(centre.X(), centre.Y(), centre.Z());
                    }
                }
                auto cluster_det = get_detector(cluster->detectorID());
                residualsXMCtruth->Fill(cluster->localX() + cluster_det->size().X() / 2 - particlePosition.X());
                auto interceptLocal = cluster_det->globalToLocal(intercept);
                telescopeResolution->Fill(interceptLocal.X() + cluster_det->size().X() / 2 - particlePosition.X());
            }

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
