#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    timingCut = m_config.get<double>("timing_cut", Units::get<double>(200, "ns"));
    spatialCut = m_config.get<XYVector>("spatial_cut", 2 * m_detector->pitch());
}

void DUTAssociation::initialise() {
  LOG(DEBUG) << "Booking histograms for detector " << m_detector->name();


  // Nr of associated clusters per track
  std::string title = m_detector->name() + ": number of associated clusters;associated clusters;events";
  hno_assoc_cls = new TH1F("no_assoc_cls", title.c_str(), 10, 0, 10);
  title = m_detector->name() + ": number of clusters discarded by cut;cut;events";
  hcut_flow = new TH1F("cut_flow", title.c_str(), 3, 1, 4);
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
        assoc_cls_per_track = 0;
        double min_distance = 99999;

        // Loop over all DUT clusters
        for(auto& cluster : (*clusters)) {
            // Check distance between track and cluster
            ROOT::Math::XYZPoint intercept = track->intercept(cluster->global().z());
            double xdistance = intercept.X() - cluster->global().x();
            double ydistance = intercept.Y() - cluster->global().y();
            double distance = sqrt(xdistance * xdistance + ydistance * ydistance);
            if(abs(xdistance) > spatialCut.x() || abs(ydistance) > spatialCut.y()) {
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                           << Units::display(abs(ydistance), {"um", "mm"}) << ")";
                hcut_flow->Fill(1);
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timingCut) {
                LOG(DEBUG) << "Discarding DUT cluster with time difference "
                           << Units::display(std::abs(cluster->timestamp() - track->timestamp()), {"ms", "s"});
                hcut_flow->Fill(2);
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                       << Units::display(abs(ydistance), {"um", "mm"}) << ")";
            track->addAssociatedCluster(cluster);
            assoc_cls_per_track++;
            assoc_cluster_counter++;

            // check if cluster is closest to track
            LOG(DEBUG) << "Distance: " << distance;
            if(distance < min_distance){
              min_distance = distance;
              track->setClosestCluster(cluster);
            }
        }
        hno_assoc_cls->Fill(assoc_cls_per_track);
        if(assoc_cls_per_track > 0){
          track_w_assoc_cls++;
        }

        // Get the closest associated cluster: //DELETE
        Cluster* closestCluster = track->getClosestCluster();
        if(!track->hasClosestCluster()){
          LOG(DEBUG) << "Closest cluster is nullpntr";
        }else{
          LOG(DEBUG) << "X position of closest cluster: " << closestCluster->global().x();
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void DUTAssociation::finalise() {
    LOG(INFO) << "In total, " << assoc_cluster_counter << " clusters are associated to tracks.";
    LOG(INFO) << "Number of tracks with at least one associated cluster: " << track_w_assoc_cls;
    return;
}
