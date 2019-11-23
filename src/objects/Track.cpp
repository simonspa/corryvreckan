#include "Track.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Track::Track() : m_momentum(-1), m_trackModel("baseClass") {
    std::cout << "Call" << std::endl;
}

Track::Track(const Track& track) : Object(track.detectorID(), track.timestamp()) {
    std::cout << "Call2" << std::endl;

    m_trackModel = track.trackModel();
    m_isFitted = track.isFitted();
    if(m_isFitted) {
        m_chi2 = track.chi2();
        m_ndof = track.ndof();
        m_chi2ndof = track.chi2ndof();
    }
    auto trackClusters = track.clusters();
    for(auto& track_cluster : trackClusters) {
        Cluster* cluster = new Cluster(*track_cluster);
        addCluster(cluster);
    }
    auto associatedClusters = track.associatedClusters();
    for(auto& assoc_cluster : associatedClusters) {
        Cluster* cluster = new Cluster(*assoc_cluster);
        addAssociatedCluster(cluster);
    }
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

std::vector<Cluster*> Track::associatedClusters() const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : m_associatedClusters) {
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(dynamic_cast<Cluster*>(cluster.GetObject()));
    }

    // Return as a vector of pixels
    return clustervec;
}

bool Track::hasClosestCluster() const {
    return closestCluster != nullptr;
}

void Track::setClosestCluster(const Cluster* cluster) {
    closestCluster = const_cast<Cluster*>(cluster);
}

Cluster* Track::getClosestCluster() const {
    return dynamic_cast<Cluster*>(closestCluster.GetObject());
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

Track* corryvreckan::Track::Factory(std::string trackModel) {
    if(trackModel == "straightline") {
        return new StraightLineTrack();
    } else {
        throw MissingReferenceException(typeid(Track), typeid(Track));
    }
}

Track* Track::Factory(const Track& track) {
    if(track.trackModel() == "straightline") {
        return new StraightLineTrack(track);
    } else {
        throw MissingReferenceException(typeid(Track), typeid(track));
    }
}
