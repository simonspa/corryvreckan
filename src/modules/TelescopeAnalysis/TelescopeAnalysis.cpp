#include "TelescopeAnalysis.h"
#include "objects/Cluster.h"
#include "objects/MCParticle.h"

using namespace corryvreckan;
using namespace std;

TelescopeAnalysis::TelescopeAnalysis(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    chi2ndofCut = m_config.get<double>("chi2ndofCut", 3.);
}

void TelescopeAnalysis::initialise() {

    telescopeResolution = new TH1F("telescopeResolution", "telescopeResolution", 600, -0.2, 0.2);

    // Initialise histograms per device
    for(auto& detector : get_detectors()) {
        std::string name = "residualX_local_" + detector->name();
        telescopeResidualsLocalX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualY_local_" + detector->name();
        telescopeResidualsLocalY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualX_global_" + detector->name();
        telescopeResidualsX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualY_global_" + detector->name();
        telescopeResidualsY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);

        name = "residualX_MC_local_" + detector->name();
        telescopeMCresidualsLocalX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualY_MC_local_" + detector->name();
        telescopeMCresidualsLocalY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualX_MC_global_" + detector->name();
        telescopeMCresidualsX[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
        name = "residualY_MC_global_" + detector->name();
        telescopeMCresidualsY[detector->name()] = new TH1F(name.c_str(), name.c_str(), 400, -0.2, 0.2);
    }
}

ROOT::Math::XYZPoint TelescopeAnalysis::closestApproach(ROOT::Math::XYZPoint position, MCParticles* particles) {
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

StatusCode TelescopeAnalysis::run(Clipboard* clipboard) {

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    for(auto& track : (*tracks)) {

        // Cut on the chi2/ndof
        if(track->chi2ndof() > chi2ndofCut) {
            continue;
        }

        // Loop over clusters of the track:
        for(auto& cluster : track->clusters()) {
            auto detector = get_detector(cluster->detectorID());
            auto name = detector->name();

            // Ignore DUT
            if(name == m_config.get<std::string>("DUT")) {
                continue;
            }

            // Get the MC particles from the clipboard
            MCParticles* mcParticles = (MCParticles*)clipboard->get(name, "mcparticles");
            if(mcParticles == NULL) {
                continue;
            }

            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
            auto interceptLocal = detector->globalToLocal(intercept);
            ROOT::Math::XYZPoint particlePosition = closestApproach(cluster->local(), mcParticles);

            telescopeResidualsLocalX[name]->Fill(cluster->localX() - interceptLocal.X());
            telescopeResidualsLocalY[name]->Fill(cluster->localY() - interceptLocal.Y());
            telescopeResidualsX[name]->Fill(cluster->globalX() - intercept.X());
            telescopeResidualsY[name]->Fill(cluster->globalY() - intercept.Y());

            telescopeMCresidualsLocalX[name]->Fill(cluster->localX() + detector->size().X() / 2 - particlePosition.X());
            telescopeMCresidualsLocalY[name]->Fill(cluster->localY() + detector->size().Y() / 2 - particlePosition.Y());
            telescopeMCresidualsX[name]->Fill(interceptLocal.X() + detector->size().X() / 2 - particlePosition.X());
            telescopeMCresidualsY[name]->Fill(interceptLocal.Y() + detector->size().Y() / 2 - particlePosition.Y());
        }

        // Calculate telescope resolution at DUT
        auto detector = get_detector(m_config.get<std::string>("DUT"));

        // Get the MC particles from the clipboard
        MCParticles* mcParticles = (MCParticles*)clipboard->get(detector->name(), "mcparticles");
        if(mcParticles == NULL) {
            continue;
        }

        auto intercept = detector->getIntercept(track);
        auto interceptLocal = detector->globalToLocal(intercept);
        auto particlePosition = closestApproach(interceptLocal, mcParticles);

        telescopeResolution->Fill(interceptLocal.X() + detector->size().X() / 2 - particlePosition.X());
    }

    // Return value telling analysis to keep running
    return Success;
}
