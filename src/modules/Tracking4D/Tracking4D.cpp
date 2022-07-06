/**
 * @file
 * @brief Implementation of module Tracking4D
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Tracking4D.h"
#include <TCanvas.h>
#include <TDirectory.h>

#include "tools/cuts.h"
#include "tools/kdtree.h"

using namespace corryvreckan;
using namespace std;

Tracking4D::Tracking4D(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    // Backwards compatibility: also allow timing_cut to be used for time_cut_abs and spatial_cut for spatial_cut_abs
    config_.setAlias("time_cut_abs", "timing_cut", true);
    config_.setAlias("spatial_cut_abs", "spatial_cut", true);

    config_.setDefault<size_t>("min_hits_on_track", 6);
    config_.setDefault<bool>("exclude_dut", true);
    config_.setDefault<std::string>("track_model", "straightline");
    config_.setDefault<double>("momentum", Units::get<double>(5, "GeV"));
    config_.setDefault<double>("max_plot_chi2", 50.0);
    config_.setDefault<double>("volume_radiation_length", Units::get<double>(304.2, "m"));
    config_.setDefault<bool>("volume_scattering", false);
    config_.setDefault<bool>("reject_by_roi", false);
    config_.setDefault<bool>("unique_cluster_usage", false);

    if(config_.count({"time_cut_rel", "time_cut_abs"}) == 0) {
        config_.setDefault("time_cut_rel", 3.0);
    }
    if(config_.count({"spatial_cut_rel", "spatial_cut_abs"}) == 0) {
        config_.setDefault("spatial_cut_rel", 3.0);
    }

    // timing cut, relative (x * time_resolution) or absolute:
    time_cuts_ = corryvreckan::calculate_cut<double>("time_cut", config_, get_regular_detectors(true));
    // spatial cut, relative (x * spatial_resolution) or absolute:
    spatial_cuts_ = corryvreckan::calculate_cut<XYVector>("spatial_cut", config_, get_regular_detectors(true));

    min_hits_on_track_ = config_.get<size_t>("min_hits_on_track");
    exclude_DUT_ = config_.get<bool>("exclude_dut");

    require_detectors_ = config_.getArray<std::string>("require_detectors", {});
    exclude_from_seed_ = config_.getArray<std::string>("exclude_from_seed", {});
    timestamp_from_ = config_.get<std::string>("timestamp_from", {});
    if(!timestamp_from_.empty() &&
       std::find(require_detectors_.begin(), require_detectors_.end(), timestamp_from_) == require_detectors_.end()) {
        LOG(WARNING) << "Adding detector " << timestamp_from_
                     << " to list of required detectors as it provides the timestamp";
        require_detectors_.push_back(timestamp_from_);
    }

    track_model_ = config_.get<std::string>("track_model");
    momentum_ = config_.get<double>("momentum");
    max_plot_chi2_ = config_.get<double>("max_plot_chi2");
    volume_radiation_length_ = config_.get<double>("volume_radiation_length");
    use_volume_scatterer_ = config_.get<bool>("volume_scattering");
    reject_by_ROI_ = config_.get<bool>("reject_by_roi");
    unique_cluster_usage_ = config_.get<bool>("unique_cluster_usage");

    // print a warning if volumeScatterer are used as this causes fit failures
    // that are still not understood
    if(use_volume_scatterer_) {
        LOG_ONCE(WARNING) << "Taking volume scattering effects into account is still WIP and causes the GBL to fail - these "
                             "tracks are rejected";
    }

    // print a warning if beam energy < 1 GeV
    if(momentum_ < Units::get<double>(1, "GeV")) {
        LOG(WARNING) << "Beam energy is less than 1 GeV";
    }
}

void Tracking4D::initialize() {

    // Set up histograms
    std::string title = "Track #chi^{2};#chi^{2};events";
    trackChi2 = new TH1F("trackChi2", title.c_str(), 300, 0, 3 * max_plot_chi2_);
    title = "Track #chi^{2}/ndof;#chi^{2}/ndof;events";
    trackChi2ndof = new TH1F("trackChi2ndof", title.c_str(), 500, 0, max_plot_chi2_);
    title = "Clusters per track;clusters;tracks";
    clustersPerTrack = new TH1F("clustersPerTrack", title.c_str(), 10, -0.5, 9.5);
    title = "Track multiplicity;tracks;events";
    tracksPerEvent = new TH1F("tracksPerEvent", title.c_str(), 100, -0.5, 99.5);
    title = "Track angle X;angle_{x} [rad];events";
    trackAngleX = new TH1F("trackAngleX", title.c_str(), 2000, -0.01, 0.01);
    title = "Track angle Y;angle_{y} [rad];events";
    trackAngleY = new TH1F("trackAngleY", title.c_str(), 2000, -0.01, 0.01);
    title = "Track time within event;track time - event start;events";
    trackTime = new TH1F("trackTime", title.c_str(), 1000, 0, 460.8);
    title = "Track time with respect to first trigger;track time - trigger;events";
    trackTimeTrigger = new TH1F("trackTimeTrigger", title.c_str(), 1000, -230.4, 230.4);
    title = "Track time with respect to first trigger vs. track chi2;track time - trigger;track #chi^{2};events";
    trackTimeTriggerChi2 = new TH2F("trackTimeTriggerChi2", title.c_str(), 1000, -230.4, 230.4, 15, 0, 15);
    tracksVsTime = new TH1F("tracksVsTime", "Number of tracks vs. time; time [s]; # entries", 3e6, 0, 3e3);

    // Loop over all planes
    for(auto& detector : get_regular_detectors(true)) {
        auto detectorID = detector->getName();

        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(detectorID.c_str());

        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();

        title = detectorID + " kink X;kink [rad];events";
        kinkX[detectorID] = new TH1F("kinkX", title.c_str(), 500, -0.01, -0.01);
        title = detectorID + " kinkY ;kink [rad];events";
        kinkY[detectorID] = new TH1F("kinkY", title.c_str(), 500, -0.01, -0.01);

        local_intersects_[detectorID] = new TH2F("local_intersect",
                                                 "local intersect, col, row",
                                                 detector->nPixels().X(),
                                                 0,
                                                 detector->nPixels().X(),
                                                 detector->nPixels().Y(),
                                                 0,
                                                 detector->nPixels().Y());

        // Do not create plots for detectors not participating in the tracking:
        if(exclude_DUT_ && detector->isDUT()) {
            continue;
        }
        // local
        TDirectory* local_res = local_directory->mkdir("local_residuals");
        local_res->cd();
        title = detectorID + "Local Residual X;x-x_{track} [mm];events";
        residualsX_local[detectorID] =
            new TH1F("LocalResidualsX", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "Local  Residual X, cluster column width 1;x-x_{track} [mm];events";
        residualsXwidth1_local[detectorID] = new TH1F(
            "LocalResidualsXwidth1", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "Local  Residual X, cluster column width  2;x-x_{track} [mm];events";
        residualsXwidth2_local[detectorID] = new TH1F(
            "LocalResidualsXwidth2", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "Local  Residual X, cluster column width  3;x-x_{track} [mm];events";
        residualsXwidth3_local[detectorID] = new TH1F(
            "LocalResidualsXwidth3", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "Local  Residual Y;y-y_{track} [mm];events";
        residualsY_local[detectorID] =
            new TH1F("LocalResidualsY", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + "Local  Residual Y, cluster row width 1;y-y_{track} [mm];events";
        residualsYwidth1_local[detectorID] = new TH1F(
            "LocalResidualsYwidth1", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + "Local  Residual Y, cluster row width 2;y-y_{track} [mm];events";
        residualsYwidth2_local[detectorID] = new TH1F(
            "LocalResidualsYwidth2", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + "Local  Residual Y, cluster row width 3;y-y_{track} [mm];events";
        residualsYwidth3_local[detectorID] = new TH1F(
            "LocalResidualsYwidth3", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());

        title = detectorID + " Pull X;x-x_{track}/resolution;events";
        pullX_local[detectorID] = new TH1F("LocalpullX", title.c_str(), 500, -5, 5);

        title = detectorID + " Pull Y;y-y_{track}/resolution;events";
        pullY_local[detectorID] = new TH1F("Localpully", title.c_str(), 500, -5, 5);
        // global
        TDirectory* global_res = local_directory->mkdir("global_residuals");
        global_res->cd();
        title = detectorID + "global Residual X;x-x_{track} [mm];events";
        residualsX_global[detectorID] =
            new TH1F("GlobalResidualsX", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());

        title = detectorID + " global  Residual X vs. global position X;x-x_{track} [mm];x [mm]";
        residualsX_vs_positionX_global[detectorID] = new TH2F("GlobalResidualsX_vs_GlobalPositionX",
                                                              title.c_str(),
                                                              500,
                                                              -3 * detector->getPitch().X(),
                                                              3 * detector->getPitch().X(),
                                                              400,
                                                              -detector->getSize().X() / 1.5,
                                                              detector->getSize().X() / 1.5);
        title = detectorID + " global  Residual X vs. global position Y;x-x_{track} [mm];y [mm]";
        residualsX_vs_positionY_global[detectorID] = new TH2F("GlobalResidualsX_vs_GlobalPositionY",
                                                              title.c_str(),
                                                              500,
                                                              -3 * detector->getPitch().X(),
                                                              3 * detector->getPitch().X(),
                                                              400,
                                                              -detector->getSize().Y() / 1.5,
                                                              detector->getSize().Y() / 1.5);

        title = detectorID + "global  Residual X, cluster column width 1;x-x_{track} [mm];events";
        residualsXwidth1_global[detectorID] = new TH1F(
            "GlobalResidualsXwidth1", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "global  Residual X, cluster column width  2;x-x_{track} [mm];events";
        residualsXwidth2_global[detectorID] = new TH1F(
            "GlobalResidualsXwidth2", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + "global  Residual X, cluster column width  3;x-x_{track} [mm];events";
        residualsXwidth3_global[detectorID] = new TH1F(
            "GlobalResidualsXwidth3", title.c_str(), 500, -3 * detector->getPitch().X(), 3 * detector->getPitch().X());
        title = detectorID + " Pull X;x-x_{track}/resolution;events";
        pullX_global[detectorID] = new TH1F("GlobalpullX", title.c_str(), 500, -5, 5);
        title = detectorID + "global  Residual Y;y-y_{track} [mm];events";
        residualsY_global[detectorID] =
            new TH1F("GlobalResidualsY", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());

        title = detectorID + " global  Residual Y vs. global position Y;y-y_{track} [mm];y [mm]";
        residualsY_vs_positionY_global[detectorID] = new TH2F("GlobalResidualsY_vs_GlobalPositionY",
                                                              title.c_str(),
                                                              500,
                                                              -3 * detector->getPitch().Y(),
                                                              3 * detector->getPitch().Y(),
                                                              400,
                                                              -detector->getSize().Y() / 1.5,
                                                              detector->getSize().Y() / 1.5);
        title = detectorID + " global  Residual Y vs. global position X;y-y_{track} [mm];x [mm]";
        residualsY_vs_positionX_global[detectorID] = new TH2F("GlobalResidualsY_vs_GlobalPositionX",
                                                              title.c_str(),
                                                              500,
                                                              -3 * detector->getPitch().Y(),
                                                              3 * detector->getPitch().Y(),
                                                              400,
                                                              -detector->getSize().X() / 1.5,
                                                              detector->getSize().X() / 1.5);

        title = detectorID + "global  Residual Y, cluster row width 1;y-y_{track} [mm];events";
        residualsYwidth1_global[detectorID] = new TH1F(
            "GlobalResidualsYwidth1", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + "global  Residual Y, cluster row width 2;y-y_{track} [mm];events";
        residualsYwidth2_global[detectorID] = new TH1F(
            "GlobalResidualsYwidth2", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + "global  Residual Y, cluster row width 3;y-y_{track} [mm];events";
        residualsYwidth3_global[detectorID] = new TH1F(
            "GlobalResidualsYwidth3", title.c_str(), 500, -3 * detector->getPitch().Y(), 3 * detector->getPitch().Y());
        title = detectorID + " Pull Y;y-y_{track}/resolution;events";
        pullY_global[detectorID] = new TH1F("Globalpully", title.c_str(), 500, -5, 5);

        residualsZ_global[detectorID] = new TH1F("GlobalResidualsz", title.c_str(), 500, -0.1, 0.1);
        title = detectorID + "global  Residual Z, cluster row width 1;z_{track}-z [mm];events";
    }
}

double Tracking4D::calculate_average_timestamp(const Track* track) {
    double sum_weighted_time = 0;
    double sum_weights = 0;
    for(auto& cluster : track->getClusters()) {
        double weight = 1 / (time_cuts_[get_detector(cluster->getDetectorID())]);
        double time_of_flight = static_cast<double>(Units::convert(cluster->global().z(), "mm") / (299.792458));
        sum_weights += weight;
        sum_weighted_time += (static_cast<double>(Units::convert(cluster->timestamp(), "ns")) - time_of_flight) * weight;
    }
    return (sum_weighted_time / sum_weights);
}

StatusCode Tracking4D::run(const std::shared_ptr<Clipboard>& clipboard) {

    LOG(DEBUG) << "Start of event";
    // Container for all clusters, and detectors in tracking
    map<std::shared_ptr<Detector>, KDTree<Cluster>> trees;

    std::shared_ptr<Detector> reference_first, reference_last;
    for(auto& detector : get_regular_detectors(!exclude_DUT_)) {
        // Get the clusters
        auto tempClusters = clipboard->getData<Cluster>(detector->getName());
        LOG(DEBUG) << "Detector " << detector->getName() << " has " << tempClusters.size() << " clusters on the clipboard";
        if(!tempClusters.empty()) {
            // Store them
            LOG(DEBUG) << "Picked up " << tempClusters.size() << " clusters from " << detector->getName();

            trees.emplace(std::piecewise_construct, std::make_tuple(detector), std::make_tuple());
            trees[detector].buildTrees(tempClusters);

            // Get first and last detectors with clusters on them:
            if(std::find(exclude_from_seed_.begin(), exclude_from_seed_.end(), detector->getName()) ==
               exclude_from_seed_.end()) {
                if(!reference_first) {
                    reference_first = detector;
                }
                reference_last = detector;
            } else {
                LOG(DEBUG) << "Not using " << detector->getName() << " as seed as chosen by config file.";
            }
        }
    }

    // If there are no detectors then stop trying to track
    if(trees.size() < 2) {
        // Fill histogram
        tracksPerEvent->Fill(0);

        LOG(DEBUG) << "Too few hit detectors for finding a track; end of event.";
        return StatusCode::Success;
    }

    // Output track container
    TrackVector tracks;

    // Time cut for combinations of reference clusters and for reference track with additional detector
    auto time_cut_ref = std::max(time_cuts_[reference_first], time_cuts_[reference_last]);
    auto time_cut_ref_track = std::min(time_cuts_[reference_first], time_cuts_[reference_last]);
    for(auto& clusterFirst : trees[reference_first].getAllElements()) {
        for(auto& clusterLast : trees[reference_last].getAllElements()) {
            LOG(DEBUG) << "Looking at next reference cluster pair";

            if(std::fabs(clusterFirst->timestamp() - clusterLast->timestamp()) > time_cut_ref) {
                LOG(DEBUG) << "Reference clusters not within time cuts.";
                continue;
            }

            // The track finding is based on a straight line. Therefore a refTrack to extrapolate to the next plane is used
            StraightLineTrack refTrack;
            refTrack.addCluster(clusterFirst.get());
            refTrack.addCluster(clusterLast.get());
            auto averageTimestamp = calculate_average_timestamp(&refTrack);
            refTrack.setTimestamp(averageTimestamp);

            // Make a new track
            auto track = Track::Factory(track_model_);
            track->addCluster(clusterFirst.get());
            track->addCluster(clusterLast.get());

            track->setTimestamp(averageTimestamp);
            if(use_volume_scatterer_) {
                track->setVolumeScatter(volume_radiation_length_);
            }
            track->setParticleMomentum(momentum_);

            // Loop over each subsequent plane and look for a cluster within the timing cuts
            size_t detector_nr = 2;
            // Get all detectors here to also include passive layers which might contribute to scattering
            for(auto& detector : get_detectors()) {
                if(detector->isAuxiliary()) {
                    continue;
                }
                auto detectorID = detector->getName();
                LOG(TRACE) << "added material budget for " << detectorID << " at z = " << detector->displacement().z();
                track->registerPlane(
                    detectorID, detector->displacement().z(), detector->materialBudget(), detector->toLocal());

                if(detector == reference_first || detector == reference_last) {
                    continue;
                }

                if(exclude_DUT_ && detector->isDUT()) {
                    LOG(DEBUG) << "Skipping DUT plane.";
                    continue;
                }

                if(detector->isPassive()) {
                    LOG(DEBUG) << "Skipping passive plane.";
                    continue;
                }

                // Determine whether a track can still be assembled given the number of current hits and the number of
                // detectors to come. Reduces computing time.
                detector_nr++;
                if(refTrack.getNClusters() + (trees.size() - detector_nr + 1) < min_hits_on_track_) {
                    LOG(DEBUG) << "No chance to find a track - too few detectors left: " << refTrack.getNClusters() << " + "
                               << trees.size() << " - " << detector_nr << " < " << min_hits_on_track_;
                    continue;
                }

                if(trees.count(detector) == 0) {
                    LOG(TRACE) << "Skipping detector " << detector->getName() << " as it has 0 clusters.";
                    continue;
                }

                // Get all neighbors within the timing cut
                LOG(DEBUG) << "Searching for neighboring cluster on device " << detector->getName();
                LOG(DEBUG) << "- reference time is " << Units::display(refTrack.timestamp(), {"ns", "us", "s"});
                Cluster* closestCluster = nullptr;

                // Use spatial cut only as initial value (check if cluster is ellipse defined by cuts is done below):
                double closestClusterDistance = sqrt(spatial_cuts_[detector].x() * spatial_cuts_[detector].x() +
                                                     spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

                double timeCut = std::max(time_cut_ref_track, time_cuts_[detector]);
                LOG(DEBUG) << "Using timing cut of " << Units::display(timeCut, {"ns", "us", "s"});

                auto neighbors = trees[detector].getAllElementsInTimeWindow(refTrack.timestamp(), timeCut);

                LOG(DEBUG) << "- found " << neighbors.size() << " neighbors within the correct time window";

                // Now look for the spatially closest cluster on the next plane
                refTrack.fit();

                PositionVector3D<Cartesian3D<double>> interceptPoint = detector->getLocalIntercept(&refTrack);
                double interceptX = interceptPoint.X();
                double interceptY = interceptPoint.Y();

                for(size_t ne = 0; ne < neighbors.size(); ne++) {
                    auto newCluster = neighbors[ne].get();

                    // Calculate the distance to the previous plane's cluster/intercept
                    double distanceX = interceptX - newCluster->local().x();
                    double distanceY = interceptY - newCluster->local().y();
                    double distance = sqrt(distanceX * distanceX + distanceY * distanceY);

                    // Check if newCluster lies within ellipse defined by spatial cuts around intercept,
                    // following this example:
                    // https://www.geeksforgeeks.org/check-if-a-point-is-inside-outside-or-on-the-ellipse/
                    //
                    // ellipse defined by: x^2/a^2 + y^2/b^2 = 1: on ellipse,
                    //                                       > 1: outside,
                    //                                       < 1: inside
                    // Continue if outside of ellipse:

                    double norm = (distanceX * distanceX) / (spatial_cuts_[detector].x() * spatial_cuts_[detector].x()) +
                                  (distanceY * distanceY) / (spatial_cuts_[detector].y() * spatial_cuts_[detector].y());

                    if(norm > 1) {
                        LOG(DEBUG) << "Cluster outside the cuts. Normalized distance: " << norm;
                        continue;
                    }

                    // If this is the closest keep it for now
                    if(distance < closestClusterDistance) {
                        closestClusterDistance = distance;
                        closestCluster = newCluster;
                    }
                }

                if(closestCluster == nullptr) {
                    LOG(DEBUG) << "No cluster within spatial cut";
                    continue;
                }

                // Add the cluster to the track
                refTrack.addCluster(closestCluster);
                track->addCluster(closestCluster);
                averageTimestamp = calculate_average_timestamp(&refTrack);
                refTrack.setTimestamp(averageTimestamp);
                track->setTimestamp(averageTimestamp);

                LOG(DEBUG) << "- added cluster to track";
            }

            // check if track has required detector(s):
            auto foundRequiredDetector = [this](Track* t) {
                for(auto& requireDet : require_detectors_) {
                    if(!requireDet.empty() && !t->hasDetector(requireDet)) {
                        LOG(DEBUG) << "No cluster from required detector " << requireDet << " on the track.";
                        return false;
                    }
                }
                return true;
            };
            if(!foundRequiredDetector(track.get())) {
                continue;
            }

            // Now should have a track with one cluster from each plane
            if(track->getNClusters() < min_hits_on_track_) {
                LOG(DEBUG) << "Not enough clusters on the track, found " << track->getNClusters() << " but "
                           << min_hits_on_track_ << " required.";
                continue;
            }

            // Fit the track
            track->fit();

            if(reject_by_ROI_ && track->isFitted()) {
                // check if the track is within ROI for all detectors
                auto ds = get_regular_detectors(!exclude_DUT_);
                auto out_of_roi =
                    std::find_if(ds.begin(), ds.end(), [track](const auto& d) { return !d->isWithinROI(track.get()); });
                if(out_of_roi != ds.end()) {
                    LOG(DEBUG) << "Rejecting track outside of ROI of detector " << out_of_roi->get()->getName();
                    continue;
                }
            }
            // save the track
            if(track->isFitted()) {
                tracks.push_back(track);
            } else {
                LOG_N(WARNING, 100) << "Rejected a track due to failure in fitting";
                continue;
            }

            if(timestamp_from_.empty()) {
                // Improve the track timestamp by taking the average of all planes
                auto timestamp = calculate_average_timestamp(track.get());
                track->setTimestamp(timestamp);
                LOG(DEBUG) << "Using average cluster timestamp of " << Units::display(timestamp, "us")
                           << " as track timestamp.";
            } else {
                // use timestamp of required detector:
                double track_timestamp = track->getClusterFromDetector(timestamp_from_)->timestamp();
                LOG(DEBUG) << "Using timestamp of detector " << timestamp_from_
                           << " as track timestamp: " << Units::display(track_timestamp, "us");
                track->setTimestamp(track_timestamp);
            }
        }
    }

    auto duplicated_hit = [this](const Track* a, const Track* b) {
        for(auto d : get_regular_detectors(!exclude_DUT_)) {
            if(a->getClusterFromDetector(d->getName()) == b->getClusterFromDetector(d->getName()) &&
               !(b->getClusterFromDetector(d->getName()) == nullptr)) {
                LOG(DEBUG) << "Duplicated hit on " << d->getName() << ": rejecting track";
                return true;
            }
        }
        return false;
    };
    // Save the tracks on the clipboard
    if(tracks.size() > 0) {

        // if requested ensure unique usage of clusters
        if(unique_cluster_usage_ && tracks.size() > 1) {
            // sort by chi2:
            LOG_ONCE(WARNING) << "Rejecting tracks with same hits";
            std::sort(tracks.begin(), tracks.end(), [](const shared_ptr<Track> a, const shared_ptr<Track> b) {
                return (a->getChi2() / static_cast<double>(a->getNdof())) <
                       (b->getChi2() / static_cast<double>(b->getNdof()));
            });
            // remove tracks with hit that is used twice
            auto track1 = tracks.begin();
            while(track1 != tracks.end()) {
                auto track2 = track1 + 1;
                while(track2 != tracks.end()) {
                    // if hit is used twice delete the track
                    if(duplicated_hit(track2->get(), track1->get())) {
                        track2 = tracks.erase(track2);
                    } else {
                        track2++;
                    }
                }
                track1++;
            }
        }
        clipboard->putData(tracks);
    }
    for(auto track : tracks) {
        // Fill track time within event (relative to event start)
        auto event = clipboard->getEvent();
        trackTime->Fill(static_cast<double>(Units::convert(track->timestamp() - event->start(), "us")));
        auto triggers = event->triggerList();
        if(!triggers.empty()) {
            trackTimeTrigger->Fill(static_cast<double>(Units::convert(track->timestamp() - triggers.begin()->second, "us")));
            trackTimeTriggerChi2->Fill(
                static_cast<double>(Units::convert(track->timestamp() - triggers.begin()->second, "us")),
                track->getChi2ndof());
        }

        trackChi2->Fill(track->getChi2());
        clustersPerTrack->Fill(static_cast<double>(track->getNClusters()));
        trackChi2ndof->Fill(track->getChi2ndof());
        tracksVsTime->Fill(track->timestamp() / 1.0e9);
        if(!(track_model_ == "gbl")) {
            trackAngleX->Fill(atan(track->getDirection(track->getClusters().front()->detectorID()).X()));
            trackAngleY->Fill(atan(track->getDirection(track->getClusters().front()->detectorID()).Y()));
        }
        // Make residuals
        auto trackClusters = track->getClusters();
        for(auto& trackCluster : trackClusters) {
            string detectorID = trackCluster->detectorID();
            ROOT::Math::XYZPoint globalRes = track->getGlobalResidual(detectorID);
            ROOT::Math::XYPoint localRes = track->getLocalResidual(detectorID);

            residualsX_local[detectorID]->Fill(localRes.X());
            residualsX_global[detectorID]->Fill(globalRes.X());
            residualsX_vs_positionX_global[detectorID]->Fill(globalRes.X(), trackCluster->global().x());
            residualsX_vs_positionY_global[detectorID]->Fill(globalRes.X(), trackCluster->global().y());

            pullX_local[detectorID]->Fill(localRes.x() / track->getClusterFromDetector(detectorID)->errorX());
            pullX_global[detectorID]->Fill(globalRes.x() / track->getClusterFromDetector(detectorID)->errorX());

            pullY_local[detectorID]->Fill(localRes.Y() / track->getClusterFromDetector(detectorID)->errorY());
            pullY_global[detectorID]->Fill(globalRes.Y() / track->getClusterFromDetector(detectorID)->errorY());

            if(trackCluster->columnWidth() == 1) {
                residualsXwidth1_local[detectorID]->Fill(localRes.X());
                residualsXwidth1_global[detectorID]->Fill(globalRes.X());
            } else if(trackCluster->columnWidth() == 2) {
                residualsXwidth2_local[detectorID]->Fill(localRes.X());
                residualsXwidth2_global[detectorID]->Fill(globalRes.X());
            } else if(trackCluster->columnWidth() == 3) {
                residualsXwidth3_local[detectorID]->Fill(localRes.X());
                residualsXwidth3_global[detectorID]->Fill(globalRes.X());
            }

            residualsY_local[detectorID]->Fill(localRes.Y());
            residualsY_global[detectorID]->Fill(globalRes.Y());
            residualsY_vs_positionY_global[detectorID]->Fill(globalRes.Y(), trackCluster->global().y());
            residualsY_vs_positionX_global[detectorID]->Fill(globalRes.Y(), trackCluster->global().x());

            if(trackCluster->rowWidth() == 1) {
                residualsYwidth1_local[detectorID]->Fill(localRes.Y());
                residualsYwidth1_global[detectorID]->Fill(globalRes.Y());
            } else if(trackCluster->rowWidth() == 2) {
                residualsYwidth2_local[detectorID]->Fill(localRes.Y());
                residualsYwidth2_global[detectorID]->Fill(globalRes.Y());
            } else if(trackCluster->rowWidth() == 3) {
                residualsYwidth3_local[detectorID]->Fill(localRes.Y());
                residualsYwidth3_global[detectorID]->Fill(globalRes.Y());
            }
            residualsZ_global[detectorID]->Fill(globalRes.Z());
        }

        for(auto& detector : get_regular_detectors(true)) {
            auto det = detector->getName();

            auto local = detector->getLocalIntercept(track.get());
            auto row = detector->getRow(local);
            auto col = detector->getColumn(local);
            LOG(TRACE) << "Local col/row intersect of track: " << col << "\t" << row;
            local_intersects_[det]->Fill(col, row);

            if(!kinkX.count(det)) {
                LOG(WARNING) << "Skipping writing kinks due to missing init of histograms for  " << det;
                continue;
            }

            XYPoint kink = track->getKinkAt(det);
            kinkX.at(det)->Fill(kink.x());
            kinkY.at(det)->Fill(kink.y());
        }
    }
    tracksPerEvent->Fill(static_cast<double>(tracks.size()));

    LOG(DEBUG) << "End of event";
    return StatusCode::Success;
}
