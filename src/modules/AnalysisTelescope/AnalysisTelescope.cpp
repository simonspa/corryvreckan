#include "AnalysisTelescope.h"
#include "objects/Cluster.hpp"
#include "objects/MCParticle.hpp"

#include <TDirectory.h>

using namespace corryvreckan;
using namespace std;

AnalysisTelescope::AnalysisTelescope(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    chi2ndofCut = m_config.get<double>("chi2ndof_cut", 3.);
}

void AnalysisTelescope::initialise() {

    // Initialise biased telescope residuals per telescope device and telescope resolution plots (using MCparticles) at the
    // position of the DUTs
    for(auto& detector : get_detectors()) {
        TDirectory* directory = getROOTDirectory();
        TDirectory* local_directory = directory->mkdir(detector->name().c_str());
        if(local_directory == nullptr) {
            throw RuntimeError("Cannot create or access local ROOT directory for module " + this->getUniqueName());
        }
        local_directory->cd();

        if(detector->isDUT()) {
            std::string title = detector->name() + " Telescope resolution X;x_{track}-x_{MC} [mm];events";
            telescopeResolutionX[detector->name()] = new TH1F("telescopeResolutionX", title.c_str(), 600, -0.2, 0.2);
            title = detector->name() + " Telescope resolution Y;y_{track}-y_{MC} [mm];events";
            telescopeResolutionY[detector->name()] = new TH1F("telescopeResolutionY", title.c_str(), 600, -0.2, 0.2);
        } else {
            std::string title = detector->name() + " Biased residual X (local);x_{track}-x_{cluster} [mm];events";
            telescopeResidualsLocalX[detector->name()] = new TH1F("residualX_local", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased residual Y (local);y_{track}-y_{cluster} [mm];events";
            telescopeResidualsLocalY[detector->name()] = new TH1F("residualY_local", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased residual X (global);x_{track}-x_{cluster} [mm];events";
            telescopeResidualsX[detector->name()] = new TH1F("residualX_global", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased residual Y (global);y_{track}-y_{cluster} [mm];events";
            telescopeResidualsY[detector->name()] = new TH1F("residualY_global", title.c_str(), 400, -0.2, 0.2);

            title = detector->name() + " Biased MC residual X (local);x_{track}-x_{MC} [mm];events";
            telescopeMCresidualsLocalX[detector->name()] = new TH1F("residualX_MC_local", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased MC residual Y (local);y_{track}-y_{MC} [mm];events";
            telescopeMCresidualsLocalY[detector->name()] = new TH1F("residualY_MC_local", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased MC residual X (global);x_{track}-x_{MC} [mm];events";
            telescopeMCresidualsX[detector->name()] = new TH1F("residualX_MC_global", title.c_str(), 400, -0.2, 0.2);
            title = detector->name() + " Biased MC residual Y (global);y_{track}-y_{MC} [mm];events";
            telescopeMCresidualsY[detector->name()] = new TH1F("residualY_MC_global", title.c_str(), 400, -0.2, 0.2);
        }

        directory->cd();
    }
}

ROOT::Math::XYZPoint AnalysisTelescope::closestApproach(ROOT::Math::XYZPoint position,
                                                        std::shared_ptr<MCParticleVector> particles) {
    // Find the closest MC particle
    double smallestDistance(DBL_MAX);
    ROOT::Math::XYZPoint particlePosition;
    for(auto& particle : (*particles)) {
        ROOT::Math::XYZPoint entry = particle->getLocalStart();
        ROOT::Math::XYZPoint exit = particle->getLocalEnd();
        ROOT::Math::XYZPoint centre((entry.X() + exit.X()) / 2., (entry.Y() + exit.Y()) / 2., (entry.Z() + exit.Z()) / 2.);
        double distance = sqrt((centre.X() - position.X()) * (centre.X() - position.X()) +
                               (centre.Y() - position.Y()) * (centre.Y() - position.Y()));
        if(distance < smallestDistance) {
            particlePosition.SetXYZ(centre.X(), centre.Y(), centre.Z());
        }
    }
    return particlePosition;
}

StatusCode AnalysisTelescope::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks from the clipboard
    auto tracks = clipboard->get<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            continue;
        }

        // Loop over clusters of the track:
        for(auto& cluster : track->clusters()) {
            auto detector = get_detector(cluster->detectorID());
            if(detector == nullptr || detector->isDUT()) {
                continue;
            }

            auto name = detector->name();
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->global().z());
            auto interceptLocal = detector->globalToLocal(intercept);
            telescopeResidualsLocalX[name]->Fill(cluster->local().x() - interceptLocal.X());
            telescopeResidualsLocalY[name]->Fill(cluster->local().y() - interceptLocal.Y());
            telescopeResidualsX[name]->Fill(cluster->global().x() - intercept.X());
            telescopeResidualsY[name]->Fill(cluster->global().y() - intercept.Y());

            // Get the MC particles from the clipboard
            auto mcParticles = clipboard->get<MCParticle>(name);
            if(mcParticles == nullptr) {
                continue;
            }

            ROOT::Math::XYZPoint particlePosition = closestApproach(cluster->local(), mcParticles);
            telescopeMCresidualsLocalX[name]->Fill(cluster->local().x() + detector->size().X() / 2 - particlePosition.X());
            telescopeMCresidualsLocalY[name]->Fill(cluster->local().y() + detector->size().Y() / 2 - particlePosition.Y());
            telescopeMCresidualsX[name]->Fill(interceptLocal.X() + detector->size().X() / 2 - particlePosition.X());
            telescopeMCresidualsY[name]->Fill(interceptLocal.Y() + detector->size().Y() / 2 - particlePosition.Y());
        }

        // Calculate telescope resolution at DUT
        for(auto& detector : get_detectors()) {
            if(!detector->isDUT()) {
                continue;
            }

            // Get the MC particles from the clipboard
            auto mcParticles = clipboard->get<MCParticle>(detector->name());
            if(mcParticles == nullptr) {
                continue;
            }

            auto intercept = detector->getIntercept(track);
            auto interceptLocal = detector->globalToLocal(intercept);
            auto particlePosition = closestApproach(interceptLocal, mcParticles);

            telescopeResolutionX[detector->name()]->Fill(interceptLocal.X() + detector->size().X() / 2 -
                                                         particlePosition.X());
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}
