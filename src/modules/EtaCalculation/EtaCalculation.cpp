#include "EtaCalculation.h"

using namespace corryvreckan;
using namespace std;

EtaCalculation::EtaCalculation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {
    m_chi2ndofCut = m_config.get<double>("chi2ndof_cut", 100.);
    m_etaFormulaX = m_config.get<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = m_config.get<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
}

void EtaCalculation::initialise() {

    // Initialise histograms
    double pitchX = m_detector->pitch().X();
    double pitchY = m_detector->pitch().Y();
    std::string title = m_detector->name() + " #eta distribution X";
    m_etaDistributionX =
        new TH2F("etaDistributionX", title.c_str(), 100., -pitchX / 2., pitchX / 2., 100., -pitchY / 2., pitchY / 2.);
    title = m_detector->name() + " #eta distribution Y";
    m_etaDistributionY =
        new TH2F("etaDistributionY", title.c_str(), 100., -pitchX / 2., pitchX / 2., 100., -pitchY / 2., pitchY / 2.);
    title = m_detector->name() + " #eta profile X";
    m_etaDistributionXprofile =
        new TProfile("etaDistributionXprofile", title.c_str(), 100., -pitchX / 2., pitchX / 2., -pitchY / 2., pitchY);
    title = m_detector->name() + " #eta profile Y";
    m_etaDistributionYprofile =
        new TProfile("etaDistributionYprofile", title.c_str(), 100., -pitchX / 2., pitchX / 2., -pitchY / 2., pitchY / 2.);

    // Prepare fit functions - we need them for every detector as they might have different pitches
    m_etaFitX = new TF1("etaFormulaX", m_etaFormulaX.c_str(), -pitchX / 2., pitchX / 2.);
    m_etaFitY = new TF1("etaFormulaY", m_etaFormulaY.c_str(), -pitchY / 2., pitchY / 2.);
}

ROOT::Math::XYVector EtaCalculation::pixelIntercept(Track* tr) {

    double pitchX = m_detector->pitch().X();
    double pitchY = m_detector->pitch().Y();
    // Get the in-pixel track intercept
    auto trackIntercept = m_detector->getIntercept(tr);
    auto trackInterceptLocal = m_detector->globalToLocal(trackIntercept);
    auto pixelIntercept = m_detector->inPixel(trackInterceptLocal);
    double pixelInterceptX = pixelIntercept.X();
    (pixelInterceptX > 0. ? pixelInterceptX -= pitchX / 2. : pixelInterceptX += pitchX / 2.); // not sure about this line
    double pixelInterceptY = pixelIntercept.Y();
    (pixelInterceptY > 0. ? pixelInterceptY -= pitchY / 2. : pixelInterceptY += pitchY / 2.); // not sure about this line
    return ROOT::Math::XYVector(pixelInterceptX, pixelInterceptY);
}

void EtaCalculation::calculateEta(Track* track, Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }

    auto detector = get_detector(cluster->detectorID());
    // Get the in-pixel track intercept
    auto pxIntercept = pixelIntercept(track);
    PositionVector3D<Cartesian3D<double>> localPosition(cluster->column(), cluster->row(), 0.);
    auto inPixel = detector->inPixel(localPosition);

    if(cluster->columnWidth() == 2) {
        m_etaDistributionX->Fill(inPixel.X(), pxIntercept.X());
        m_etaDistributionXprofile->Fill(inPixel.X(), pxIntercept.X());
    }

    if(cluster->rowWidth() == 2) {
        m_etaDistributionY->Fill(inPixel.Y(), pxIntercept.Y());
        m_etaDistributionYprofile->Fill(inPixel.Y(), pxIntercept.Y());
    }
}

StatusCode EtaCalculation::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all tracks and look at the associated clusters to plot the eta distribution
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > m_chi2ndofCut) {
            continue;
        }

        // Look at the associated clusters and plot the eta function
        for(auto& dutCluster : track->associatedClusters()) {
            if(dutCluster->detectorID() != m_detector->name()) {
                continue;
            }
            calculateEta(track, dutCluster);
        }
        // Do the same for all clusters of the track:
        for(auto& cluster : track->clusters()) {
            if(cluster->detectorID() != m_detector->name()) {
                continue;
            }
            calculateEta(track, cluster);
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

std::string EtaCalculation::fit(TF1* function, std::string fname, TProfile* profile) {
    std::stringstream parameters;

    // Get the eta distribution profiles and fit them to extract the correction parameters
    profile->Fit(function, "q");
    TF1* fit = profile->GetFunction(fname.c_str());

    for(int i = 0; i < fit->GetNumberFreeParameters(); i++) {
        parameters << " " << fit->GetParameter(i);
    }
    return parameters.str();
}

void EtaCalculation::finalise() {

    std::stringstream config;
    config << std::endl
           << "eta_constants_x_" << m_detector->name() << " =" << fit(m_etaFitX, "etaFormulaX", m_etaDistributionXprofile);
    config << std::endl
           << "eta_constants_y_" << m_detector->name() << " =" << fit(m_etaFitY, "etaFormulaY", m_etaDistributionYprofile);

    LOG(INFO) << "\"EtaCorrection\":" << config.str();
}
