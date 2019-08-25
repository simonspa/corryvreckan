/**
 * @file
 * @brief Implementation of [AlignmentDUTResidual] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AlignmentDUTResidual.h"

#include <TProfile.h>
#include <TVirtualFitter.h>

using namespace corryvreckan;

// Global container declarations
TrackVector globalTracks;
std::shared_ptr<Detector> globalDetector;

AlignmentDUTResidual::AlignmentDUTResidual(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_numberOfTracksForAlignment = m_config.get<size_t>("number_of_tracks", 20000);
    nIterations = m_config.get<size_t>("iterations", 3);

    m_pruneTracks = m_config.get<bool>("prune_tracks", false);
    m_alignPosition = m_config.get<bool>("align_position", true);
    if(m_alignPosition) {
        LOG(INFO) << "Aligning positions";
    }
    m_alignOrientation = m_config.get<bool>("align_orientation", true);
    if(m_alignOrientation) {
        LOG(INFO) << "Aligning orientations";
    }
    m_maxAssocClusters = m_config.get<size_t>("max_associated_clusters", 1);
    m_maxTrackChi2 = m_config.get<double>("max_track_chi2ndof", 10.);

    LOG(INFO) << "Aligning detector \"" << m_detector->name() << "\"";
}

void AlignmentDUTResidual::initialise() {

    auto detname = m_detector->name();
    std::string title = detname + " Residuals X;x_{track}-x [#mum];events";
    residualsXPlot = new TH1F("residualsX", title.c_str(), 1000, -500, 500);
    title = detname + " Residuals Y;y_{track}-y [#mum];events";
    residualsYPlot = new TH1F("residualsY", title.c_str(), 1000, -500, 500);
    title = detname + " Residual profile dY/X;x [#mum];y_{track}-y [#mum]";
    profile_dY_X = new TProfile("profile_dY_X", title.c_str(), 1000, -500, 500);
    title = detname + " Residual profile dY/Y;y [#mum];y_{track}-y [#mum]";
    profile_dY_Y = new TProfile("profile_dY_Y", title.c_str(), 1000, -500, 500);
    title = detname + " Residual profile dX/X;x [#mum];x_{track}-x [#mum]";
    profile_dX_X = new TProfile("profile_dX_X", title.c_str(), 1000, -500, 500);
    title = detname + " Residual profile dX/y;y [#mum];x_{track}-x [#mum]";
    profile_dX_Y = new TProfile("profile_dX_Y", title.c_str(), 1000, -500, 500);
}

StatusCode AlignmentDUTResidual::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks
    auto tracks = clipboard->get<Track>();
    if(tracks == nullptr) {
        return StatusCode::Success;
    }

    // Make a local copy and store it
    for(auto& track : (*tracks)) {

        // Apply selection to tracks for alignment
        if(m_pruneTracks) {
            // Only allow one associated cluster:
            if(track->associatedClusters().size() > m_maxAssocClusters) {
                LOG(DEBUG) << "Discarded track with " << track->associatedClusters().size() << " associated clusters";
                m_discardedtracks++;
                continue;
            }

            // Only allow tracks with certain Chi2/NDoF:
            if(track->chi2ndof() > m_maxTrackChi2) {
                LOG(DEBUG) << "Discarded track with Chi2/NDoF - " << track->chi2ndof();
                m_discardedtracks++;
                continue;
            }
        }

        Track* alignmentTrack = new Track(*track);
        m_alignmenttracks.push_back(alignmentTrack);

        // Find the cluster that needs to have its position recalculated
        for(auto& associatedCluster : track->associatedClusters()) {
            if(associatedCluster->detectorID() != m_detector->name()) {
                continue;
            }
            // Local position of the cluster
            auto position = associatedCluster->local();

            // Get the track intercept with the detector
            auto trackIntercept = m_detector->getIntercept(track);
            auto intercept = m_detector->globalToLocal(trackIntercept);

            // Calculate the local residuals
            double residualX = intercept.X() - position.X();
            double residualY = intercept.Y() - position.Y();

            // Fill the alignment residual profile plots
            residualsXPlot->Fill(static_cast<double>(Units::convert(residualX, "um")));
            residualsYPlot->Fill(static_cast<double>(Units::convert(residualY, "um")));
            profile_dY_X->Fill(static_cast<double>(Units::convert(residualY, "um")),
                               static_cast<double>(Units::convert(position.X(), "um")),
                               1);
            profile_dY_Y->Fill(static_cast<double>(Units::convert(residualY, "um")),
                               static_cast<double>(Units::convert(position.Y(), "um")),
                               1);
            profile_dX_X->Fill(static_cast<double>(Units::convert(residualX, "um")),
                               static_cast<double>(Units::convert(position.X(), "um")),
                               1);
            profile_dX_Y->Fill(static_cast<double>(Units::convert(residualX, "um")),
                               static_cast<double>(Units::convert(position.Y(), "um")),
                               1);
        }
    }

    // If we have enough tracks for the alignment, tell the event loop to finish
    if(m_alignmenttracks.size() >= m_numberOfTracksForAlignment) {
        LOG(STATUS) << "Accumulated " << m_alignmenttracks.size() << " tracks, interrupting processing.";
        if(m_discardedtracks > 0) {
            LOG(STATUS) << "Discarded " << m_discardedtracks << " input tracks.";
        }
        return StatusCode::EndRun;
    }

    // Otherwise keep going
    return StatusCode::Success;
}

// METHOD 1
// This method will move the detector in question and try to minimise the
// (unbiased) residuals. It uses
// the associated cluster container on the track (no refitting of the track)
void AlignmentDUTResidual::MinimiseResiduals(Int_t&, Double_t*, Double_t& result, Double_t* par, Int_t) {

    // Pick up new alignment conditions
    globalDetector->displacement(XYZPoint(par[0], par[1], par[2]));
    globalDetector->rotation(XYZVector(par[3], par[4], par[5]));

    // Apply new alignment conditions
    globalDetector->update();
    LOG(DEBUG) << "Updated parameters for " << globalDetector->name();

    // The chi2 value to be returned
    result = 0.;

    LOG(DEBUG) << "Looping over " << globalTracks.size() << " tracks";

    // Loop over all tracks
    for(auto& track : globalTracks) {
        LOG(TRACE) << "track has chi2 " << track->chi2();
        LOG(TRACE) << "- track has gradient x " << track->direction().X();
        LOG(TRACE) << "- track has gradient y " << track->direction().Y();

        // Find the cluster that needs to have its position recalculated
        for(auto& associatedCluster : track->associatedClusters()) {
            if(associatedCluster->detectorID() != globalDetector->name()) {
                continue;
            }

            // Get the track intercept with the detector
            auto position = associatedCluster->local();
            auto trackIntercept = globalDetector->getIntercept(track);
            auto intercept = globalDetector->globalToLocal(trackIntercept);

            /*
            // Recalculate the global position from the local
            auto positionLocal = associatedCluster->local();
            auto position = globalDetector->localToGlobal(positionLocal);

            // Get the track intercept with the detector
            ROOT::Math::XYZPoint intercept = track->intercept(position.Z());
            */

            // Calculate the residuals
            double residualX = intercept.X() - position.X();
            double residualY = intercept.Y() - position.Y();

            double error = associatedCluster->error();
            LOG(TRACE) << "- track has intercept (" << intercept.X() << "," << intercept.Y() << ")";
            LOG(DEBUG) << "- cluster has position (" << position.X() << "," << position.Y() << ")";

            double deltachi2 = ((residualX * residualX + residualY * residualY) / (error * error));
            LOG(TRACE) << "- delta chi2 = " << deltachi2;
            // Add the new residual2
            result += deltachi2;
            LOG(TRACE) << "- result is now " << result;
        }
    }
}

void AlignmentDUTResidual::finalise() {

    // Make the fitting object
    TVirtualFitter* residualFitter = TVirtualFitter::Fitter(nullptr, 50);
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

    globalDetector = m_detector;
    auto name = m_detector->name();

    size_t n_associatedClusters = 0;
    // count associated clusters:
    for(auto& track : globalTracks) {
        ClusterVector associatedClusters = track->associatedClusters();
        for(auto& associatedCluster : associatedClusters) {
            std::string detectorID = associatedCluster->detectorID();
            if(detectorID != name) {
                continue;
            }
            n_associatedClusters++;
            break;
        }
    }
    if(n_associatedClusters < globalTracks.size() / 2) {
        LOG(WARNING) << "Only " << 100 * static_cast<double>(n_associatedClusters) / static_cast<double>(globalTracks.size())
                     << "% of all tracks have associated clusters on detector " << name;
    } else {
        LOG(INFO) << 100 * static_cast<double>(n_associatedClusters) / static_cast<double>(globalTracks.size())
                  << "% of all tracks have associated clusters on detector " << name;
    }

    LOG(STATUS) << name << " initial alignment: " << std::endl
                << "T" << Units::display(m_detector->displacement(), {"mm", "um"}) << " R"
                << Units::display(m_detector->rotation(), {"deg"});

    // Add the parameters to the fitter (z displacement not allowed to move!)
    if(m_alignPosition) {
        residualFitter->SetParameter(0, (name + "_displacementX").c_str(), m_detector->displacement().X(), 0.01, -50, 50);
        residualFitter->SetParameter(1, (name + "_displacementY").c_str(), m_detector->displacement().Y(), 0.01, -50, 50);
    } else {
        residualFitter->SetParameter(0, (name + "_displacementX").c_str(), m_detector->displacement().X(), 0, -50, 50);
        residualFitter->SetParameter(1, (name + "_displacementY").c_str(), m_detector->displacement().Y(), 0, -50, 50);
    }

    // Z is never changed:
    residualFitter->SetParameter(2, (name + "_displacementZ").c_str(), m_detector->displacement().Z(), 0, -10, 500);

    if(m_alignOrientation) {
        residualFitter->SetParameter(3, (name + "_rotationX").c_str(), m_detector->rotation().X(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(4, (name + "_rotationY").c_str(), m_detector->rotation().Y(), 0.001, -6.30, 6.30);
        residualFitter->SetParameter(5, (name + "_rotationZ").c_str(), m_detector->rotation().Z(), 0.001, -6.30, 6.30);
    } else {
        residualFitter->SetParameter(3, (name + "_rotationX").c_str(), m_detector->rotation().X(), 0, -6.30, 6.30);
        residualFitter->SetParameter(4, (name + "_rotationY").c_str(), m_detector->rotation().Y(), 0, -6.30, 6.30);
        residualFitter->SetParameter(5, (name + "_rotationZ").c_str(), m_detector->rotation().Z(), 0, -6.30, 6.30);
    }

    for(size_t iteration = 0; iteration < nIterations; iteration++) {

        auto old_position = m_detector->displacement();
        auto old_orientation = m_detector->rotation();

        // Fit this plane (minimising global track chi2)
        residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

        // Set the alignment parameters of this plane to be the optimised values from the alignment
        m_detector->displacement(
            XYZPoint(residualFitter->GetParameter(0), residualFitter->GetParameter(1), residualFitter->GetParameter(2)));
        m_detector->rotation(
            XYZVector(residualFitter->GetParameter(3), residualFitter->GetParameter(4), residualFitter->GetParameter(5)));

        LOG(INFO) << m_detector->name() << "/" << iteration << " dT"
                  << Units::display(m_detector->displacement() - old_position, {"mm", "um"}) << " dR"
                  << Units::display(m_detector->rotation() - old_orientation, {"deg"});
    }

    LOG(STATUS) << m_detector->name() << " new alignment: " << std::endl
                << "T" << Units::display(m_detector->displacement(), {"mm", "um"}) << " R"
                << Units::display(m_detector->rotation(), {"deg"});
}
