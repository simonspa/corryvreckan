#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_DUT = m_config.get<std::string>("DUT");
    auto det = get_detector(m_DUT);

    timingCut = m_config.get<double>("timingCut", Units::convert(200, "ns"));
    spatialCut = m_config.get<XYVector>("spatialCut", 2 * det->pitch());
}

StatusCode DUTAssociation::run(Clipboard* clipboard) {

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT clusters from the clipboard
    Clusters* clusters = (Clusters*)clipboard->get(m_DUT, "clusters");
    if(clusters == NULL) {
        LOG(DEBUG) << "No DUT clusters on the clipboard";
        return Success;
    }

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        // Loop over all DUT clusters
        bool associated = false;
        for(auto& cluster : (*clusters)) {
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->globalZ());
            double xdistance = intercept.X() - cluster->globalX();
            double ydistance = intercept.Y() - cluster->globalY();
            if(abs(xdistance) > spatialCut.x() || abs(ydistance) > spatialCut.y()) {
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << abs(xdistance) << "," << abs(ydistance) << ")";
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timingCut) {
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << abs(xdistance) << "," << abs(ydistance) << ")";
            track->addAssociatedCluster(cluster);
        }
    }

    // Return value telling analysis to keep running
    return Success;
}
