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
    // Cut flow histogram
    std::string title = m_detector->name() + ": number of tracks discarded by different cuts;cut type;clusters";
    hCutHisto = new TH1F("hCutHisto", title.c_str(), 2, 1, 3);
    hCutHisto->GetXaxis()->SetBinLabel(1, "Spatial");
    hCutHisto->GetXaxis()->SetBinLabel(2, "Timing");

    hX1X2 = new TH1D("hX1X2", "hX1X2; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hY1Y2 = new TH1D("hY1Y2", "hY1Y2; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hX1X2_1px =
        new TH1D("hX1X2_1px", "hX1X2_1px; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hY1Y2_1px =
        new TH1D("hY1Y2_1px", "hY1Y2_1px; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hX1X2_2px =
        new TH1D("hX1X2_2px", "hX1X2_2px; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hY1Y2_2px =
        new TH1D("hY1Y2_2px", "hY1Y2_2px; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hX1X2_3px =
        new TH1D("hX1X2_3px", "hX1X2_3px; xdistance(cluster) - xdistance(closest pixel) [um]; # events", 2000, -1000, 1000);
    hY1Y2_3px =
        new TH1D("hY1Y2_3px", "hY1Y2_3px; ydistance(cluster) - ydistance(closest pixel) [um]; # events", 2000, -1000, 1000);
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

            // distance of track to cluster centre
            double xdistance_centre = std::abs(interceptLocal.X() - cluster->local().x());
            double ydistance_centre = std::abs(interceptLocal.Y() - cluster->local().y());

            // distance of track to nearest pixel: initialise to maximal possible value
            auto xdistance_nearest = std::numeric_limits<double>::max();
            auto ydistance_nearest = std::numeric_limits<double>::max();

            for(auto& pixel : (*cluster->pixels())) {
                // convert pixel address to local coordinates:
                auto pixelPositionLocal =
                    m_detector->getLocalPosition(static_cast<double>(pixel->column()), static_cast<double>(pixel->row()));

                xdistance_nearest = std::min(xdistance_nearest, std::abs(interceptLocal.X() - pixelPositionLocal.x()));
                ydistance_nearest = std::min(ydistance_nearest, std::abs(interceptLocal.Y() - pixelPositionLocal.y()));
            }

            hX1X2->Fill(xdistance_centre - xdistance_nearest);
            hY1Y2->Fill(ydistance_centre - ydistance_nearest);
            if(cluster->columnWidth() == 1) {
                hX1X2_1px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 1) {
                hY1Y2_1px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }
            if(cluster->columnWidth() == 2) {
                hX1X2_2px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 2) {
                hY1Y2_2px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }
            if(cluster->columnWidth() == 3) {
                hX1X2_3px->Fill(static_cast<double>(Units::convert(xdistance_centre - xdistance_nearest, "um")));
            }
            if(cluster->rowWidth() == 3) {
                hY1Y2_3px->Fill(static_cast<double>(Units::convert(ydistance_centre - ydistance_nearest, "um")));
            }

            // Check if the cluster is close in space (either use cluster centre of closest pixel to track)
            auto xdistance = (useClusterCentre ? xdistance_centre : xdistance_nearest);
            auto ydistance = (useClusterCentre ? ydistance_centre : ydistance_nearest);
            if(std::abs(xdistance) > spatialCut.x() || std::abs(ydistance) > spatialCut.y()) {
                LOG(DEBUG) << "Discarding DUT cluster with distance (" << Units::display(std::abs(xdistance), {"um", "mm"})
                           << "," << Units::display(std::abs(ydistance), {"um", "mm"}) << ")";
                hCutHisto->Fill(1);
                num_cluster++;
                continue;
            }

            // Check if the cluster is close in time
            if(std::abs(cluster->timestamp() - track->timestamp()) > timingCut) {
                LOG(DEBUG) << "Discarding DUT cluster with time difference "
                           << Units::display(std::abs(cluster->timestamp() - track->timestamp()), {"ms", "s"});
                hCutHisto->Fill(2);
                num_cluster++;
                continue;
            }

            LOG(DEBUG) << "Found associated cluster with distance (" << Units::display(abs(xdistance), {"um", "mm"}) << ","
                       << Units::display(abs(ydistance), {"um", "mm"}) << ")";
            track->addAssociatedCluster(cluster);
            assoc_cluster_counter++;
            num_cluster++;
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void DUTAssociation::finalise() {
    hCutHisto->Scale(1 / double(num_cluster));
    LOG(INFO) << "In total, " << assoc_cluster_counter << " clusters are associated to tracks.";
    return;
}
