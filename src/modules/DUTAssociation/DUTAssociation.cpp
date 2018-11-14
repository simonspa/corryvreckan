#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    timingCut = m_config.get<double>("timingCut", static_cast<double>(Units::convert(200, "ns")));
    spatialCut = m_config.get<XYVector>("spatialCut", 2 * m_detector->pitch());
}

StatusCode DUTAssociation::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks from the clipboard
    auto tracks = clipboard->get<Tracks>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Get the DUT clusters from the clipboard
    auto clusters = clipboard->get<Clusters>(m_detector->name());
    if(clusters == nullptr) {
        LOG(DEBUG) << "No DUT clusters on the clipboard";
        return StatusCode::Success;
    }

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        // Loop over all DUT clusters
        for(auto& cluster : (*clusters)) {
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->global().z());
            double xdistance = intercept.X() - cluster->global().x();
            double ydistance = intercept.Y() - cluster->global().y();
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
    return StatusCode::Success;
}
