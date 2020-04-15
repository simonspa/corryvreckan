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

Multiplet::Multiplet() : Track() {}

Multiplet::Multiplet(const Multiplet& multiplet) : Track(multiplet) {}

Multiplet::Multiplet(Track* upstream, Track* downstream) : Track() {
    m_upstream = upstream->clone();
    m_downstream = downstream->clone();

    for(auto& cluster : m_upstream->clusters()) {
        this->addCluster(cluster);
    }
    for(auto& cluster : m_downstream->clusters()) {
        this->addCluster(cluster);
    }
}

Multiplet::~Multiplet() {
    delete m_upstream;
    delete m_downstream;
}

void Multiplet::calculateChi2() {

    m_chi2 = m_upstream->chi2() + m_downstream->chi2() + sqrt(m_offsetAtScatterer.Dot(m_offsetAtScatterer));
    m_ndof = static_cast<double>(m_trackClusters.size()) - 4.;
    m_chi2ndof = m_chi2 / m_ndof;
}

void Multiplet::calculateResiduals() {
    for(auto c : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        m_residual[cluster->detectorID()] = cluster->global() - intercept(cluster->global().z());
    }
}

void Multiplet::fit() {

    // FIXME: Currently asking for direction of "". Should be the last detector plane -> Would enable using more generic
    // tracks
    m_positionAtScatterer = ((m_downstream->intercept(m_scattererPosition) -
                              (ROOT::Math::XYZPoint(0, 0, 0) - m_upstream->intercept(m_scattererPosition))) /
                             2.);
    m_offsetAtScatterer = m_downstream->intercept(m_scattererPosition) - m_upstream->intercept(m_scattererPosition);

    // Calculate the angle
    double slopeXup = m_upstream->direction("").X() / m_upstream->direction("").Z();
    double slopeYup = m_upstream->direction("").Y() / m_upstream->direction("").Z();
    double slopeXdown = m_downstream->direction("").X() / m_downstream->direction("").Z();
    double slopeYdown = m_downstream->direction("").Y() / m_downstream->direction("").Z();
    m_kinkAtScatterer = ROOT::Math::XYVector(slopeXdown - slopeXup, slopeYdown - slopeYup);

    this->calculateChi2();
    this->calculateResiduals();
    m_isFitted = true;
}

ROOT::Math::XYZPoint Multiplet::intercept(double z) const {
    return z == m_scattererPosition ? m_positionAtScatterer
                                    : (z < m_scattererPosition ? m_upstream->intercept(z) : m_downstream->intercept(z));
}

ROOT::Math::XYZPoint Multiplet::state(std::string detectorID) const {
    return getClusterFromDetector(detectorID)->global().z() <= m_scattererPosition ? m_upstream->state(detectorID)
                                                                                   : m_downstream->state(detectorID);
}

ROOT::Math::XYZVector Multiplet::direction(std::string detectorID) const {
    return getClusterFromDetector(detectorID)->global().z() <= m_scattererPosition ? m_upstream->direction(detectorID)
                                                                                   : m_downstream->direction(detectorID);
}

void Multiplet::print(std::ostream& out) const {
    out << "Multiplet " << this->m_scattererPosition << ", " << this->m_positionAtScatterer << ", "
        << this->m_offsetAtScatterer << ", " << this->m_kinkAtScatterer << ", " << this->m_chi2 << ", " << this->m_ndof
        << ", " << this->m_chi2ndof << ", " << this->timestamp();
}
