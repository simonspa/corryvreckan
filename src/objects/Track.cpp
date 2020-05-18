/**
 * @file
 * @brief Implementation of track base object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Track.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Plane::Plane(double z, double x_x0, std::string name) : Object(), z_(z), x_x0_(x_x0), name_(name) {}

std::type_index Plane::getBaseType() {
    return typeid(Plane);
}

double Plane::getPlanePosition() const {
    return z_;
}

double Plane::getMaterialBudget() const {
    return x_x0_;
}

bool Plane::hasCluster() const {
    return (cluster_.IsValid() && cluster_.GetObject() != nullptr);
}

const std::string& Plane::getName() const {
    return name_;
}

unsigned Plane::getGblPointPosition() const {
    return gbl_points_pos_;
}

Cluster* Plane::getCluster() const {
    if(!cluster_.IsValid() || cluster_.GetObject() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return dynamic_cast<Cluster*>(cluster_.GetObject());
}

Transform3D Plane::getToLocal() const {
    return to_local_;
}

Transform3D Plane::getToGlobal() const {
    return to_global_;
}

bool Plane::operator<(const Plane& pl) const {
    return z_ < pl.z_;
}

void Plane::setGblPointPosition(unsigned pos) {
    gbl_points_pos_ = pos;
}

void Plane::setCluster(const Cluster* cluster) {
    cluster_ = const_cast<Cluster*>(cluster);
}

void Plane::setToLocal(Transform3D toLocal) {
    to_local_ = toLocal;
}

void Plane::setToGlobal(Transform3D toGlobal) {
    to_global_ = toGlobal;
}

void Plane::print(std::ostream& os) const {
    os << "Plane at " << z_ << " with rad. length " << x_x0_ << ", name " << name_ << " and";
    if(hasCluster()) {
        os << "cluster with global pos: " << getCluster()->global();
    } else {
        os << "no clsuter";
    }
}

void Track::addCluster(const Cluster* cluster) {
    track_clusters_.push_back(const_cast<Cluster*>(cluster));
}
void Track::addAssociatedCluster(const Cluster* cluster) {
    associated_clusters_.push_back(const_cast<Cluster*>(cluster));
}

std::vector<Cluster*> Track::getClusters() const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : track_clusters_) {
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(dynamic_cast<Cluster*>(cluster.GetObject()));
    }

    // Return as a vector of pixels
    return clustervec;
}

std::vector<Cluster*> Track::getAssociatedClusters(const std::string& detectorID) const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : associated_clusters_) {
        // Check if reference is valid:
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        auto cluster_ref = dynamic_cast<Cluster*>(cluster.GetObject());
        if(cluster_ref->getDetectorID() != detectorID) {
            continue;
        }
        clustervec.emplace_back(cluster_ref);
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

double Track::getNdof() const {
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
        cl->second = const_cast<Cluster*>(cluster);
    } else {
        closest_cluster_.emplace(id, const_cast<Cluster*>(cluster));
    }
}

Cluster* Track::getClosestCluster(const std::string& id) const {
    auto cluster_it = closest_cluster_.find(id);
    auto cluster = cluster_it->second;
    if(cluster_it != closest_cluster_.end() && cluster.IsValid() && cluster.GetObject() != nullptr) {
        return dynamic_cast<Cluster*>(cluster.GetObject());
    }
    throw MissingReferenceException(typeid(*this), typeid(Cluster));
}

bool Track::isAssociated(Cluster* cluster) const {
    auto it = find_if(associated_clusters_.begin(), associated_clusters_.end(), [&cluster](TRef cl) {
        auto acl = dynamic_cast<Cluster*>(cl.GetObject());
        return acl == cluster;
    });
    if(it == associated_clusters_.end()) {
        return false;
    }
    return true;
}

bool Track::hasDetector(const std::string& detectorID) const {
    auto it = find_if(track_clusters_.begin(), track_clusters_.end(), [&detectorID](TRef cl) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        return cluster->getDetectorID() == detectorID;
    });
    if(it == track_clusters_.end()) {
        return false;
    }
    return true;
}

Cluster* Track::getClusterFromDetector(std::string detectorID) const {
    auto it = find_if(track_clusters_.begin(), track_clusters_.end(), [&detectorID](TRef cl) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        return cluster->getDetectorID() == detectorID;
    });
    if(it == track_clusters_.end()) {
        return nullptr;
    }
    return dynamic_cast<Cluster*>(it->GetObject());
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

XYPoint Track::getLocalResidual(const std::string& detectorID) const {
    return residual_local_.at(detectorID);
}

XYZPoint Track::getGlobalResidual(const std::string& detectorID) const {
    return residual_global_.at(detectorID);
}

double Track::getMaterialBudget(const std::string& detectorID) const {
    auto budget = std::find_if(planes_.begin(), planes_.end(), [&detectorID](Plane plane) {
                      return plane.getName() == detectorID;
                  })->getMaterialBudget();
    return budget;
}

ROOT::Math::XYZPoint Track::getCorrection(const std::string& detectorID) const {
    if(corrections_.count(detectorID) == 1)
        return corrections_.at(detectorID);
    else
        throw TrackError(typeid(Track), " calles correction on non existing detector " + detectorID);
}

void Track::registerPlane(Plane p) {
    planes_.push_back(p);
}

void Track::replacePlane(Plane p) {
    std::replace_if(
        planes_.begin(), planes_.end(), [&p](Plane const& plane) { return plane.getName() == p.getName(); }, std::move(p));
}

Plane* Track::getPlane(std::string detetorID) {
    auto plane =
        std::find_if(planes_.begin(), planes_.end(), [&detetorID](Plane const& p) { return p.getName() == detetorID; });
    if(plane == planes_.end())
        return nullptr;
    return &(*plane);
}

std::shared_ptr<Track> corryvreckan::Track::Factory(std::string trackModel) {
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
