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
    config_.setDefault<std::string>("eta_formula_x", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");
    config_.setDefault<std::string>("eta_formula_y", "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5");

    m_chi2ndofCut = config_.get<double>("chi2ndof_cut");
    m_etaFormulaX = config_.get<std::string>("eta_formula_x");
    m_etaFormulaY = config_.get<std::string>("eta_formula_y");
}

void EtaCalculation::initialize() {

    // Initialise histograms
    auto pitch_x = static_cast<double>(Units::convert(m_detector->getPitch().X(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(m_detector->getPitch().Y(), "um"));
    std::string mod_axes_x = "in-pixel x_{cluster} [#mum];in-pixel x_{track} [#mum];";
    std::string mod_axes_y = "in-pixel y_{cluster} [#mum];in-pixel y_{track} [#mum];";

    std::string title = "2D #eta distribution X;" + mod_axes_x + "No. entries";
    m_etaDistributionX = new TH2F("etaDistributionX",
                                  title.c_str(),
                                  static_cast<int>(pitch_x),
                                  -pitch_x / 2.,
                                  pitch_x / 2.,
                                  static_cast<int>(pitch_x),
                                  -pitch_x / 2.,
                                  pitch_x / 2.);
    title = "2D #eta distribution Y;" + mod_axes_y + "No. entries";
    m_etaDistributionY = new TH2F("etaDistributionY",
                                  title.c_str(),
                                  static_cast<int>(pitch_y),
                                  -pitch_y / 2.,
                                  pitch_y / 2.,
                                  static_cast<int>(pitch_y),
                                  -pitch_y / 2.,
                                  pitch_y / 2.);

    title = "#eta distribution X;" + mod_axes_x;
    m_etaDistributionXprofile =
        new TProfile("etaDistributionXprofile", title.c_str(), static_cast<int>(pitch_x), -pitch_x / 2., pitch_x / 2.);
    title = "#eta distribution Y;" + mod_axes_x;
    m_etaDistributionYprofile =
        new TProfile("etaDistributionYprofile", title.c_str(), static_cast<int>(pitch_y), -pitch_y / 2., pitch_y / 2.);

    title = "In-pixel track intercept; in-pixel x_{track} [#mum]; in-pixel y_{track} [#mum]";
    m_inpixelTrackIntercept = new TH2F("inpixelTrackIntercept",
                                       title.c_str(),
                                       static_cast<int>(pitch_x),
                                       -pitch_x / 2.,
                                       pitch_x / 2.,
                                       static_cast<int>(pitch_y),
                                       -pitch_y / 2.,
                                       pitch_y / 2.);

    // Prepare fit functions - we need them for every detector as they might have different pitches
    m_etaFitX = new TF1("etaFormulaX", m_etaFormulaX.c_str(), -pitch_x / 2., pitch_x / 2.);
    m_etaFitY = new TF1("etaFormulaY", m_etaFormulaY.c_str(), -pitch_y / 2., pitch_y / 2.);
}

void EtaCalculation::calculateEta(Track* track, Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->size() == 1) {
        return;
    }
    auto localIntercept = m_detector->getLocalIntercept(track);
    auto inpixel = m_detector->inPixel(localIntercept);
    auto xmod_track = static_cast<double>(Units::convert(inpixel.X(), "um"));
    auto ymod_track = static_cast<double>(Units::convert(inpixel.Y(), "um"));

    auto asclust_inPixel = m_detector->inPixel(cluster->column(), cluster->row());
    auto xmod_cluster = static_cast<double>(Units::convert(asclust_inPixel.X(), "um"));
    auto ymod_cluster = static_cast<double>(Units::convert(asclust_inPixel.Y(), "um"));

    m_inpixelTrackIntercept->Fill(xmod_track, ymod_track);

    if(cluster->columnWidth() == 2) {
        m_etaDistributionX->Fill(xmod_cluster, xmod_track);
        m_etaDistributionXprofile->Fill(xmod_cluster, xmod_track);
    }
    if(cluster->rowWidth() == 2) {
        m_etaDistributionY->Fill(ymod_cluster, ymod_track);
        m_etaDistributionYprofile->Fill(ymod_cluster, ymod_track);
    }
}

StatusCode EtaCalculation::run(const std::shared_ptr<Clipboard>& clipboard) {

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

void EtaCalculation::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    std::stringstream config;
    config << std::endl
           << "eta_constants_x_" << m_detector->getName() << " ="
           << fit(m_etaFitX, "etaFormulaX", m_etaDistributionXprofile);
    config << std::endl
           << "eta_constants_y_" << m_detector->getName() << " ="
           << fit(m_etaFitY, "etaFormulaY", m_etaDistributionYprofile);

    LOG(INFO) << "\"EtaCorrection\":" << config.str();
}
