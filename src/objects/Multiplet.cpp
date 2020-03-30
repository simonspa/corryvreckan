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
    m_upstream = upstream;
    m_downstream = downstream;
}

void Multiplet::calculateChi2() {

    m_chi2 = m_upstream->chi2() + m_downstream->chi2() + sqrt(m_offsetAtScatterer.Dot(m_offsetAtScatterer));
}

void Multiplet::fit() {

    // FIXME: Currently asking for direction of "". Should be the last detector plane -> Would enable using more generic
    // tracks
    m_offsetAtScatterer = m_downstream->intercept(m_scattererPosition) - m_upstream->intercept(m_scattererPosition);

    // Calculate the angle
    double slopeXup = m_upstream->direction("").X() / m_upstream->direction("").Z();
    double slopeYup = m_upstream->direction("").Y() / m_upstream->direction("").Z();
    double slopeXdown = m_downstream->direction("").X() / m_downstream->direction("").Z();
    double slopeYdown = m_downstream->direction("").Y() / m_downstream->direction("").Z();
    m_kinkAtScatterer = ROOT::Math::XYVector(slopeXdown - slopeXup, slopeYdown - slopeYup);

    this->calculateChi2();
}

ROOT::Math::XYZPoint Multiplet::intercept(double z) const {
    return z <= m_scattererPosition ? m_upstream->intercept(z) : m_downstream->intercept(z);
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
    out << "Multiplet " << this->m_scattererPosition << ", " << this->m_offsetAtScatterer << ", " << this->m_kinkAtScatterer
        << ", " << this->m_chi2 << ", " << this->timestamp();
}
