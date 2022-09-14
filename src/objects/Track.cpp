/**
 * @file
 * @brief Implementation of track base object
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Track.hpp"
#include "exceptions.h"

#include "core/utils/type.h"

using namespace corryvreckan;

Track::Plane::Plane(std::string name, double z, double x_x0, Transform3D to_local)
    : z_(z), x_x0_(x_x0), name_(std::move(name)), to_local_(to_local) {}

double Track::Plane::getPosition() const {
    return z_;
}

double Track::Plane::getMaterialBudget() const {
    return x_x0_;
}

bool Track::Plane::hasCluster() const {
    return (cluster_.get() != nullptr);
}

const std::string& Track::Plane::getName() const {
    return name_;
}

Cluster* Track::Plane::getCluster() const {
    auto* cluster = cluster_.get();
    if(cluster == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return cluster;
}

Transform3D Track::Plane::getToLocal() const {
    return to_local_;
}

Transform3D Track::Plane::getToGlobal() const {
    return to_local_.Inverse();
}

bool Track::Plane::operator<(const Plane& pl) const {
    return z_ < pl.z_;
}

void Track::Plane::setCluster(const Cluster* cluster) {
    cluster_ = PointerWrapper<Cluster>(cluster);
}

void Track::Plane::print(std::ostream& os) const {
    os << "Plane at " << z_ << " with rad. length " << x_x0_ << ", name " << name_ << " and";
    if(hasCluster()) {
        os << "cluster with global pos: " << getCluster()->global();
    } else {
        os << "without cluster";
    }
}

void Track::Plane::loadHistory() {
    cluster_.get();
}
void Track::Plane::petrifyHistory() {
    cluster_.store();
}

void Track::addCluster(const Cluster* cluster) {
    track_clusters_.emplace_back(const_cast<Cluster*>(cluster));
}
void Track::addAssociatedCluster(const Cluster* cluster) {
    associated_clusters_[cluster->getDetectorID()].emplace_back(const_cast<Cluster*>(cluster));
}

std::vector<Cluster*> Track::getClusters() const {
    std::vector<Cluster*> clustervec;
    for(const auto& cl : track_clusters_) {
        auto* cluster = cl.get();
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(cluster);
    }

    // Return as a vector of pixels
    return clustervec;
}

std::vector<Cluster*> Track::getAssociatedClusters(const std::string& detectorID) const {
    std::vector<Cluster*> clustervec;
    if(associated_clusters_.find(detectorID) == associated_clusters_.end()) {
        return clustervec;
    }
    for(const auto& cl : associated_clusters_.at(detectorID)) {
        auto* cluster = cl.get();
        // Check if reference is valid:
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        clustervec.emplace_back(cluster);
    }

    // Return as a vector of pixels
    return clustervec;
}

bool Track::hasClosestCluster(const std::string& detectorID) const {
    return (closest_cluster_.find(detectorID) != closest_cluster_.end());
}

void Track::print(std::ostream& out) const {
    out << "Base class - nothing to see here" << std::endl;
}

void Track::setParticleMomentum(double p) {
    momentum_ = p;
}

double Track::getChi2() const {
    if(!isFitted_) {
        throw RequestParameterBeforeFitError(this, "chi2");
    }
    return chi2_;
}

double Track::getChi2ndof() const {
    if(!isFitted_) {
        throw RequestParameterBeforeFitError(this, "chi2ndof");
    }
    return chi2ndof_;
}

size_t Track::getNdof() const {
    if(!isFitted_) {
        throw RequestParameterBeforeFitError(this, "ndof");
    }
    return ndof_;
}

void Track::setClosestCluster(const Cluster* cluster) {
    auto id = cluster->getDetectorID();

    // Check if this detector has a closest cluster and overwrite it:
    auto cl = closest_cluster_.find(id);
    if(cl != closest_cluster_.end()) {
        cl->second = PointerWrapper<Cluster>(cluster);
    } else {
        closest_cluster_.emplace(id, const_cast<Cluster*>(cluster));
    }
}

Cluster* Track::getClosestCluster(const std::string& id) const {
    auto cluster_it = closest_cluster_.find(id);
    auto* cluster = cluster_it->second.get();
    if(cluster_it != closest_cluster_.end() && cluster != nullptr) {
        return cluster;
    }
    throw MissingReferenceException(typeid(*this), typeid(Cluster));
}

bool Track::isAssociated(Cluster* cluster) const {
    auto detectorID = cluster->getDetectorID();
    if(associated_clusters_.find(detectorID) == associated_clusters_.end()) {
        return false;
    }
    auto it = find_if(associated_clusters_.at(detectorID).begin(),
                      associated_clusters_.at(detectorID).end(),
                      [&cluster](auto& cl) { return cl.get() == cluster; });
    if(it == associated_clusters_.at(detectorID).end()) {
        return false;
    }
    return true;
}

bool Track::hasDetector(const std::string& detectorID) const {
    auto it = find_if(track_clusters_.begin(), track_clusters_.end(), [&detectorID](auto& cl) {
        return cl.get()->getDetectorID() == detectorID;
    });
    if(it == track_clusters_.end()) {
        return false;
    }
    return true;
}

Cluster* Track::getClusterFromDetector(std::string detectorID) const {
    auto it = find_if(track_clusters_.begin(), track_clusters_.end(), [&detectorID](auto& cl) {
        return cl.get()->getDetectorID() == detectorID;
    });
    if(it == track_clusters_.end()) {
        return nullptr;
    }
    return it->get();
}

XYZPoint Track::getIntercept(double) const {
    return ROOT::Math::XYZPoint(0.0, 0.0, 0.0);
}

XYZPoint Track::getState(const std::string&) const {
    return ROOT::Math::XYZPoint(0.0, 0.0, 0.0);
}

XYZVector Track::getDirection(const std::string&) const {
    return ROOT::Math::XYZVector(0.0, 0.0, 0.0);
}
XYZVector Track::getDirection(const double&) const {
    return ROOT::Math::XYZVector(0.0, 0.0, 0.0);
}

XYPoint Track::getLocalResidual(const std::string& detectorID) const {
    return residual_local_.at(detectorID);
}

XYZPoint Track::getGlobalResidual(const std::string& detectorID) const {
    return residual_global_.at(detectorID);
}

double Track::getMaterialBudget(const std::string& detectorID) const {
    auto budget = std::find_if(planes_.begin(), planes_.end(), [&detectorID](const Plane& plane) {
                      return plane.getName() == detectorID;
                  })->getMaterialBudget();
    return budget;
}

void Track::registerPlane(const std::string& name, double z, double x0, Transform3D g2l) {
    Plane p(name, z, x0, g2l);
    auto pl =
        std::find_if(planes_.begin(), planes_.end(), [&p](const Plane& plane) { return plane.getName() == p.getName(); });
    if(pl == planes_.end()) {
        planes_.push_back(std::move(p));
    } else {
        *pl = std::move(p);
    }
}

Track::Plane* Track::get_plane(std::string detetorID) {
    auto plane =
        std::find_if(planes_.begin(), planes_.end(), [&detetorID](Plane const& p) { return p.getName() == detetorID; });
    if(plane == planes_.end()) {
        return nullptr;
    }
    return &(*plane);
}

std::shared_ptr<Track> corryvreckan::Track::Factory(const std::string& trackModel) {
    if(trackModel == "straightline") {
        return std::make_shared<StraightLineTrack>();
    } else if(trackModel == "gbl") {
        return std::make_shared<GblTrack>();
    } else {
        throw UnknownTrackModel(typeid(Track), trackModel);
    }
}

std::type_index Track::getBaseType() {
    return typeid(Track);
}

std::string Track::getType() const {
    return corryvreckan::demangle(typeid(*this).name());
}

void Track::loadHistory() {
    std::for_each(planes_.begin(), planes_.end(), [](auto& n) { n.loadHistory(); });

    std::for_each(track_clusters_.begin(), track_clusters_.end(), [](auto& n) { n.get(); });
    for(auto& [detectorID, associated_clusters_det] : associated_clusters_) {
        std::for_each(associated_clusters_det.begin(), associated_clusters_det.end(), [](auto& n) { n.get(); });
    }
    std::for_each(closest_cluster_.begin(), closest_cluster_.end(), [](auto& n) { n.second.get(); });
}
void Track::petrifyHistory() {
    std::for_each(planes_.begin(), planes_.end(), [](auto& n) { n.petrifyHistory(); });

    std::for_each(track_clusters_.begin(), track_clusters_.end(), [](auto& n) { n.store(); });
    for(auto& [detectorID, associated_clusters_det] : associated_clusters_) {
        std::for_each(associated_clusters_det.begin(), associated_clusters_det.end(), [](auto& n) { n.store(); });
    }
    std::for_each(closest_cluster_.begin(), closest_cluster_.end(), [](auto& n) { n.second.store(); });
}
