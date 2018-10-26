#include "EtaCalculation.h"

using namespace corryvreckan;
using namespace std;

EtaCalculation::EtaCalculation(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {
    m_chi2ndofCut = m_config.get<double>("chi2ndofCut", 100.);
    m_etaFormulaX = m_config.get<std::string>("EtaFormulaX", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = m_config.get<std::string>("EtaFormulaY", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
}

void EtaCalculation::initialise() {
    for(auto& detector : get_detectors()) {

        // Initialise histograms
        double pitchX = detector->pitch().X();
        double pitchY = detector->pitch().Y();
        string name = "etaDistributionX_" + detector->name();
        m_etaDistributionX[detector->name()] = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);
        name = "etaDistributionY_" + detector->name();
        m_etaDistributionY[detector->name()] = new TH2F(name.c_str(), name.c_str(), 100., 0., pitchX, 100., 0., pitchY);
        name = "etaDistributionXprofile_" + detector->name();
        m_etaDistributionXprofile[detector->name()] = new TProfile(name.c_str(), name.c_str(), 100., 0., pitchX, 0., pitchY);
        name = "etaDistributionYprofile_" + detector->name();
        m_etaDistributionYprofile[detector->name()] = new TProfile(name.c_str(), name.c_str(), 100., 0., pitchX, 0., pitchY);

        // Prepare fit functions - we need them for every detector as they might have different pitches
        m_etaFitX[detector->name()] = new TF1("etaFormulaX", m_etaFormulaX.c_str(), 0, pitchX);
        m_etaFitY[detector->name()] = new TF1("etaFormulaY", m_etaFormulaY.c_str(), 0, pitchY);
    }
}

ROOT::Math::XYVector EtaCalculation::pixelIntercept(Track* tr, std::shared_ptr<Detector> det) {

    double pitchX = det->pitch().X();
    double pitchY = det->pitch().Y();
    // Get the in-pixel track intercept
    PositionVector3D<Cartesian3D<double>> trackIntercept = det->getIntercept(tr);
    PositionVector3D<Cartesian3D<double>> trackInterceptLocal = det->globalToLocal(trackIntercept);
    double pixelInterceptX = det->inPixelX(trackInterceptLocal);
    (pixelInterceptX > pitchX / 2. ? pixelInterceptX -= pitchX / 2. : pixelInterceptX += pitchX / 2.);
    double pixelInterceptY = det->inPixelY(trackInterceptLocal);
    (pixelInterceptY > pitchY / 2. ? pixelInterceptY -= pitchY / 2. : pixelInterceptY += pitchY / 2.);
    return ROOT::Math::XYVector(pixelInterceptX, pixelInterceptY);
}

void EtaCalculation::calculateEta(Track* track, Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }

    auto detector = get_detector(cluster->detectorID());
    // Get the in-pixel track intercept
    auto pxIntercept = pixelIntercept(track, detector);

    if(cluster->columnWidth() == 2) {
        double inPixelX = detector->pitch().X() * (cluster->column() - floor(cluster->column()));
        m_etaDistributionX[detector->name()]->Fill(inPixelX, pxIntercept.X());
        m_etaDistributionXprofile[detector->name()]->Fill(inPixelX, pxIntercept.X());
    }

    if(cluster->rowWidth() == 2) {
        double inPixelY = detector->pitch().Y() * (cluster->row() - floor(cluster->row()));
        m_etaDistributionY[detector->name()]->Fill(inPixelY, pxIntercept.Y());
        m_etaDistributionYprofile[detector->name()]->Fill(inPixelY, pxIntercept.Y());
    }
}

StatusCode EtaCalculation::run(Clipboard* clipboard) {

    // Get the tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Loop over all tracks and look at the associated clusters to plot the eta distribution
    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > m_chi2ndofCut) {
            continue;
        }

        // Look at the associated clusters and plot the eta function
        for(auto& dutCluster : track->associatedClusters()) {
            calculateEta(track, dutCluster);
        }
        // Do the same for all clusters of the track:
        for(auto& cluster : track->clusters()) {
            calculateEta(track, cluster);
        }
    }

    // Return value telling analysis to keep running
    return Success;
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
    for(auto& detector : get_detectors()) {
        config << std::endl
               << "EtaConstantsX_" << detector->name() << " ="
               << fit(m_etaFitX[detector->name()], "etaFormulaX", m_etaDistributionXprofile[detector->name()]);
        config << std::endl
               << "EtaConstantsY_" << detector->name() << " ="
               << fit(m_etaFitY[detector->name()], "etaFormulaY", m_etaDistributionYprofile[detector->name()]);
    }

    LOG(INFO) << "Configuration for \"EtaCorrection\" algorithm:" << config.str();
}
