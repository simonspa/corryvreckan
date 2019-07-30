#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    timingCut = m_config.get<double>("timing_cut", Units::get<double>(200, "ns"));
    spatialCut = m_config.get<XYVector>("spatial_cut", 2 * m_detector->pitch());
}

void DUTAssociation::initialise() {
  // Cut flow histogram
  std::string title = m_detector->name() + ": number of clusters discarded by cuts;cut type;events";
  hCutHisto = new TH1F("hCutHisto", title.c_str(), 2, 1, 3);
  hCutHisto->GetXaxis()->SetBinLabel(1,"Spatial");
  hCutHisto->GetXaxis()->SetBinLabel(2,"Timing");
}

StatusCode DUTAssociation::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the tracks from the clipboard
    Tracks* tracks = reinterpret_cast<Tracks*>(clipboard->get("tracks"));
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Get the DUT clusters from the clipboard
    Clusters* clusters = reinterpret_cast<Clusters*>(clipboard->get(m_detector->name(), "clusters"));
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
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                           << Units::display(abs(ydistance), {"um", "mm"}) << ")";
                hCutHisto->Fill(1);
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timingCut) {
                LOG(DEBUG) << "Discarding DUT cluster with time difference "
                           << Units::display(std::abs(cluster->timestamp() - track->timestamp()), {"ms", "s"});
                hCutHisto->Fill(2);
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                       << Units::display(abs(ydistance), {"um", "mm"}) << ")";
            track->addAssociatedCluster(cluster);
            assoc_cluster_counter++;
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void DUTAssociation::finalise() {
    LOG(INFO) << "In total, " << assoc_cluster_counter << " clusters are associated to tracks.";
    return;
}
