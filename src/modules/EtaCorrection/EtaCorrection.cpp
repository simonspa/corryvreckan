/**
 * @file
 * @brief Implementation of module EtaCorrection
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EtaCorrection.h"

using namespace corryvreckan;
using namespace std;

EtaCorrection::EtaCorrection(Configuration config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {
    m_etaFormulaX = config_.get<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = config_.get<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
}

void EtaCorrection::initialise() {

    // Initialise histograms
    double pitchX = m_detector->getPitch().X();
    double pitchY = m_detector->getPitch().Y();

    // Get info from configuration:
    std::vector<double> m_etaConstantsX = config_.getArray<double>("eta_constants_x_" + m_detector->getName(), {});
    std::vector<double> m_etaConstantsY = config_.getArray<double>("eta_constants_y_" + m_detector->getName(), {});
    if(!m_etaConstantsX.empty() || !m_etaConstantsY.empty()) {
        LOG(INFO) << "Found Eta correction factors for detector \"" << m_detector->getName()
                  << "\": " << (m_etaConstantsX.empty() ? "" : "X ") << (m_etaConstantsY.empty() ? "" : "Y ");
    }

    if(!m_etaConstantsX.empty()) {
        m_correctX = true;
        m_etaCorrectorX = new TF1("etaCorrectorX", m_etaFormulaX.c_str(), -pitchX / 2., pitchX / 2.);
        for(size_t x = 0; x < m_etaConstantsX.size(); x++) {
            m_etaCorrectorX->SetParameter(static_cast<int>(x), m_etaConstantsX[x]);
        }
    } else {
        m_correctX = false;
    }

    if(!m_etaConstantsY.empty()) {
        m_correctY = true;
        m_etaCorrectorY = new TF1("etaCorrectorY", m_etaFormulaY.c_str(), -pitchY / 2., pitchY / 2.);
        for(size_t y = 0; y < m_etaConstantsY.size(); y++)
            m_etaCorrectorY->SetParameter(static_cast<int>(y), m_etaConstantsY[y]);
    } else {
        m_correctY = false;
    }
}

void EtaCorrection::applyEta(Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }

    double newX = cluster->local().x();
    double newY = cluster->local().y();
    auto inPixelPos = m_detector->inPixel(cluster->column(), cluster->row());

    if(cluster->columnWidth() == 2) {
        // Apply the eta correction
        if(m_correctX) {
            newX = floor(cluster->column() + m_detector->getPitch().X() / 2) + m_etaCorrectorX->Eval(inPixelPos.X());
        }
    }

    if(cluster->rowWidth() == 2) {
        // Apply the eta correction
        if(m_correctY) {
            newY = floor(cluster->row() + m_detector->getPitch().Y() / 2) + m_etaCorrectorY->Eval(inPixelPos.Y());
        }
    }

    PositionVector3D<Cartesian3D<double>> positionLocal(newX, newY, 0);
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}

StatusCode EtaCorrection::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the clusters
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());
    for(auto& cluster : clusters) {
        applyEta(cluster.get());
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
