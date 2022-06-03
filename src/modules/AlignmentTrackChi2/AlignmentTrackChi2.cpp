/**
 * @file
 * @brief Implementation of module AlignmentTrackChi2
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AlignmentTrackChi2.h"

#include <TVirtualFitter.h>
#include <numeric>

using namespace corryvreckan;
using namespace std;

// Global container declarations
TrackVector AlignmentTrackChi2::globalTracks;
std::shared_ptr<Detector> AlignmentTrackChi2::globalDetector;
int AlignmentTrackChi2::detNum;
ThreadPool* AlignmentTrackChi2::thread_pool;

AlignmentTrackChi2::AlignmentTrackChi2(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {
    config_.setDefault<size_t>("iterations", 3);
    config_.setDefault<bool>("prune_tracks", false);
    config_.setDefault<bool>("align_position", true);
    config_.setDefault<bool>("align_orientation", true);
    config_.setDefault<size_t>("max_associated_clusters", 1);
    config_.setDefault<double>("max_track_chi2ndof", 10.);
    config_.setDefault<unsigned int>("workers", std::max(std::thread::hardware_concurrency() - 1, 1u));

    m_workers = config.get<unsigned int>("workers");
    nIterations = config_.get<size_t>("iterations");
    m_pruneTracks = config_.get<bool>("prune_tracks");

    m_alignPosition = config_.get<bool>("align_position");
    if(m_alignPosition) {
        LOG(INFO) << "Aligning positions";
    }

    m_alignOrientation = config_.get<bool>("align_orientation");
    if(m_alignOrientation) {
        LOG(INFO) << "Aligning orientations";
    }

    m_maxAssocClusters = config_.get<size_t>("max_associated_clusters");
    m_maxTrackChi2 = config_.get<double>("max_track_chi2ndof");
    LOG(INFO) << "Aligning telescope";
}

// During run, just pick up tracks and save them till the end
StatusCode AlignmentTrackChi2::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the tracks
    auto tracks = clipboard->getData<Track>();
    TrackVector alignmenttracks;
    std::map<std::string, std::vector<Cluster*>> alignmentclusters;

    // Make a local copy and store it
    for(auto& track : tracks) {

        // Apply selection to tracks for alignment - only allow tracks with certain Chi2/NDoF:
        if(m_pruneTracks && track->getChi2ndof() > m_maxTrackChi2) {
            LOG(DEBUG) << "Discarded track with Chi2/NDoF - " << track->getChi2ndof();
            m_discardedtracks++;
            continue;
        }

        LOG(TRACE) << "Storing track with track model \"" << track->getType() << "\" for alignment";
        alignmenttracks.push_back(track);
        for(auto& cluster : track->getClusters()) {
            alignmentclusters[cluster->detectorID()].push_back(cluster);
        }
    }

    // Store all tracks we want for alignment on the permanent storage:
    clipboard->putPersistentData(alignmenttracks);
    // Copy the objects of all track clusters on the clipboard to persistent storage:
    for(auto& clusters : alignmentclusters) {
        clipboard->copyToPersistentData(clusters.second, clusters.first);
    }

    // Otherwise keep going
    return StatusCode::Success;
}

// ========================================
//  Minimisation functions for Minuit
// ========================================

// METHOD 0
// This method will move the detector in question, refit all of the tracks, and
// try to minimise the
// track chi2. If there were no clusters from this detector on any tracks then
// it would do nothing!
void AlignmentTrackChi2::MinimiseTrackChi2(Int_t&, Double_t*, Double_t& result, Double_t* par, Int_t) {

    static size_t fitIterations = 0;
    static string detName = "";
    LOG(DEBUG) << AlignmentTrackChi2::globalDetector->displacement() << "' " << globalDetector->rotation();
    // Pick up new alignment conditions
    AlignmentTrackChi2::globalDetector->displacement(
        XYZPoint(par[detNum * 6 + 0], par[detNum * 6 + 1], par[detNum * 6 + 2]));
    AlignmentTrackChi2::globalDetector->rotation(XYZVector(par[detNum * 6 + 3], par[detNum * 6 + 4], par[detNum * 6 + 5]));

    // Apply new alignment conditions
    AlignmentTrackChi2::globalDetector->update();

    // The chi2 value to be returned
    result = 0.;

    std::vector<std::shared_future<double>> result_futures;
    auto track_refit = [&](auto& track) {
        // Get all clusters on the track
        auto trackClusters = track->getClusters();
        // Find the cluster that needs to have its position recalculated
        for(size_t iTrackCluster = 0; iTrackCluster < trackClusters.size(); iTrackCluster++) {
            Cluster* trackCluster = trackClusters[iTrackCluster];
            if(AlignmentTrackChi2::globalDetector->getName() != trackCluster->detectorID()) {
                continue;
            }

            // Recalculate the global position from the local
            auto positionLocal = trackCluster->local();
            auto positionGlobal = AlignmentTrackChi2::globalDetector->localToGlobal(positionLocal);
            trackCluster->setClusterCentre(positionGlobal);
            LOG(DEBUG) << "Updating cluster with corrected global position for detector "
                       << AlignmentTrackChi2::globalDetector->getName();
        }

        // Refit the track
        track->registerPlane(AlignmentTrackChi2::globalDetector->getName(),
                             AlignmentTrackChi2::globalDetector->displacement().z(),
                             AlignmentTrackChi2::globalDetector->materialBudget(),
                             AlignmentTrackChi2::globalDetector->toLocal());
        LOG(DEBUG) << "Updated transformations for detector " << AlignmentTrackChi2::globalDetector->getName();
        if(detName != AlignmentTrackChi2::globalDetector->getName()) {
            detName = AlignmentTrackChi2::globalDetector->getName();
            fitIterations = 0;
        }

        track->fit();

        // Add the new chi2
        return track->getChi2();
    };

    // Loop over all tracks
    for(auto& track : AlignmentTrackChi2::globalTracks) {
        result_futures.push_back(AlignmentTrackChi2::thread_pool->submit(track_refit, track));
    }

    for(auto& result_future : result_futures) {
        result += result_future.get();
    }

    LOG_PROGRESS(INFO, "t") << "Refit of " << result_futures.size() << " track, MINUIT iteration " << fitIterations;
    fitIterations++;
    AlignmentTrackChi2::thread_pool->wait();
}

// ==================================================================
//  The finalise function - effectively the brains of the alignment!
// ==================================================================

void AlignmentTrackChi2::finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) {

    if(m_discardedtracks > 0) {
        LOG(INFO) << "Discarded " << m_discardedtracks << " input tracks.";
    }

    // Make the fitting object
    TVirtualFitter* residualFitter = TVirtualFitter::Fitter(nullptr, 50);
    residualFitter->SetFCN(MinimiseTrackChi2);

    // Set the global parameters
    AlignmentTrackChi2::globalTracks = clipboard->getPersistentData<Track>();

    // Create thread pool:
    ThreadPool::registerThreadCount(m_workers);
    AlignmentTrackChi2::thread_pool =
        new ThreadPool(m_workers,
                       m_workers * 1024,
                       [log_level = corryvreckan::Log::getReportingLevel(), log_format = corryvreckan::Log::getFormat()]() {
                           // clang-format on
                           // Initialize the threads to the same log level and format as the master setting
                           corryvreckan::Log::setReportingLevel(log_level);
                           corryvreckan::Log::setFormat(log_format);
                       });

    // Set the printout arguments of the fitter
    Double_t arglist[10];
    arglist[0] = -1;
    residualFitter->ExecuteCommand("SET PRINT", arglist, 1);

    // Set some fitter parameters
    arglist[0] = 1000;  // number of function calls
    arglist[1] = 0.001; // tolerance

    // Store the alignment shifts per detector:
    std::map<std::string, std::vector<double>> shiftsX;
    std::map<std::string, std::vector<double>> shiftsY;
    std::map<std::string, std::vector<double>> rotX;
    std::map<std::string, std::vector<double>> rotY;
    std::map<std::string, std::vector<double>> rotZ;

    // Loop over all planes. For each plane, set the plane alignment parameters which will be varied, and then minimise the
    // track chi2 (sum of biased residuals). This means that tracks are refitted with each minimisation step.
    for(size_t iteration = 0; iteration < nIterations; iteration++) {

        LOG_PROGRESS(STATUS, "alignment_track") << "Alignment iteration " << (iteration + 1) << " of " << nIterations;

        int det = 0;
        for(auto& detector : get_regular_detectors(false)) {
            string detectorID = detector->getName();

            // Do not align the reference plane
            if(detector->isReference()) {
                LOG(DEBUG) << "Skipping detector " << detector->getName();
                continue;
            }

            // Say that this is the detector we align
            AlignmentTrackChi2::globalDetector = detector;

            detNum = det;
            // Add the parameters to the fitter (z displacement not allowed to move!)
            if(m_alignPosition) {
                residualFitter->SetParameter(
                    det * 6 + 0, (detectorID + "_displacementX").c_str(), detector->displacement().X(), 0.01, -50, 50);
                residualFitter->SetParameter(
                    det * 6 + 1, (detectorID + "_displacementY").c_str(), detector->displacement().Y(), 0.01, -50, 50);

            } else {
                residualFitter->SetParameter(
                    det * 6 + 0, (detectorID + "_displacementX").c_str(), detector->displacement().X(), 0, -50, 50);
                residualFitter->SetParameter(
                    det * 6 + 1, (detectorID + "_displacementY").c_str(), detector->displacement().Y(), 0, -50, 50);
            }
            residualFitter->SetParameter(
                det * 6 + 2, (detectorID + "_displacementZ").c_str(), detector->displacement().Z(), 0, -10, 500);

            if(m_alignOrientation) {
                residualFitter->SetParameter(
                    det * 6 + 3, (detectorID + "_rotationX").c_str(), detector->rotation().X(), 0.001, -6.30, 6.30);
                residualFitter->SetParameter(
                    det * 6 + 4, (detectorID + "_rotationY").c_str(), detector->rotation().Y(), 0.001, -6.30, 6.30);
                residualFitter->SetParameter(
                    det * 6 + 5, (detectorID + "_rotationZ").c_str(), detector->rotation().Z(), 0.001, -6.30, 6.30);
            } else {
                residualFitter->SetParameter(
                    det * 6 + 3, (detectorID + "_rotationX").c_str(), detector->rotation().X(), 0, -6.30, 6.30);
                residualFitter->SetParameter(
                    det * 6 + 4, (detectorID + "_rotationY").c_str(), detector->rotation().Y(), 0, -6.30, 6.30);
                residualFitter->SetParameter(
                    det * 6 + 5, (detectorID + "_rotationZ").c_str(), detector->rotation().Z(), 0, -6.30, 6.30);
            }
            auto old_position = detector->displacement();
            auto old_orientation = detector->rotation();

            // Fit this plane (minimising global track chi2)
            LOG(DEBUG) << "fitting residuals for detetcor " << detector->getName();
            residualFitter->ExecuteCommand("MIGRAD", arglist, 2);

            // Retrieve fit results:
            auto displacementX = residualFitter->GetParameter(det * 6 + 0);
            auto displacementY = residualFitter->GetParameter(det * 6 + 1);
            auto displacementZ = residualFitter->GetParameter(det * 6 + 2);
            auto rotationX = residualFitter->GetParameter(det * 6 + 3);
            auto rotationY = residualFitter->GetParameter(det * 6 + 4);
            auto rotationZ = residualFitter->GetParameter(det * 6 + 5);

            // Store corrections:
            shiftsX[detectorID].push_back(
                static_cast<double>(Units::convert(detector->displacement().X() - old_position.X(), "um")));
            shiftsY[detectorID].push_back(
                static_cast<double>(Units::convert(detector->displacement().Y() - old_position.Y(), "um")));
            rotX[detectorID].push_back(
                static_cast<double>(Units::convert(detector->rotation().X() - old_orientation.X(), "deg")));
            rotY[detectorID].push_back(
                static_cast<double>(Units::convert(detector->rotation().Y() - old_orientation.Y(), "deg")));
            rotZ[detectorID].push_back(
                static_cast<double>(Units::convert(detector->rotation().Z() - old_orientation.Z(), "deg")));

            LOG(INFO) << detector->getName() << "/" << iteration << " dT"
                      << Units::display(detector->displacement() - old_position, {"mm", "um"}) << " dR"
                      << Units::display(detector->rotation() - old_orientation, {"deg"});

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
            detector->displacement(XYZPoint(displacementX, displacementY, displacementZ));
            detector->rotation(XYZVector(rotationX, rotationY, rotationZ));
            detector->update();
            det++;
        }
    }

    LOG_PROGRESS(STATUS, "alignment_track") << "Alignment finished, " << nIterations << " iteration.";

    // Now list the new alignment parameters
    for(auto& detector : get_regular_detectors(false)) {
        // Do not align the reference plane
        if(detector->isReference()) {
            continue;
        }

        LOG(STATUS) << detector->getName() << " new alignment: " << std::endl
                    << "T" << Units::display(detector->displacement(), {"mm", "um"}) << " R"
                    << Units::display(detector->rotation(), {"deg"});

        // Fill the alignment convergence graphs:
        std::vector<double> iterations(nIterations);
        std::iota(std::begin(iterations), std::end(iterations), 0);

        std::string name = "alignment_correction_displacementX_" + detector->getName();
        align_correction_shiftX[detector->getName()] = new TGraph(
            static_cast<int>(shiftsX[detector->getName()].size()), &iterations[0], &shiftsX[detector->getName()][0]);
        align_correction_shiftX[detector->getName()]->GetXaxis()->SetTitle("# iteration");
        align_correction_shiftX[detector->getName()]->GetYaxis()->SetTitle("correction [#mum]");
        align_correction_shiftX[detector->getName()]->Write(name.c_str());

        name = "alignment_correction_displacementY_" + detector->getName();
        align_correction_shiftY[detector->getName()] = new TGraph(
            static_cast<int>(shiftsY[detector->getName()].size()), &iterations[0], &shiftsY[detector->getName()][0]);
        align_correction_shiftY[detector->getName()]->GetXaxis()->SetTitle("# iteration");
        align_correction_shiftY[detector->getName()]->GetYaxis()->SetTitle("correction [#mum]");
        align_correction_shiftY[detector->getName()]->Write(name.c_str());

        name = "alignment_correction_rotationX_" + detector->getName();
        align_correction_rotX[detector->getName()] =
            new TGraph(static_cast<int>(rotX[detector->getName()].size()), &iterations[0], &rotX[detector->getName()][0]);
        align_correction_rotX[detector->getName()]->GetXaxis()->SetTitle("# iteration");
        align_correction_rotX[detector->getName()]->GetYaxis()->SetTitle("correction [deg]");
        align_correction_rotX[detector->getName()]->Write(name.c_str());

        name = "alignment_correction_rotationY_" + detector->getName();
        align_correction_rotY[detector->getName()] =
            new TGraph(static_cast<int>(rotY[detector->getName()].size()), &iterations[0], &rotY[detector->getName()][0]);
        align_correction_rotY[detector->getName()]->GetXaxis()->SetTitle("# iteration");
        align_correction_rotY[detector->getName()]->GetYaxis()->SetTitle("correction [deg]");
        align_correction_rotY[detector->getName()]->Write(name.c_str());

        name = "alignment_correction_rotationZ_" + detector->getName();
        align_correction_rotZ[detector->getName()] =
            new TGraph(static_cast<int>(rotZ[detector->getName()].size()), &iterations[0], &rotZ[detector->getName()][0]);
        align_correction_rotZ[detector->getName()]->GetXaxis()->SetTitle("# iteration");
        align_correction_rotZ[detector->getName()]->GetYaxis()->SetTitle("correction [deg]");
        align_correction_rotZ[detector->getName()]->Write(name.c_str());
    }

    // Clean up local track storage
    AlignmentTrackChi2::globalTracks.clear();
    AlignmentTrackChi2::globalDetector.reset();
}
