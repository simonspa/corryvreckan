#include "Alignment.h"

#include <TVirtualFitter.h>
#include <numeric>

using namespace corryvreckan;
using namespace std;

// Global container declarations
Tracks globalTracks;
std::string detectorToAlign;
Detector* globalDetector;
int detNum;

Alignment::Alignment(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_numberOfTracksForAlignment = m_config.get<int>("number_of_tracks", 20000);
    nIterations = m_config.get<int>("iterations", 3);

    // Get alignment method:
    alignmentMethod = m_config.get<int>("alignmentMethod");

    if(m_config.has("detectorToAlign")) {
        detectorToAlign = m_config.get<std::string>("detectorToAlign");
    } else {
        detectorToAlign = m_config.get<std::string>("DUT");
    }
    LOG(INFO) << "Aligning detector \"" << detectorToAlign << "\"";
}

void Alignment::initialise() {
    if(alignmentMethod == 1) {
        auto detector = get_detector(detectorToAlign);
        auto detname = detector->name();
        std::string title = detname + "_residualsX";
        residualsXPlot = new TH1F(title.c_str(), title.c_str(), 400, -0.2, 0.2);
        title = detname + "_residualsY";
        residualsYPlot = new TH1F(title.c_str(), title.c_str(), 400, -0.2, 0.2);
        title = detname + "_profile_dY_X";
        profile_dY_X = new TProfile(title.c_str(), title.c_str(), 400, -0.2, 0.2);
        title = detname + "_profile_dY_Y";
        profile_dY_Y = new TProfile(title.c_str(), title.c_str(), 400, -0.2, 0.2);
        title = detname + "_profile_dX_X";
        profile_dX_X = new TProfile(title.c_str(), title.c_str(), 400, -0.2, 0.2);
        title = detname + "_profile_dX_Y";
        profile_dX_Y = new TProfile(title.c_str(), title.c_str(), 400, -0.2, 0.2);
    }
}

// During run, just pick up tracks and save them till the end
StatusCode Alignment::run(Clipboard* clipboard) {
    auto detector = get_detector(detectorToAlign);
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

        Clusters associatedClusters = track->associatedClusters();
        if(alignmentMethod == 0)
            continue;
        // Find the cluster that needs to have its position recalculated
        for(auto& associatedCluster : associatedClusters) {
            // Recalculate the global position from the local
            PositionVector3D<Cartesian3D<double>> positionLocal(
                associatedCluster->localX(), associatedCluster->localY(), associatedCluster->localZ());
            PositionVector3D<Cartesian3D<double>> positionGlobal = *(detector->localToGlobal()) * positionLocal;
            // Get the track intercept with the detector
            ROOT::Math::XYZPoint intercept = track->intercept(positionGlobal.Z());
            // Calculate the residuals
            double residualX = intercept.X() - positionGlobal.X();
            double residualY = intercept.Y() - positionGlobal.Y();
            // Fill the alignment residual profile plots
            residualsXPlot->Fill(residualX);
            residualsYPlot->Fill(residualY);
            profile_dY_X->Fill(residualY, positionLocal.X(), 1);
            profile_dY_Y->Fill(residualY, positionLocal.Y(), 1);
            profile_dX_X->Fill(residualX, positionLocal.X(), 1);
            profile_dX_Y->Fill(residualX, positionLocal.Y(), 1);
        }
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
void Alignment::MinimiseTrackChi2(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag) {

    // Pick up new alignment conditions
    globalDetector->displacementX(par[detNum * 6 + 0]);
    globalDetector->displacementY(par[detNum * 6 + 1]);
    globalDetector->displacementZ(par[detNum * 6 + 2]);
    globalDetector->rotationX(par[detNum * 6 + 3]);
    globalDetector->rotationY(par[detNum * 6 + 4]);
    globalDetector->rotationZ(par[detNum * 6 + 5]);

    // Apply new alignment conditions
    globalDetector->update();

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
            if(globalDetector->name() != trackCluster->detectorID()) {
                continue;
            }
            // Recalculate the global position from the local
            PositionVector3D<Cartesian3D<double>> positionLocal(
                trackCluster->localX(), trackCluster->localY(), trackCluster->localZ());
            PositionVector3D<Cartesian3D<double>> positionGlobal = *(globalDetector->localToGlobal()) * positionLocal;
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
void Alignment::MinimiseResiduals(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag) {

    // Pick up new alignment conditions
    globalDetector->displacementX(par[0]);
    globalDetector->displacementY(par[1]);
    globalDetector->displacementZ(par[2]);
    globalDetector->rotationX(par[3]);
    globalDetector->rotationY(par[4]);
    globalDetector->rotationZ(par[5]);

    // Apply new alignment conditions
    globalDetector->update();
    LOG(DEBUG) << "Updated parameters for " << detectorToAlign;

    // The chi2 value to be returned
    result = 0.;

    LOG(DEBUG) << "Looping over " << globalTracks.size() << " tracks";

    // Loop over all tracks
    for(auto& track : globalTracks) {
        // Get all clusters on the track
        Clusters associatedClusters = track->associatedClusters();

        LOG(DEBUG) << "- track has chi2 " << track->chi2();
        LOG(DEBUG) << "- track has gradient x " << track->m_direction.X();
        LOG(DEBUG) << "- track has gradient y " << track->m_direction.Y();

        // Find the cluster that needs to have its position recalculated
        for(auto& associatedCluster : associatedClusters) {
            string detectorID = associatedCluster->detectorID();
            if(detectorID != detectorToAlign)
                continue;
            // Recalculate the global position from the local
            PositionVector3D<Cartesian3D<double>> positionLocal(
                associatedCluster->localX(), associatedCluster->localY(), associatedCluster->localZ());
            PositionVector3D<Cartesian3D<double>> positionGlobal = *(globalDetector->localToGlobal()) * positionLocal;
            // Get the track intercept with the detector
            ROOT::Math::XYZPoint intercept = track->intercept(positionGlobal.Z());
            // Calculate the residuals
            double residualX = intercept.X() - positionGlobal.X();
            double residualY = intercept.Y() - positionGlobal.Y();
            double error = associatedCluster->error();
            LOG(DEBUG) << "- track has intercept (" << intercept.X() << "," << intercept.Y() << ")";
            LOG(DEBUG) << "- cluster has position (" << positionGlobal.X() << "," << positionGlobal.Y() << ")";
            double deltachi2 = ((residualX * residualX + residualY * residualY) / (error * error));
            LOG(DEBUG) << "- delta chi2 = " << deltachi2;
            // Add the new residual2
            result += deltachi2;
            LOG(DEBUG) << "- result is now " << result;
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

    // Set the printout arguments of the fitter
    Double_t arglist[10];
    arglist[0] = -1;
    residualFitter->ExecuteCommand("SET PRINT", arglist, 1);

    // Set some fitter parameters
    arglist[0] = 1000;  // number of function calls
    arglist[1] = 0.001; // tolerance

    // This has been inserted in a temporary way. If the alignment method is 1
    // then it will align the single detector and then
    // return. This should be made into separate functions.
    if(alignmentMethod == 1) {
        auto detector = get_detector(detectorToAlign);
        globalDetector = detector;

        int n_associatedClusters = 0;
        // count associated clusters:
        for(auto& track : globalTracks) {
            Clusters associatedClusters = track->associatedClusters();
            for(auto& associatedCluster : associatedClusters) {
                string detectorID = associatedCluster->detectorID();
                if(detectorID != detectorToAlign) {
                    continue;
                }
                n_associatedClusters++;
                break;
            }
        }
        if(n_associatedClusters < 0.5 * globalTracks.size()) {
            LOG(ERROR) << "Only " << 100 * static_cast<double>(n_associatedClusters) / globalTracks.size()
                       << "% of all tracks have associated clusters on detector " << detectorToAlign;
        } else {
            LOG(INFO) << 100 * static_cast<double>(n_associatedClusters) / globalTracks.size()
                      << "% of all tracks have associated clusters on detector " << detectorToAlign;
        }

        LOG(STATUS) << detectorToAlign << " initial alignment: " << std::endl
                    << "T" << display_vector(detector->displacement(), {"mm", "um"}) << " R"
                    << display_vector(detector->rotation(), {"deg"});

        // Add the parameters to the fitter (z displacement not allowed to move!)
        residualFitter->SetParameter(
            0, (detectorToAlign + "_displacementX").c_str(), detector->displacementX(), 0.01, -50, 50);
        residualFitter->SetParameter(
            1, (detectorToAlign + "_displacementY").c_str(), detector->displacementY(), 0.01, -50, 50);
        residualFitter->SetParameter(
            2, (detectorToAlign + "_displacementZ").c_str(), detector->displacementZ(), 0, -10, 500);
        residualFitter->SetParameter(3, (detectorToAlign + "_rotationX").c_str(), detector->rotationX(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(4, (detectorToAlign + "_rotationY").c_str(), detector->rotationY(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(5, (detectorToAlign + "_rotationZ").c_str(), detector->rotationZ(), 0.001, -6.30, 6.30);

        for(int iteration = 0; iteration < nIterations; iteration++) {

            auto old_position = detector->displacement();
            auto old_orientation = detector->rotation();

            // Fit this plane (minimising global track chi2)
            residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

            // Set the alignment parameters of this plane to be the optimised values from the alignment
            detector->displacementX(residualFitter->GetParameter(0));
            detector->displacementY(residualFitter->GetParameter(1));
            detector->displacementZ(residualFitter->GetParameter(2));
            detector->rotationX(residualFitter->GetParameter(3));
            detector->rotationY(residualFitter->GetParameter(4));
            detector->rotationZ(residualFitter->GetParameter(5));

            LOG(INFO) << detector->name() << "/" << iteration << " dT"
                      << display_vector(detector->displacement() - old_position, {"mm", "um"}) << " dR"
                      << display_vector(detector->rotation() - old_orientation, {"deg"});
        }

        LOG(STATUS) << detectorToAlign << " new alignment: " << std::endl
                    << "T" << display_vector(detector->displacement(), {"mm", "um"}) << " R"
                    << display_vector(detector->rotation(), {"deg"});

        return;
    }

    // Store the alignment shifts per detector:
    std::map<std::string, std::vector<double>> shiftsX;
    std::map<std::string, std::vector<double>> shiftsY;
    std::map<std::string, std::vector<double>> shiftsZ;
    std::map<std::string, std::vector<double>> rotX;
    std::map<std::string, std::vector<double>> rotY;
    std::map<std::string, std::vector<double>> rotZ;

    // Loop over all planes. For each plane, set the plane alignment parameters which will be varied, and then minimise the
    // track chi2 (sum of biased residuals). This means that tracks are refitted with each minimisation step.

    int det = 0;
    for(int iteration = 0; iteration < nIterations; iteration++) {

        det = 0;
        for(auto& detector : get_detectors()) {
            string detectorID = detector->name();

            // Do not align the reference plane
            if(detectorID == m_config.get<std::string>("reference") || detectorID == m_config.get<std::string>("DUT")) {
                continue;
            }

            LOG_PROGRESS(STATUS, "alignment_track")
                << "Alignment iteration " << (iteration + 1) << " of " << nIterations << ", detector " << detectorID;

            // Say that this is the detector we align
            detectorToAlign = detectorID;
            globalDetector = detector;

            detNum = det;
            // Add the parameters to the fitter (z displacement not allowed to move!)
            residualFitter->SetParameter(
                det * 6 + 0, (detectorID + "_displacementX").c_str(), detector->displacementX(), 0.01, -50, 50);
            residualFitter->SetParameter(
                det * 6 + 1, (detectorID + "_displacementY").c_str(), detector->displacementY(), 0.01, -50, 50);
            residualFitter->SetParameter(
                det * 6 + 2, (detectorID + "_displacementZ").c_str(), detector->displacementZ(), 0, -10, 500);
            residualFitter->SetParameter(
                det * 6 + 3, (detectorID + "_rotationX").c_str(), detector->rotationX(), 0.001, -6.30, 6.30);
            residualFitter->SetParameter(
                det * 6 + 4, (detectorID + "_rotationY").c_str(), detector->rotationY(), 0.001, -6.30, 6.30);
            residualFitter->SetParameter(
                det * 6 + 5, (detectorID + "_rotationZ").c_str(), detector->rotationZ(), 0.001, -6.30, 6.30);

            // Fit this plane (minimising global track chi2)
            residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

            // Retrieve fit results:
            auto displacementX = residualFitter->GetParameter(det * 6 + 0);
            auto displacementY = residualFitter->GetParameter(det * 6 + 1);
            auto displacementZ = residualFitter->GetParameter(det * 6 + 2);
            auto rotationX = residualFitter->GetParameter(det * 6 + 3);
            auto rotationY = residualFitter->GetParameter(det * 6 + 4);
            auto rotationZ = residualFitter->GetParameter(det * 6 + 5);

            // Store corrections:
            shiftsX[detectorID].push_back(detector->displacementX() - displacementX);
            shiftsY[detectorID].push_back(detector->displacementY() - displacementY);
            shiftsZ[detectorID].push_back(detector->displacementZ() - displacementZ);
            rotX[detectorID].push_back(detector->rotationX() - rotationX);
            rotY[detectorID].push_back(detector->rotationY() - rotationY);
            rotZ[detectorID].push_back(detector->rotationZ() - rotationZ);

            LOG(INFO) << detector->name() << "/" << iteration << " dT(" << (detector->displacementX() - displacementX) << ","
                      << (detector->displacementY() - displacementY) << "," << (detector->displacementZ() - displacementZ)
                      << ") dR(" << (detector->rotationX() - rotationX) << "," << (detector->rotationY() - rotationY) << ","
                      << (detector->rotationZ() - rotationZ) << ")";

            // Now that this device is fitted, set parameter errors to 0 so that they
            // are not fitted again
            residualFitter->SetParameter(det * 6 + 0, (detectorID + "_displacementX").c_str(), displacementX, 0, -50, 50);
            residualFitter->SetParameter(det * 6 + 1, (detectorID + "_displacementY").c_str(), displacementY, 0, -50, 50);
            residualFitter->SetParameter(det * 6 + 2, (detectorID + "_displacementZ").c_str(), displacementZ, 0, -10, 500);
            residualFitter->SetParameter(det * 6 + 3, (detectorID + "_rotationX").c_str(), rotationX, 0, -6.30, 6.30);
            residualFitter->SetParameter(det * 6 + 4, (detectorID + "_rotationY").c_str(), rotationY, 0, -6.30, 6.30);
            residualFitter->SetParameter(det * 6 + 5, (detectorID + "_rotationZ").c_str(), rotationZ, 0, -6.30, 6.30);

            // Set the alignment parameters of this plane to be the optimised values
            // from the alignment
            detector->displacementX(displacementX);
            detector->displacementY(displacementY);
            detector->displacementZ(displacementZ);
            detector->rotationX(rotationX);
            detector->rotationY(rotationY);
            detector->rotationZ(rotationZ);
            detector->update();
            det++;
        }
    }

    LOG_PROGRESS(STATUS, "alignment_track") << "Alignment finished, " << nIterations << " iteration.";

    // Now list the new alignment parameters
    for(auto& detector : get_detectors()) {
        // Do not align the reference plane
        if(detector->name() == m_config.get<std::string>("reference") ||
           detector->name() == m_config.get<std::string>("DUT")) {
            continue;
        }

        LOG(INFO) << detector->name() << " new alignment: T(" << detector->displacementX() << ","
                  << detector->displacementY() << "," << detector->displacementZ() << ") R(" << detector->rotationX() << ","
                  << detector->rotationY() << "," << detector->rotationZ() << ")";

        // Fill the alignment convergence graphs:
        std::vector<double> iterations(nIterations);
        std::iota(std::begin(iterations), std::end(iterations), 0);

        std::string name = "alignment_correction_displacementX_" + detector->name();
        align_correction_shiftX[detector->name()] =
            new TGraph(shiftsX[detector->name()].size(), &iterations[0], &shiftsX[detector->name()][0]);
        align_correction_shiftX[detector->name()]->Write(name.c_str());

        name = "alignment_correction_displacementY_" + detector->name();
        align_correction_shiftY[detector->name()] =
            new TGraph(shiftsY[detector->name()].size(), &iterations[0], &shiftsY[detector->name()][0]);
        align_correction_shiftY[detector->name()]->Write(name.c_str());

        name = "alignment_correction_displacementZ_" + detector->name();
        align_correction_shiftZ[detector->name()] =
            new TGraph(shiftsZ[detector->name()].size(), &iterations[0], &shiftsZ[detector->name()][0]);
        align_correction_shiftZ[detector->name()]->Write(name.c_str());

        name = "alignment_correction_rotationX_" + detector->name();
        align_correction_rotX[detector->name()] =
            new TGraph(rotX[detector->name()].size(), &iterations[0], &rotX[detector->name()][0]);
        align_correction_rotX[detector->name()]->Write(name.c_str());

        name = "alignment_correction_rotationY_" + detector->name();
        align_correction_rotY[detector->name()] =
            new TGraph(rotY[detector->name()].size(), &iterations[0], &rotY[detector->name()][0]);
        align_correction_rotY[detector->name()]->Write(name.c_str());

        name = "alignment_correction_rotationZ_" + detector->name();
        align_correction_rotZ[detector->name()] =
            new TGraph(rotZ[detector->name()].size(), &iterations[0], &rotZ[detector->name()][0]);
        align_correction_rotZ[detector->name()]->Write(name.c_str());
    }
}
