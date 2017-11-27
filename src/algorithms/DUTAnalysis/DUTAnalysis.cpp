#include "DUTAnalysis.h"
#include "objects/Cluster.h"
#include "objects/MCParticle.h"
#include "objects/Pixel.h"
#include "objects/SpidrSignal.h"
#include "objects/Track.h"

using namespace corryvreckan;

DUTAnalysis::DUTAnalysis(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_DUT = m_config.get<std::string>("DUT");
    m_useMCtruth = m_config.get<bool>("useMCtruth", false);
}

void DUTAnalysis::initialise() {

    // Initialise single histograms
    tracksVersusTime = new TH1F("tracksVersusTime", "tracksVersusTime", 300000, 0, 300);
    associatedTracksVersusTime = new TH1F("associatedTracksVersusTime", "associatedTracksVersusTime", 300000, 0, 300);
    residualsX = new TH1F("residualsX", "residualsX", 400, -0.2, 0.2);
    residualsY = new TH1F("residualsY", "residualsY", 400, -0.2, 0.2);

    clusterTotAssociated = new TH1F("clusterTotAssociated", "clusterTotAssociated", 250, 0, 10000);
    clusterSizeAssociated = new TH1F("clusterSizeAssociated", "clusterSizeAssociated", 30, 0, 30);
    residualsTime = new TH1F("residualsTime", "residualsTime", 20000, -0.000001, 0.000001);

    hTrackCorrelationX = new TH1F("hTrackCorrelationX", "hTrackCorrelationX", 4000, -10., 10.);
    hTrackCorrelationY = new TH1F("hTrackCorrelationY", "hTrackCorrelationY", 4000, -10., 10.);
    hTrackCorrelationTime = new TH1F("hTrackCorrelationTime", "hTrackCorrelationTime", 2000000, -0.005, 0.005);
    clusterToTVersusTime = new TH2F("clusterToTVersusTime", "clusterToTVersusTime", 300000, 0., 300., 200, 0, 1000);

    residualsTimeVsTime = new TH2F("residualsTimeVsTime", "residualsTimeVsTime", 20000, 0, 200, 400, -0.0005, 0.0005);
    residualsTimeVsSignal =
        new TH2F("residualsTimeVsSignal", "residualsTimeVsSignal", 5000, 0, 50000, 2000, -0.000001, 0.000001);

    hAssociatedTracksGlobalPosition =
        new TH2F("hAssociatedTracksGlobalPosition", "hAssociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);
    hUnassociatedTracksGlobalPosition =
        new TH2F("hUnassociatedTracksGlobalPosition", "hUnassociatedTracksGlobalPosition", 200, -10, 10, 200, -10, 10);

    if(m_useMCtruth) {
        residualsXMCtruth = new TH1F("residualsXMCtruth", "residualsXMCtruth", 400, -0.2, 0.2);
    }

    // Initialise member variables
    m_eventNumber = 0;
    m_nAlignmentClusters = 0;
}

StatusCode DUTAnalysis::run(Clipboard* clipboard) {

    // Timing cut for association
    double timingCut = 200. / 1000000000.; // 200 ns
    long long int timingCutInt = (timingCut * 4096. * 40000000.);

    // Spatial cut
    double spatialCut = 0.2; // 200 um

    // Track chi2/ndof cut
    double chi2ndofCut = 3.;

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT clusters from the clipboard
    Clusters* clusters = (Clusters*)clipboard->get(m_DUT, "clusters");
    if(clusters == NULL) {
        LOG(DEBUG) << "No DUT clusters on the clipboard";
    }

    // Get the MC particles from the clipboard
    MCParticles* mcParticles = (MCParticles*)clipboard->get(m_DUT, "mcparticles");
    if(mcParticles == NULL) {
        LOG(DEBUG) << "No DUT MC particles on the clipboard";
    }

    // Loop over all tracks
    bool first_track = true;
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            continue;
        }

        // Get the DUT detector:
        auto detector = get_detector(m_DUT);
        // Check if it intercepts the DUT
        PositionVector3D<Cartesian3D<double>> globalIntercept = detector->getIntercept(track);
        if(!detector->hasIntercept(track, 1.)) {
            continue;
        }

        // Check that it doesn't go through/near a masked pixel
        if(detector->hitMasked(track, 1.)) {
            continue;
        }

        tracksVersusTime->Fill((double)track->timestamp() / (4096. * 40000000.));

        // If no DUT clusters then continue to the next track
        if(clusters == NULL)
            continue;

        // Correlation plot
        for(int itCluster = 0; itCluster < clusters->size(); itCluster++) {
            // Get the cluster pointer
            Cluster* cluster = (*clusters)[itCluster];

            // Check if the cluster is close in time
            if(abs(cluster->timestamp() - track->timestamp()) > timingCutInt)
                continue;

            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());

            // Fill the correlation plot
            hTrackCorrelationX->Fill(intercept.X() - cluster->globalX());
            hTrackCorrelationY->Fill(intercept.Y() - cluster->globalY());
            hTrackCorrelationTime->Fill((double)(track->timestamp() - cluster->timestamp()) / (4096. * 40000000.));

            if(fabs(intercept.X() - cluster->globalX()) < 0.1 && fabs(intercept.Y() - cluster->globalY()) < 0.1) {
                residualsTime->Fill((double)(track->timestamp() - cluster->timestamp()) / (4096. * 40000000.));
                residualsTimeVsTime->Fill((double)track->timestamp() / (4096. * 40000000.),
                                          (double)(track->timestamp() - cluster->timestamp()) / (4096. * 40000000.));
                residualsTimeVsSignal->Fill(cluster->tot(),
                                            (double)(track->timestamp() - cluster->timestamp()) / (4096. * 40000000.));
            }
        }

        // Loop over all DUT clusters
        bool associated = false;
        for(auto& cluster : (*clusters)) {

            // Fill the tot histograms on the first run
            if(first_track == 0)
                clusterToTVersusTime->Fill((double)cluster->timestamp() / (4096. * 40000000.), cluster->tot());

            // Check if the cluster is close in time
            if(abs(cluster->timestamp() - track->timestamp()) > timingCutInt)
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
            clusterTotAssociated->Fill(cluster->tot());
            clusterSizeAssociated->Fill(cluster->size());
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
                residualsXMCtruth->Fill(cluster->localX() - particlePosition.X());
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
