/**
 * @file
 * @brief Implementation of module EtaCalculation
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EtaCalculation.h"

using namespace corryvreckan;
using namespace std;

EtaCalculation::EtaCalculation(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("chi2ndof_cut", 100.);
    m_chi2ndofCut = config_.get<double>("chi2ndof_cut");

    config_.setDefault<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaX = config_.get<std::string>("eta_formula_x");

    config_.setDefault<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    m_etaFormulaY = config_.get<std::string>("eta_formula_y");
}

void EtaCalculation::initialise() {

    // Initialise histograms
    double pitchX = m_detector->getPitch().X();
    double pitchY = m_detector->getPitch().Y();
    std::string title = m_detector->getName() + " #eta distribution X";
    m_etaDistributionX =
        new TH2F("etaDistributionX", title.c_str(), 100., -pitchX / 2., pitchX / 2., 100., -pitchY / 2., pitchY / 2.);
    title = m_detector->getName() + " #eta distribution Y";
    m_etaDistributionY =
        new TH2F("etaDistributionY", title.c_str(), 100., -pitchX / 2., pitchX / 2., 100., -pitchY / 2., pitchY / 2.);
    title = m_detector->getName() + " #eta profile X";
    m_etaDistributionXprofile =
        new TProfile("etaDistributionXprofile", title.c_str(), 100., -pitchX / 2., pitchX / 2., -pitchY / 2., pitchY);
    title = m_detector->getName() + " #eta profile Y";
    m_etaDistributionYprofile =
        new TProfile("etaDistributionYprofile", title.c_str(), 100., -pitchX / 2., pitchX / 2., -pitchY / 2., pitchY / 2.);

    // Prepare fit functions - we need them for every detector as they might have different pitches
    m_etaFitX = new TF1("etaFormulaX", m_etaFormulaX.c_str(), -pitchX / 2., pitchX / 2.);
    m_etaFitY = new TF1("etaFormulaY", m_etaFormulaY.c_str(), -pitchY / 2., pitchY / 2.);
}

ROOT::Math::XYVector EtaCalculation::pixelIntercept(Track* tr) {

    double pitchX = m_detector->getPitch().X();
    double pitchY = m_detector->getPitch().Y();
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

    // Loop over all tracks and look at the associated clusters to plot the eta distribution
    auto tracks = clipboard->getData<Track>();
    for(auto& track : tracks) {

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > m_chi2ndofCut) {
            continue;
        }

        // Look at the associated clusters and plot the eta function
        for(auto& dutCluster : track->getAssociatedClusters(m_detector->getName())) {
            calculateEta(track.get(), dutCluster);
        }

        // Do the same for all clusters of the track:
        for(auto& cluster : track->getClusters()) {
            if(cluster->detectorID() != m_detector->getName()) {
                continue;
            }
            calculateEta(track.get(), cluster);
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
           << "eta_constants_x_" << m_detector->getName() << " ="
           << fit(m_etaFitX, "etaFormulaX", m_etaDistributionXprofile);
    config << std::endl
           << "eta_constants_y_" << m_detector->getName() << " ="
           << fit(m_etaFitY, "etaFormulaY", m_etaDistributionYprofile);

    LOG(INFO) << "\"EtaCorrection\":" << config.str();
}
