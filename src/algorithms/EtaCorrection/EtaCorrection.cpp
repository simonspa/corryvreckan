#include "EtaCorrection.h"

using namespace corryvreckan;
using namespace std;

EtaCorrection::EtaCorrection(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_DUT = m_config.get<std::string>("DUT");
    m_chi2ndofCut = m_config.get<double>("chi2ndofCut", 100.);
    m_etaFormulaX = m_config.get<std::string>("EtaFormulaX", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaConstantsX = m_config.getArray<double>("EtaConstantsX", {});
    m_etaFormulaY = m_config.get<std::string>("EtaFormulaY", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaConstantsY = m_config.getArray<double>("EtaConstantsY", {});
}

void EtaCorrection::initialise() {

    // Initialise single histograms
    m_detector = get_detector(m_DUT);
    double pitchX = m_detector->pitchX();
    double pitchY = m_detector->pitchY();
    string name = "etaDistributionX";
    m_etaDistributionX = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);
    name = "etaDistributionY";
    m_etaDistributionY = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);
    name = "etaDistributionXprofile";
    m_etaDistributionXprofile = new TProfile(name.c_str(), name.c_str(), 100., 0., pitchX, 0., pitchY);
    name = "etaDistributionYprofile";
    m_etaDistributionYprofile = new TProfile(name.c_str(), name.c_str(), 100., 0., pitchX, 0., pitchY);
    name = "etaDistributionXcorrected";
    m_etaDistributionXcorrected = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);
    name = "etaDistributionYcorrected";
    m_etaDistributionYcorrected = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);

    // Initialise member variables
    m_etaCorrectorX = new TF1("etaCorrectorX", m_etaFormulaX.c_str(), 0, m_detector->pitchX());
    for(int x = 0; x < m_etaConstantsX.size(); x++)
        m_etaCorrectorX->SetParameter(x, m_etaConstantsX[x]);
    m_etaCorrectorY = new TF1("etaCorrectorY", m_etaFormulaY.c_str(), 0, m_detector->pitchY());
    for(int y = 0; y < m_etaConstantsY.size(); y++)
        m_etaCorrectorY->SetParameter(y, m_etaConstantsY[y]);
    m_eventNumber = 0;
}

StatusCode EtaCorrection::run(Clipboard* clipboard) {

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Loop over all tracks and look at the associated clusters to plot the eta distribution
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > m_chi2ndofCut) {
            continue;
        }

        // Get the in-pixel track intercept
        PositionVector3D<Cartesian3D<double>> trackIntercept = m_detector->getIntercept(track);
        PositionVector3D<Cartesian3D<double>> trackInterceptLocal = *(m_detector->globalToLocal()) * trackIntercept;
        double pixelInterceptX = m_detector->inPixelX(trackInterceptLocal);
        double pixelInterceptY = m_detector->inPixelY(trackInterceptLocal);
        (pixelInterceptX > m_detector->pitchX() / 2. ? pixelInterceptX -= m_detector->pitchX() / 2.
                                                     : pixelInterceptX += m_detector->pitchX() / 2.);
        (pixelInterceptY > m_detector->pitchY() / 2. ? pixelInterceptY -= m_detector->pitchY() / 2.
                                                     : pixelInterceptY += m_detector->pitchY() / 2.);

        // Look at the associated clusters and plot the eta function
        for(auto& dutCluster : track->associatedClusters()) {

            // Only look at clusters from the DUT
            if(dutCluster->detectorID() != m_DUT)
                continue;

            // Ignore single pixel clusters
            if(dutCluster->size() == 1)
                continue;

            // Get the fraction along the pixel
            double inPixelX = m_detector->pitchX() * (dutCluster->column() - floor(dutCluster->column()));
            double inPixelY = m_detector->pitchY() * (dutCluster->row() - floor(dutCluster->row()));
            if(dutCluster->columnWidth() == 2) {
                m_etaDistributionX->Fill(inPixelX, pixelInterceptX);
                m_etaDistributionXprofile->Fill(inPixelX, pixelInterceptX);
                // Apply the eta correction
                double newX = floor(dutCluster->localX() / m_detector->pitchX()) * m_detector->pitchX() +
                              m_etaCorrectorX->Eval(inPixelX);
                PositionVector3D<Cartesian3D<double>> positionLocal(newX, dutCluster->localY(), 0);
                PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);
                dutCluster->setClusterCentre(positionGlobal);
                dutCluster->setClusterCentreLocal(positionLocal);
            }
            if(dutCluster->rowWidth() == 2) {
                m_etaDistributionY->Fill(inPixelY, pixelInterceptY);
                m_etaDistributionYprofile->Fill(inPixelY, pixelInterceptY);
            }
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void EtaCorrection::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
