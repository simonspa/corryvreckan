#include "DUTAssociation.h"

using namespace corryvreckan;
using namespace std;

DUTAssociation::DUTAssociation(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    timingCut = m_config.get<double>("timing_cut", Units::get<double>(200, "ns"));
    spatialCut = m_config.get<XYVector>("spatial_cut", 2 * m_detector->pitch());
    useClusterCentre = m_config.get<bool>("use_cluster_centre", true);
}

void DUTAssociation::initialise() {
    hX1X2 = new TH1D("hX1X2", "hX1X2; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hY1Y2 = new TH1D("hY1Y2", "hY1Y2; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hX1X2_1px =
        new TH1D("hX1X2_1px", "hX1X2_1px; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hY1Y2_1px =
        new TH1D("hY1Y2_1px", "hY1Y2_1px; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hX1X2_npx =
        new TH1D("hX1X2_npx", "hX1X2_npx; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hY1Y2_npx =
        new TH1D("hY1Y2_npx", "hY1Y2_npx; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 1000, -1000, 9000);
    hClusterSize_largeDistance =
        new TH1D("hClusterSize_largeDistance",
                 "hClusterSize_largeDistance; clusterSize if (distance cluster - nearest pixel > 100um); # events",
                 20,
                 0,
                 20);
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
            auto interceptLocal = m_detector->globalToLocal(intercept);

            double xdistance = 0;
            double ydistance = 0;
            double xdistance2 = 0;
            double ydistance2 = 0;
            // if(useClusterCentre) {
            // use cluster centre for distance to track:
            // xdistance = abs(intercept.X() - cluster->global().x());
            // ydistance = abs(intercept.Y() - cluster->global().y());
            xdistance = abs(interceptLocal.X() - cluster->local().x());
            ydistance = abs(interceptLocal.Y() - cluster->local().y());
            // } else {
            // use nearest pixel for distance to track (for efficiency analysis):
            for(auto& pixel : (*cluster->pixels())) {

                // convert pixel address to global coordinates:
                auto pixelPositionLocal =
                    m_detector->getLocalPosition(static_cast<double>(pixel->column()), static_cast<double>(pixel->row()));
                // auto pixelPositionGlobal = m_detector->localToGlobal(pixelPositionLocal);

                // LOG(DEBUG) << "px pos global: " << Units::display(pixelPositionGlobal.x(), "um") << ", "
                // << Units::display(pixelPositionGlobal.y(), "um");
                auto xdistance_new = abs(interceptLocal.X() - pixelPositionLocal.x());
                auto ydistance_new = abs(interceptLocal.Y() - pixelPositionLocal.y());
                xdistance2 = (xdistance2 < xdistance_new) ? xdistance2 : xdistance_new;
                ydistance2 = (ydistance2 < ydistance_new) ? xdistance2 : ydistance_new;
            }
            // }
            LOG(DEBUG) << "xdistance(cluster) - xdistance(nearest pixel) = " << Units::display(xdistance - xdistance2, "um");
            hX1X2->Fill(static_cast<double>(Units::convert(xdistance - xdistance2, "um")));
            hY1Y2->Fill(static_cast<double>(Units::convert(ydistance - ydistance2, "um")));
            if(cluster->size() == 1) {
                hX1X2_1px->Fill(static_cast<double>(Units::convert(xdistance - xdistance2, "um")));
                hY1Y2_1px->Fill(static_cast<double>(Units::convert(ydistance - ydistance2, "um")));
            } else {
                hX1X2_npx->Fill(static_cast<double>(Units::convert(xdistance - xdistance2, "um")));
                hY1Y2_npx->Fill(static_cast<double>(Units::convert(ydistance - ydistance2, "um")));
            }

            if(Units::convert(xdistance - xdistance2, "um") > 500 || Units::convert(ydistance - ydistance2, "um") > 500) {
                hClusterSize_largeDistance->Fill(static_cast<double>(cluster->size()));
            }

            if(abs(xdistance) > spatialCut.x() || abs(ydistance) > spatialCut.y()) {
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                           << Units::display(abs(ydistance), {"um", "mm"}) << ")";
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timingCut) {
                LOG(DEBUG) << "Discarding DUT cluster with time difference "
                           << Units::display(std::abs(cluster->timestamp() - track->timestamp()), {"ms", "s"});
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << abs(xdistance) << "," << abs(ydistance) << ")";
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
