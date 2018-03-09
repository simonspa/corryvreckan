#include "EtaCorrection.h"

using namespace corryvreckan;
using namespace std;

EtaCorrection::EtaCorrection(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_etaFormulaX = m_config.get<std::string>("EtaFormulaX", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = m_config.get<std::string>("EtaFormulaY", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
}

void EtaCorrection::initialise() {

    // Initialise histograms
    for(auto& detector : get_detectors()) {
        double pitchX = detector->pitchX();
        double pitchY = detector->pitchY();

        // Get info from configuration:
        std::vector<double> m_etaConstantsX = m_config.getArray<double>("EtaConstantsX_" + detector->name(), {});
        std::vector<double> m_etaConstantsY = m_config.getArray<double>("EtaConstantsY_" + detector->name(), {});
        if(!m_etaConstantsX.empty() || !m_etaConstantsY.empty()) {
            LOG(INFO) << "Found Eta correction factors for detector \"" << detector->name()
                      << "\": " << (m_etaConstantsX.empty() ? "" : "X ") << (m_etaConstantsY.empty() ? "" : "Y ");
        }

        if(!m_etaConstantsX.empty()) {
            m_correctX[detector->name()] = true;
            m_etaCorrectorX[detector->name()] = new TF1("etaCorrectorX", m_etaFormulaX.c_str(), 0, detector->pitchX());
            for(int x = 0; x < m_etaConstantsX.size(); x++) {
                m_etaCorrectorX[detector->name()]->SetParameter(x, m_etaConstantsX[x]);
            }
        } else {
            m_correctX[detector->name()] = false;
        }

        if(!m_etaConstantsY.empty()) {
            m_correctY[detector->name()] = true;
            m_etaCorrectorY[detector->name()] = new TF1("etaCorrectorY", m_etaFormulaY.c_str(), 0, detector->pitchY());
            for(int y = 0; y < m_etaConstantsY.size(); y++)
                m_etaCorrectorY[detector->name()]->SetParameter(y, m_etaConstantsY[y]);
        } else {
            m_correctY[detector->name()] = false;
        }
    }
}

void EtaCorrection::applyEta(Cluster* cluster, Detector* detector) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }

    double newX = cluster->localX();
    double newY = cluster->localY();

    if(cluster->columnWidth() == 2) {
        double inPixelX = detector->pitchX() * (cluster->column() - floor(cluster->column()));

        // Apply the eta correction
        if(m_correctX[detector->name()]) {
            newX = floor(cluster->localX() / detector->pitchX()) * detector->pitchX() +
                   m_etaCorrectorX[detector->name()]->Eval(inPixelX);
        }
    }

    if(cluster->rowWidth() == 2) {
        double inPixelY = detector->pitchY() * (cluster->row() - floor(cluster->row()));

        // Apply the eta correction
        if(m_correctY[detector->name()]) {
            newY = floor(cluster->localY() / detector->pitchY()) * detector->pitchY() +
                   m_etaCorrectorY[detector->name()]->Eval(inPixelY);
        }
    }

    PositionVector3D<Cartesian3D<double>> positionLocal(newX, newY, 0);
    PositionVector3D<Cartesian3D<double>> positionGlobal = detector->localToGlobal(positionLocal);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}

StatusCode EtaCorrection::run(Clipboard* clipboard) {

    for(auto& detector : get_detectors()) {
        // Get the clusters
        Clusters* clusters = (Clusters*)clipboard->get(detector->name(), "clusters");
        if(clusters == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any clusters on the clipboard";
            return Success;
        }

        for(auto& cluster : (*clusters)) {
            applyEta(cluster, detector);
        }
    }

    // Return value telling analysis to keep running
    return Success;
}
