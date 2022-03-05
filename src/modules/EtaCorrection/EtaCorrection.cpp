/**
 * @file
 * @brief Implementation of module EtaCorrection
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EtaCorrection.h"

using namespace corryvreckan;
using namespace std;

EtaCorrection::EtaCorrection(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    config_.setDefault<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");

    m_etaFormulaX = config_.get<std::string>("eta_formula_x");
    m_etaFormulaY = config_.get<std::string>("eta_formula_y");
}

void EtaCorrection::initialize() {

    // Initialise histograms
    // Get info from configuration:
    std::vector<double> m_etaConstantsX = config_.getArray<double>("eta_constants_x_" + m_detector->getName(), {});
    std::vector<double> m_etaConstantsY = config_.getArray<double>("eta_constants_y_" + m_detector->getName(), {});
    if(!m_etaConstantsX.empty() || !m_etaConstantsY.empty()) {
        LOG(INFO) << "Found Eta correction factors for detector \"" << m_detector->getName()
                  << "\": " << (m_etaConstantsX.empty() ? "" : "X ") << (m_etaConstantsY.empty() ? "" : "Y ");
    }

    if(!m_etaConstantsX.empty()) {
        m_correctX = true;
        m_etaCorrectorX =
            new TF1("etaCorrectorX", m_etaFormulaX.c_str(), -1 * m_detector->getPitch().X(), m_detector->getPitch().X());
        for(size_t x = 0; x < m_etaConstantsX.size(); x++) {
            m_etaCorrectorX->SetParameter(static_cast<int>(x), m_etaConstantsX[x]);
        }
    } else {
        m_correctX = false;
    }

    if(!m_etaConstantsY.empty()) {
        m_correctY = true;
        m_etaCorrectorY = new TF1(
            "etaCorrectorY", m_etaFormulaY.c_str(), -1 * m_detector->getPitch().Y(), -1 * m_detector->getPitch().Y());
        for(size_t y = 0; y < m_etaConstantsY.size(); y++)
            m_etaCorrectorY->SetParameter(static_cast<int>(y), m_etaConstantsY[y]);
    } else {
        m_correctY = false;
    }
    nPixels = m_detector->nPixels();
}

void EtaCorrection::applyEta(Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }
    double newX = cluster->local().x();
    double newY = cluster->local().y();

    if(cluster->columnWidth() == 2) {
        if(m_correctX) {
            auto reference_col = 0;
            for(auto& pixel : cluster->pixels()) {
                if(pixel->column() > reference_col) {
                    reference_col = pixel->column();
                }
            }
            auto reference_X = m_detector->getPitch().X() * (reference_col - 0.5 * m_detector->nPixels().X());
            auto xmod_cluster = cluster->local().X() - reference_X;
            newX = m_etaCorrectorX->Eval(xmod_cluster) + reference_X;
        }
    }

    if(cluster->rowWidth() == 2) {
        if(m_correctY) {
            auto reference_row = 0;
            for(auto& pixel : cluster->pixels()) {
                if(pixel->row() > reference_row) {
                    reference_row = pixel->row();
                }
            }
            auto reference_Y = m_detector->getPitch().Y() * (reference_row - 0.5 * m_detector->nPixels().Y());
            auto ymod_cluster = cluster->local().Y() - reference_Y;
            newY = m_etaCorrectorY->Eval(ymod_cluster) + reference_Y;
        }
    }

    PositionVector3D<Cartesian3D<double>> positionLocal(newX, newY, 0);
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}

StatusCode EtaCorrection::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the clusters
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());
    for(auto& cluster : clusters) {
        applyEta(cluster.get());
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
