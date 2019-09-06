#include "Clustering4D.h"

using namespace corryvreckan;
using namespace std;

Clustering4D::Clustering4D(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    timingCut = m_config.get<double>("timing_cut", Units::get<double>(100, "ns"));
    neighbour_radius_row = m_config.get<int>("neighbour_radius_row", 1);
    neighbour_radius_col = m_config.get<int>("neighbour_radius_col", 1);
}

void Clustering4D::initialise() {

    // Cluster plots
    std::string title = m_detector->name() + " Cluster size;cluster size;events";
    clusterSize = new TH1F("clusterSize", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster seed charge;cluster seed charge [e];events";
    clusterSeedCharge = new TH1F("clusterSeedCharge", title.c_str(), 256, 0, 256);
    title = m_detector->name() + " Cluster Width - Rows;cluster width [rows];events";
    clusterWidthRow = new TH1F("clusterWidthRow", title.c_str(), 25, 0, 25);
    title = m_detector->name() + " Cluster Width - Columns;cluster width [columns];events";
    clusterWidthColumn = new TH1F("clusterWidthColumn", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster Charge;cluster charge [e];events";
    clusterCharge = new TH1F("clusterCharge", title.c_str(), 5000, 0, 50000);
    title = m_detector->name() + " Cluster Position (Global);x [mm];y [mm];events";
    clusterPositionGlobal = new TH2F("clusterPositionGlobal", title.c_str(), 400, -10., 10., 400, -10., 10.);
    title = ";cluster timestamp [ns]; # events";
    clusterTimes = new TH1F("clusterTimes", title.c_str(), 3e6, 0, 3e9);
    title = m_detector->name() + " Cluster multiplicity;clusters;events";
    clusterMultiplicity = new TH1F("clusterMultiplicity", title.c_str(), 50, 0, 50);
}

// Sort function for pixels from low to high times
bool Clustering4D::sortByTime(Pixel* pixel1, Pixel* pixel2) {
    return (pixel1->timestamp() < pixel2->timestamp());
}

StatusCode Clustering4D::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the pixels
    auto pixels = clipboard->get<Pixel>(m_detector->name());
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }
    LOG(DEBUG) << "Picked up " << pixels->size() << " pixels for device " << m_detector->name();

    // Sort the pixels from low to high timestamp
    std::sort(pixels->begin(), pixels->end(), sortByTime);
    size_t totalPixels = pixels->size();

    // Make the cluster storage
    auto deviceClusters = std::make_shared<ClusterVector>();

    // Keep track of which pixels are used
    map<Pixel*, bool> used;

    // Start to cluster
    for(size_t iP = 0; iP < pixels->size(); iP++) {
        Pixel* pixel = (*pixels)[iP];

        // Check if pixel is used
        if(used[pixel]) {
            continue;
        }

        // Make the new cluster object
        Cluster* cluster = new Cluster();
        LOG(DEBUG) << "==== New cluster";

        // Keep adding hits to the cluster until no more are found
        cluster->addPixel(pixel);
        double clusterTime = pixel->timestamp();
        used[pixel] = true;
        LOG(DEBUG) << "Adding pixel: " << pixel->column() << "," << pixel->row();
        size_t nPixels = 0;
        while(cluster->size() != nPixels) {

            nPixels = cluster->size();
            // Loop over all pixels
            for(size_t iNeighbour = (iP + 1); iNeighbour < totalPixels; iNeighbour++) {
                Pixel* neighbour = (*pixels)[iNeighbour];
                // Check if they are compatible in time with the cluster pixels
                if((neighbour->timestamp() - clusterTime) > timingCut)
                    break;
                //          if(!closeInTime(neighbour,cluster)) break;
                // Check if they have been used
                if(used[neighbour])
                    continue;

                // Check if they are touching cluster pixels
                if(!touching(neighbour, cluster))
                    continue;

                // Add to cluster
                cluster->addPixel(neighbour);
                clusterTime = neighbour->timestamp();
                used[neighbour] = true;
                LOG(DEBUG) << "Adding pixel: " << neighbour->column() << "," << neighbour->row() << " time "
                           << Units::display(neighbour->timestamp(), {"ns", "us", "s"});
            }
        }

        // Finalise the cluster and save it
        calculateClusterCentre(cluster);

        // Fill cluster histograms
        clusterSize->Fill(static_cast<double>(cluster->size()));
        clusterWidthRow->Fill(cluster->rowWidth());
        clusterWidthColumn->Fill(cluster->columnWidth());
        clusterCharge->Fill(cluster->charge());
        clusterSeedCharge->Fill(cluster->getSeedPixel()->charge());
        clusterPositionGlobal->Fill(cluster->global().x(), cluster->global().y());
        clusterTimes->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "ns")));

        deviceClusters->push_back(cluster);
    }

    clusterMultiplicity->Fill(static_cast<double>(deviceClusters->size()));

    // Put the clusters on the clipboard
    clipboard->put(deviceClusters, m_detector->name());
    LOG(DEBUG) << "Made " << deviceClusters->size() << " clusters for device " << m_detector->name();

    return StatusCode::Success;
}

// Check if a pixel touches any of the pixels in a cluster
bool Clustering4D::touching(Pixel* neighbour, Cluster* cluster) {

    bool Touching = false;

    for(auto pixel : cluster->pixels()) {
        int row_distance = abs(pixel->row() - neighbour->row());
        int col_distance = abs(pixel->column() - neighbour->column());

        if(row_distance <= neighbour_radius_row && col_distance <= neighbour_radius_col) {
            if(row_distance > 1 || col_distance > 1) {
                cluster->setSplit(true);
            }
            Touching = true;
            break;
        }
    }
    return Touching;
}

// Check if a pixel is close in time to the pixels of a cluster
bool Clustering4D::closeInTime(Pixel* neighbour, Cluster* cluster) {

    bool CloseInTime = false;

    auto pixels = cluster->pixels();
    for(auto& px : pixels) {

        double timeDifference = abs(neighbour->timestamp() - px->timestamp());
        if(timeDifference < timingCut)
            CloseInTime = true;
    }
    return CloseInTime;
}

void Clustering4D::calculateClusterCentre(Cluster* cluster) {

    LOG(DEBUG) << "== Making cluster centre";
    // Empty variables to calculate cluster position
    double column(0), row(0), charge(0);

    // Get the pixels on this cluster
    auto pixels = cluster->pixels();
    string detectorID = pixels.front()->detectorID();
    double timestamp = pixels.front()->timestamp();
    LOG(DEBUG) << "- cluster has " << pixels.size() << " pixels";

    // Loop over all pixels
    for(auto& pixel : pixels) {
        charge += pixel->charge();
        column += (pixel->column() * pixel->charge());
        row += (pixel->row() * pixel->charge());
        if(pixel->timestamp() < timestamp) {
            timestamp = pixel->timestamp();
        }
    }

    // Column and row positions are charge-weighted
    // If charge == 0 (use epsilon to avoid errors in floating-point arithmetics)
    // calculate simple arithmetic mean
    column /= (charge > std::numeric_limits<double>::epsilon() ? charge : 1);
    row /= (charge > std::numeric_limits<double>::epsilon() ? charge : 1);

    LOG(DEBUG) << "- cluster col, row: " << column << "," << row;

    if(detectorID != m_detector->name()) {
        // Should never happen...
        return;
    }

    // Calculate local cluster position
    auto positionLocal = m_detector->getLocalPosition(column, row);

    // Calculate global cluster position
    auto positionGlobal = m_detector->localToGlobal(positionLocal);

    // Set the cluster parameters
    cluster->setColumn(column);
    cluster->setRow(row);
    cluster->setCharge(charge);

    // Set uncertainty on position from intrinstic detector resolution:
    cluster->setError(m_detector->resolution());

    cluster->setTimestamp(timestamp);
    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}
