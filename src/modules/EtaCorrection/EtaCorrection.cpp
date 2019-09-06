#include "EtaCorrection.h"

using namespace corryvreckan;
using namespace std;

EtaCorrection::EtaCorrection(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {
    m_etaFormulaX = m_config.get<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = m_config.get<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
}

void EtaCorrection::initialise() {

    // Initialise histograms
    double pitchX = m_detector->pitch().X();
    double pitchY = m_detector->pitch().Y();

    // Get info from configuration:
    std::vector<double> m_etaConstantsX = m_config.getArray<double>("eta_constants_x_" + m_detector->name(), {});
    std::vector<double> m_etaConstantsY = m_config.getArray<double>("eta_constants_y_" + m_detector->name(), {});
    if(!m_etaConstantsX.empty() || !m_etaConstantsY.empty()) {
        LOG(INFO) << "Found Eta correction factors for detector \"" << m_detector->name()
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
            newX = floor(cluster->column() + m_detector->pitch().X() / 2) + m_etaCorrectorX->Eval(inPixelPos.X());
        }
    }

    if(cluster->rowWidth() == 2) {
        // Apply the eta correction
        if(m_correctY) {
            newY = floor(cluster->row() + m_detector->pitch().Y() / 2) + m_etaCorrectorY->Eval(inPixelPos.Y());
        }
    }

    PositionVector3D<Cartesian3D<double>> positionLocal(newX, newY, 0);
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}

StatusCode EtaCorrection::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the clusters
    auto clusters = clipboard->get<Cluster>(m_detector->name());
    if(clusters == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any clusters on the clipboard";
        return StatusCode::Success;
    }

    for(auto& cluster : (*clusters)) {
        applyEta(cluster);
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
