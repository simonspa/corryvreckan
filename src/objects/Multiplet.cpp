/**
 * @file
 * @brief Implementation of Multiplet base object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Multiplet.hpp"
#include "TMath.h"
#include "core/utils/unit.h"
#include "exceptions.h"

using namespace corryvreckan;
Multiplet::Multiplet(std::shared_ptr<Track> upstream, std::shared_ptr<Track> downstream) : Track() {
    m_upstream = upstream;
    m_downstream = downstream;

    // All clusters from up- and downstream should be referenced from this track:
    for(auto& cluster : m_upstream->getClusters()) {
        this->addCluster(cluster);
    }
    for(auto& cluster : m_downstream->getClusters()) {
        this->addCluster(cluster);
    }
}

ROOT::Math::XYPoint Multiplet::getKinkAt(std::string) const {
    return ROOT::Math::XYPoint(0, 0);
}

void Multiplet::calculateChi2() {

    chi2_ = m_upstream->getChi2() + m_downstream->getChi2() + sqrt(m_offsetAtScatterer.Dot(m_offsetAtScatterer));
    ndof_ = static_cast<double>(track_clusters_.size()) - 4.;
    chi2ndof_ = chi2_ / ndof_;
}

void Multiplet::calculateResiduals() {
    for(auto c : track_clusters_) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        residual_[cluster->detectorID()] = cluster->global() - getIntercept(cluster->global().z());
    }
}

void Multiplet::fit() {

    // FIXME: Currently asking for direction of "". Should be the last detector plane -> Would enable using more generic
    // tracks
    positionAtScatterer_ = ((m_downstream->getIntercept(m_scattererPosition) -
                             (ROOT::Math::XYZPoint(0, 0, 0) - m_upstream->getIntercept(m_scattererPosition))) /
                            2.);
    m_offsetAtScatterer = m_downstream->getIntercept(m_scattererPosition) - m_upstream->getIntercept(m_scattererPosition);

    // Calculate the angle
    double slopeXup = m_upstream->getDirection("").X() / m_upstream->getDirection("").Z();
    double slopeYup = m_upstream->getDirection("").Y() / m_upstream->getDirection("").Z();
    double slopeXdown = m_downstream->getDirection("").X() / m_downstream->getDirection("").Z();
    double slopeYdown = m_downstream->getDirection("").Y() / m_downstream->getDirection("").Z();
    kinkAtScatterer_ = ROOT::Math::XYVector(slopeXdown - slopeXup, slopeYdown - slopeYup);

    this->calculateChi2();
    this->calculateResiduals();
    isFitted_ = true;
}

ROOT::Math::XYZPoint Multiplet::getIntercept(double z) const {
    return z == m_scattererPosition
               ? positionAtScatterer_
               : (z < m_scattererPosition ? m_upstream->getIntercept(z) : m_downstream->getIntercept(z));
}

ROOT::Math::XYZPoint Multiplet::getState(std::string detectorID) const {
    return getClusterFromDetector(detectorID)->global().z() <= m_scattererPosition ? m_upstream->getState(detectorID)
                                                                                   : m_downstream->getState(detectorID);
}

ROOT::Math::XYZVector Multiplet::getDirection(std::string detectorID) const {
    return getClusterFromDetector(detectorID)->global().z() <= m_scattererPosition ? m_upstream->getDirection(detectorID)
                                                                                   : m_downstream->getDirection(detectorID);
}

void Multiplet::print(std::ostream& out) const {
    out << "Multiplet " << this->m_scattererPosition << ", " << this->positionAtScatterer_ << ", "
        << this->m_offsetAtScatterer << ", " << this->kinkAtScatterer_ << ", " << this->chi2_ << ", " << this->ndof_ << ", "
        << this->chi2ndof_ << ", " << this->timestamp();
}
