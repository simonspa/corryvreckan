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

Cluster* Plane::cluster() const {
    if(!m_cluster.IsValid() || m_cluster.GetObject() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return dynamic_cast<Cluster*>(m_cluster.GetObject());
}

Track::Track() : m_momentum(-1) {}

Track::Track(const Track& track) : Object(track.detectorID(), track.timestamp()) {
    m_isFitted = track.isFitted();
    m_chi2 = track.chi2();
    m_ndof = track.ndof();
    m_chi2ndof = track.chi2ndof();
    //    m_planes = track.m_planes;
    for(auto& p : track.m_planes)
        m_planes.push_back(Plane(p));
    m_use_volume_scatter = track.m_use_volume_scatter;
    auto trackClusters = track.clusters();
    for(auto& track_cluster : trackClusters) {
        Cluster* cluster = new Cluster(*track_cluster);
        addCluster(cluster);
    }
    auto associatedClusters = track.m_associatedClusters;
    for(auto& assoc_cluster : associatedClusters) {
        Cluster* cluster = new Cluster(*dynamic_cast<Cluster*>(assoc_cluster.GetObject()));
        addAssociatedCluster(cluster);
    }
    m_scattering_length_volume = track.m_scattering_length_volume;
    m_residual = track.m_residual;
    m_kink = track.m_kink;
    m_momentum = track.m_momentum;
    m_corrections = track.m_corrections;
}

void Track::addCluster(const Cluster* cluster) {
    m_trackClusters.push_back(const_cast<Cluster*>(cluster));
}
void Track::addAssociatedCluster(const Cluster* cluster) {
    m_associatedClusters.push_back(const_cast<Cluster*>(cluster));
}

std::vector<Cluster*> Track::clusters() const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : m_trackClusters) {
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(dynamic_cast<Cluster*>(cluster.GetObject()));
    }

    // Return as a vector of pixels
    return clustervec;
}

std::vector<Cluster*> Track::associatedClusters(const std::string& detectorID) const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : m_associatedClusters) {
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
    return (closestCluster.find(detectorID) != closestCluster.end());
}

double Track::chi2() const {
    if(!m_isFitted) {
        throw RequestParameterBeforeFitError(this, "chi2");
    }
    return m_chi2;
}

double Track::chi2ndof() const {
    if(!m_isFitted) {
        throw RequestParameterBeforeFitError(this, "chi2ndof");
    }
    return m_chi2ndof;
}

double Track::ndof() const {
    if(!m_isFitted) {
        throw RequestParameterBeforeFitError(this, "ndof");
    }
    return m_ndof;
}

void Track::setClosestCluster(const Cluster* cluster) {
    auto id = cluster->getDetectorID();

    // Check if this detector has a closest cluster and overwrite it:
    auto cl = closestCluster.find(id);
    if(cl != closestCluster.end()) {
        cl->second = const_cast<Cluster*>(cluster);
    } else {
        closestCluster.emplace(id, const_cast<Cluster*>(cluster));
    }
}

Cluster* Track::getClosestCluster(const std::string& id) const {
    auto cluster_it = closestCluster.find(id);
    auto cluster = cluster_it->second;
    if(cluster_it != closestCluster.end() && cluster.IsValid() && cluster.GetObject() != nullptr) {
        return dynamic_cast<Cluster*>(cluster.GetObject());
    }
    throw MissingReferenceException(typeid(*this), typeid(Cluster));
}

bool Track::isAssociated(Cluster* cluster) const {
    auto it = find_if(m_associatedClusters.begin(), m_associatedClusters.end(), [&cluster](TRef cl) {
        auto acl = dynamic_cast<Cluster*>(cl.GetObject());
        return acl == cluster;
    });
    if(it == m_associatedClusters.end()) {
        return false;
    }
    return true;
}

bool Track::hasDetector(std::string detectorID) const {
    auto it = find_if(m_trackClusters.begin(), m_trackClusters.end(), [&detectorID](TRef cl) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        return cluster->getDetectorID() == detectorID;
    });
    if(it == m_trackClusters.end()) {
        return false;
    }
    return true;
}

Cluster* Track::getClusterFromDetector(std::string detectorID) const {
    auto it = find_if(m_trackClusters.begin(), m_trackClusters.end(), [&detectorID](TRef cl) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        return cluster->getDetectorID() == detectorID;
    });
    if(it == m_trackClusters.end()) {
        return nullptr;
    }
    return dynamic_cast<Cluster*>(it->GetObject());
}

ROOT::Math::XYPoint Track::kink(std::string detectorID) const {
    if(m_kink.count(detectorID) == 1) {
        return m_kink.at(detectorID);
    } else {
        return ROOT::Math::XYPoint(0, 0);
    }
}

void Track::updatePlane(Plane p) {
    std::replace_if(
        m_planes.begin(), m_planes.end(), [&p](auto const& plane) { return plane.name() == p.name(); }, std::move(p));
}

ROOT::Math::XYZPoint Track::correction(std::string detectorID) const {
    if(m_corrections.count(detectorID) == 1)
        return m_corrections.at(detectorID);
    else
        throw TrackError(typeid(Track), " calles correction on non existing detector " + detectorID);
}

Track* corryvreckan::Track::Factory(std::string trackModel) {
    if(trackModel == "straightline") {

        return new StraightLineTrack();
    } else if(trackModel == "gbl") {
        return new GblTrack();
    } else {
        throw UnknownTrackModel(typeid(Track), trackModel);
    }
}
