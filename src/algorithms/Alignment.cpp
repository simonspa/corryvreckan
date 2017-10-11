#include "Alignment.h"

#include <TVirtualFitter.h>

using namespace corryvreckan;
using namespace std;

Alignment::Alignment(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_numberOfTracksForAlignment = m_config.get<int>("number_of_tracks", 20000);
    nIterations = m_config.get<int>("iterations", 3);

    // Get alignment method:
    alignmentMethod = m_config.get<int>("alignmentMethod");
}

// Global container declarations
Tracks globalTracks;
string detectorToAlign;
Parameters* globalParameters;
int detNum;

void Alignment::initialise(Parameters* par) {
    // Pick up the global parameters
    parameters = par;
}

// During run, just pick up tracks and save them till the end
StatusCode Alignment::run(Clipboard* clipboard) {

    // Get the tracks
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        return Success;
    }

    // Make a local copy and store it
    for(int iTrack = 0; iTrack < tracks->size(); iTrack++) {
        Track* track = (*tracks)[iTrack];
        Track* alignmentTrack = new Track(track);
        m_alignmenttracks.push_back(alignmentTrack);
    }

    // If we have enough tracks for the alignment, tell the event loop to finish
    if(m_alignmenttracks.size() >= m_numberOfTracksForAlignment) {
        LOG(STATUS) << "Accumulated " << m_alignmenttracks.size() << " tracks, interrupting processing.";
        return Failure;
    }

    // Otherwise keep going
    return Success;
}

// ========================================
//  Minimisation functions for Minuit
// ========================================

// METHOD 0
// This method will move the detector in question, refit all of the tracks, and
// try to minimise the
// track chi2. If there were no clusters from this detector on any tracks then
// it would do nothing!
void MinimiseTrackChi2(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag) {

    // Pick up new alignment conditions
    globalParameters->detector[detectorToAlign]->displacementX(par[detNum * 6 + 0]);
    globalParameters->detector[detectorToAlign]->displacementY(par[detNum * 6 + 1]);
    globalParameters->detector[detectorToAlign]->displacementZ(par[detNum * 6 + 2]);
    globalParameters->detector[detectorToAlign]->rotationX(par[detNum * 6 + 3]);
    globalParameters->detector[detectorToAlign]->rotationY(par[detNum * 6 + 4]);
    globalParameters->detector[detectorToAlign]->rotationZ(par[detNum * 6 + 5]);

    // Apply new alignment conditions
    globalParameters->detector[detectorToAlign]->update();

    // The chi2 value to be returned
    result = 0.;

    // Loop over all tracks
    for(int iTrack = 0; iTrack < globalTracks.size(); iTrack++) {
        // Get the track
        Track* track = globalTracks[iTrack];
        // Get all clusters on the track
        Clusters trackClusters = track->clusters();
        // Find the cluster that needs to have its position recalculated
        for(int iTrackCluster = 0; iTrackCluster < trackClusters.size(); iTrackCluster++) {
            Cluster* trackCluster = trackClusters[iTrackCluster];
            string detectorID = trackCluster->detectorID();
            if(detectorID != detectorToAlign)
                continue;
            // Recalculate the global position from the local
            PositionVector3D<Cartesian3D<double>> positionLocal(
                trackCluster->localX(), trackCluster->localY(), trackCluster->localZ());
            PositionVector3D<Cartesian3D<double>> positionGlobal =
                *(globalParameters->detector[detectorID]->m_localToGlobal) * positionLocal;
            trackCluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(), positionGlobal.Z());
        }

        // Refit the track
        track->fit();

        // Add the new chi2
        result += track->chi2();
    }
}

// METHOD 1
// This method will move the detector in question and try to minimise the
// (unbiased) residuals. It uses
// the associated cluster container on the track (no refitting of the track)
void MinimiseResiduals(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag) {

    // Pick up new alignment conditions
    globalParameters->detector[globalParameters->detectorToAlign]->displacementX(par[0]);
    globalParameters->detector[globalParameters->detectorToAlign]->displacementY(par[1]);
    globalParameters->detector[globalParameters->detectorToAlign]->displacementZ(par[2]);
    globalParameters->detector[globalParameters->detectorToAlign]->rotationX(par[3]);
    globalParameters->detector[globalParameters->detectorToAlign]->rotationY(par[4]);
    globalParameters->detector[globalParameters->detectorToAlign]->rotationZ(par[5]);

    // Apply new alignment conditions
    globalParameters->detector[globalParameters->detectorToAlign]->update();

    // The chi2 value to be returned
    result = 0.;

    // Loop over all tracks
    for(int iTrack = 0; iTrack < globalTracks.size(); iTrack++) {
        // Get the track
        Track* track = globalTracks[iTrack];
        // Get all clusters on the track
        Clusters associatedClusters = track->associatedClusters();

        // Find the cluster that needs to have its position recalculated
        for(int iAssociatedCluster = 0; iAssociatedCluster < associatedClusters.size(); iAssociatedCluster++) {
            Cluster* associatedCluster = associatedClusters[iAssociatedCluster];
            string detectorID = associatedCluster->detectorID();
            if(detectorID != globalParameters->detectorToAlign)
                continue;
            // Recalculate the global position from the local
            PositionVector3D<Cartesian3D<double>> positionLocal(
                associatedCluster->localX(), associatedCluster->localY(), associatedCluster->localZ());
            PositionVector3D<Cartesian3D<double>> positionGlobal =
                *(globalParameters->detector[globalParameters->detectorToAlign]->m_localToGlobal) * positionLocal;
            // Get the track intercept with the detector
            ROOT::Math::XYZPoint intercept = track->intercept(positionGlobal.Z());
            // Calculate the residuals
            double residualX = intercept.X() - positionGlobal.X();
            double residualY = intercept.Y() - positionGlobal.Y();
            double error = associatedCluster->error();
            // Add the new residual2
            result += ((residualX * residualX + residualY * residualY) / (error * error));
        }
    }
}

// ==================================================================
//  The finalise function - effectively the brains of the alignment!
// ==================================================================

void Alignment::finalise() {

    // If not enough tracks were produced, do nothing
    // if(m_alignmenttracks.size() < m_numberOfTracksForAlignment) return;

    // Make the fitting object
    TVirtualFitter* residualFitter = TVirtualFitter::Fitter(0, 50);

    // Tell it what to minimise
    if(alignmentMethod == 0)
        residualFitter->SetFCN(MinimiseTrackChi2);
    if(alignmentMethod == 1)
        residualFitter->SetFCN(MinimiseResiduals);

    // Set the global parameters
    globalTracks = m_alignmenttracks;
    globalParameters = parameters;

    // Set the printout arguments of the fitter
    Double_t arglist[10];
    arglist[0] = 3;
    residualFitter->ExecuteCommand("SET PRINT", arglist, 1);

    // Set some fitter parameters
    arglist[0] = 1000;  // number of function calls
    arglist[1] = 0.001; // tolerance

    // This has been inserted in a temporary way. If the alignment method is 1
    // then it will align the single detector and then
    // return. This should be made into separate functions.
    if(alignmentMethod == 1) {

        // Get the name of the detector to align
        string detectorID = parameters->detectorToAlign;

        // Add the parameters to the fitter (z displacement not allowed to move!)
        residualFitter->SetParameter(
            0, (detectorID + "_displacementX").c_str(), parameters->detector[detectorID]->displacementX(), 0.01, -50, 50);
        residualFitter->SetParameter(
            1, (detectorID + "_displacementY").c_str(), parameters->detector[detectorID]->displacementY(), 0.01, -50, 50);
        residualFitter->SetParameter(
            2, (detectorID + "_displacementZ").c_str(), parameters->detector[detectorID]->displacementZ(), 0, -10, 500);
        residualFitter->SetParameter(
            3, (detectorID + "_rotationX").c_str(), parameters->detector[detectorID]->rotationX(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(
            4, (detectorID + "_rotationY").c_str(), parameters->detector[detectorID]->rotationY(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(
            5, (detectorID + "_rotationZ").c_str(), parameters->detector[detectorID]->rotationZ(), 0.001, -6.30, 6.30);

        for(int iteration = 0; iteration < nIterations; iteration++) {

            // Fit this plane (minimising global track chi2)
            residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

            // Set the alignment parameters of this plane to be the optimised values
            // from the alignment
            parameters->detector[detectorID]->displacementX(residualFitter->GetParameter(0));
            parameters->detector[detectorID]->displacementY(residualFitter->GetParameter(1));
            parameters->detector[detectorID]->displacementZ(residualFitter->GetParameter(2));
            parameters->detector[detectorID]->rotationX(residualFitter->GetParameter(3));
            parameters->detector[detectorID]->rotationY(residualFitter->GetParameter(4));
            parameters->detector[detectorID]->rotationZ(residualFitter->GetParameter(5));
        }

        // Write the output alignment file
        parameters->writeConditions();

        return;
    }

    // Loop over all planes. For each plane, set the plane alignment parameters
    // which will be varied, and
    // then minimise the track chi2 (sum of biased residuals). This means that
    // tracks are refitted with
    // each minimisation step.

    int det = 0;
    for(int iteration = 0; iteration < nIterations; iteration++) {

        det = 0;
        for(int ndet = 0; ndet < parameters->nDetectors; ndet++) {
            string detectorID = parameters->detectors[ndet];
            // Do not align the reference plane
            if(detectorID == parameters->reference)
                continue;
            if(parameters->excludedFromTracking.count(detectorID) != 0)
                continue;
            // Say that this is the detector we align
            detectorToAlign = detectorID;
            detNum = det;
            // Add the parameters to the fitter (z displacement not allowed to move!)
            residualFitter->SetParameter(det * 6 + 0,
                                         (detectorID + "_displacementX").c_str(),
                                         parameters->detector[detectorID]->displacementX(),
                                         0.01,
                                         -50,
                                         50);
            residualFitter->SetParameter(det * 6 + 1,
                                         (detectorID + "_displacementY").c_str(),
                                         parameters->detector[detectorID]->displacementY(),
                                         0.01,
                                         -50,
                                         50);
            residualFitter->SetParameter(det * 6 + 2,
                                         (detectorID + "_displacementZ").c_str(),
                                         parameters->detector[detectorID]->displacementZ(),
                                         0,
                                         -10,
                                         500);
            residualFitter->SetParameter(det * 6 + 3,
                                         (detectorID + "_rotationX").c_str(),
                                         parameters->detector[detectorID]->rotationX(),
                                         0.001,
                                         -6.30,
                                         6.30);
            residualFitter->SetParameter(det * 6 + 4,
                                         (detectorID + "_rotationY").c_str(),
                                         parameters->detector[detectorID]->rotationY(),
                                         0.001,
                                         -6.30,
                                         6.30);
            residualFitter->SetParameter(det * 6 + 5,
                                         (detectorID + "_rotationZ").c_str(),
                                         parameters->detector[detectorID]->rotationZ(),
                                         0.001,
                                         -6.30,
                                         6.30);

            // Fit this plane (minimising global track chi2)
            residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

            // Now that this device is fitted, set parameter errors to 0 so that they
            // are not fitted again
            residualFitter->SetParameter(
                det * 6 + 0, (detectorID + "_displacementX").c_str(), residualFitter->GetParameter(det * 6 + 0), 0, -50, 50);
            residualFitter->SetParameter(
                det * 6 + 1, (detectorID + "_displacementY").c_str(), residualFitter->GetParameter(det * 6 + 1), 0, -50, 50);
            residualFitter->SetParameter(det * 6 + 2,
                                         (detectorID + "_displacementZ").c_str(),
                                         residualFitter->GetParameter(det * 6 + 2),
                                         0,
                                         -10,
                                         500);
            residualFitter->SetParameter(
                det * 6 + 3, (detectorID + "_rotationX").c_str(), residualFitter->GetParameter(det * 6 + 3), 0, -6.30, 6.30);
            residualFitter->SetParameter(
                det * 6 + 4, (detectorID + "_rotationY").c_str(), residualFitter->GetParameter(det * 6 + 4), 0, -6.30, 6.30);
            residualFitter->SetParameter(
                det * 6 + 5, (detectorID + "_rotationZ").c_str(), residualFitter->GetParameter(det * 6 + 5), 0, -6.30, 6.30);

            // Set the alignment parameters of this plane to be the optimised values
            // from the alignment
            parameters->detector[detectorID]->displacementX(residualFitter->GetParameter(det * 6 + 0));
            parameters->detector[detectorID]->displacementY(residualFitter->GetParameter(det * 6 + 1));
            parameters->detector[detectorID]->displacementZ(residualFitter->GetParameter(det * 6 + 2));
            parameters->detector[detectorID]->rotationX(residualFitter->GetParameter(det * 6 + 3));
            parameters->detector[detectorID]->rotationY(residualFitter->GetParameter(det * 6 + 4));
            parameters->detector[detectorID]->rotationZ(residualFitter->GetParameter(det * 6 + 5));
            parameters->detector[detectorID]->update();
            det++;
        }
    }
    det = 0;

    // Now list the new alignment parameters
    for(int ndet = 0; ndet < parameters->nDetectors; ndet++) {
        string detectorID = parameters->detectors[ndet];
        // Do not align the reference plane
        if(detectorID == parameters->reference)
            continue;
        if(parameters->excludedFromTracking.count(detectorID) != 0)
            continue;

        // Get the alignment parameters
        double displacementX = residualFitter->GetParameter(det * 6 + 0);
        double displacementY = residualFitter->GetParameter(det * 6 + 1);
        double displacementZ = residualFitter->GetParameter(det * 6 + 2);
        double rotationX = residualFitter->GetParameter(det * 6 + 3);
        double rotationY = residualFitter->GetParameter(det * 6 + 4);
        double rotationZ = residualFitter->GetParameter(det * 6 + 5);

        LOG(INFO) << " Detector " << detectorID << " new alignment parameters: T(" << displacementX << "," << displacementY
                  << "," << displacementZ << ") R(" << rotationX << "," << rotationY << "," << rotationZ << ")";

        det++;
    }

    // Write the output alignment file
    parameters->writeConditions();
}
