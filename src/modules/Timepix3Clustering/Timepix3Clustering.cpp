#include "Timepix3Clustering.h"

using namespace corryvreckan;
using namespace std;

Timepix3Clustering::Timepix3Clustering(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    timingCut = m_config.get<double>("timingCut", Units::convert(100, "ns")); // 100 ns
    neighbour_radius_row = m_config.get<int>("neighbour_radius_row", 1);
    neighbour_radius_col = m_config.get<int>("neighbour_radius_col", 1);
}

void Timepix3Clustering::initialise() {

    for(auto& detector : get_detectors()) {

        // Cluster plots
        string name = "clusterSize_" + detector->name();
        clusterSize[detector->name()] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);
        name = "clusterWidthRow_" + detector->name();
        clusterWidthRow[detector->name()] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);
        name = "clusterWidthColumn_" + detector->name();
        clusterWidthColumn[detector->name()] = new TH1F(name.c_str(), name.c_str(), 25, 0, 25);
        name = "clusterTot_" + detector->name();
        clusterTot[detector->name()] = new TH1F(name.c_str(), name.c_str(), 10000, 0, 100000);
        name = "clusterPositionGlobal_" + detector->name();
        clusterPositionGlobal[detector->name()] = new TH2F(name.c_str(), name.c_str(), 400, -10., 10., 400, -10., 10.);
    }
}

// Sort function for pixels from low to high times
bool sortByTime(Pixel* pixel1, Pixel* pixel2) {
    return (pixel1->timestamp() < pixel2->timestamp());
}

StatusCode Timepix3Clustering::run(Clipboard* clipboard) {

    // Loop over all Timepix3 and for each device perform the clustering
    for(auto& detector : get_detectors()) {

        // Check if they are a Timepix3
        if(detector->type() != "Timepix3")
            continue;

        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }
        LOG(DEBUG) << "Picked up " << pixels->size() << " pixels for device " << detector->name();

        //    if(pixels->size() > 500.){
        //      LOG(DEBUG) <<"Skipping large event with "<<pixels->size()<<" pixels
        //      for device "<<detector->name();
        //      continue;
        //    }

        // Sort the pixels from low to high timestamp
        std::sort(pixels->begin(), pixels->end(), sortByTime);
        int totalPixels = pixels->size();

        // Make the cluster storage
        Clusters* deviceClusters = new Clusters();

        // Keep track of which pixels are used
        map<Pixel*, bool> used;

        // Start to cluster
        for(int iP = 0; iP < pixels->size(); iP++) {
            Pixel* pixel = (*pixels)[iP];

            // Check if pixel is used
            if(used[pixel])
                continue;

            if(pixel->m_adc == 0.)
                continue;

            // Make the new cluster object
            Cluster* cluster = new Cluster();
            LOG(DEBUG) << "==== New cluster";

            // Keep adding hits to the cluster until no more are found
            cluster->addPixel(pixel);
            double clusterTime = pixel->timestamp();
            used[pixel] = true;
            LOG(DEBUG) << "Adding pixel: " << pixel->m_row << "," << pixel->m_column;
            int nPixels = 0;
            while(cluster->size() != nPixels) {

                nPixels = cluster->size();
                // Loop over all pixels
                for(int iNeighbour = (iP + 1); iNeighbour < totalPixels; iNeighbour++) {
                    Pixel* neighbour = (*pixels)[iNeighbour];
                    // Check if they are compatible in time with the cluster pixels
                    if((neighbour->timestamp() - clusterTime) > timingCut)
                        break;
                    //          if(!closeInTime(neighbour,cluster)) break;
                    // Check if they have been used
                    if(used[neighbour])
                        continue;

                    if(neighbour->m_adc == 0.)
                        continue;

                    // Check if they are touching cluster pixels
                    if(!touching(neighbour, cluster))
                        continue;

                    // Add to cluster
                    cluster->addPixel(neighbour);
                    clusterTime = neighbour->timestamp();
                    used[neighbour] = true;
                    LOG(DEBUG) << "Adding pixel: " << neighbour->m_row << "," << neighbour->m_column << " time "
                               << Units::display(neighbour->timestamp(), {"ns", "us", "s"});
                }
            }

            // Finalise the cluster and save it
            calculateClusterCentre(cluster);

            // Fill cluster histograms
            clusterSize[detector->name()]->Fill(cluster->size());
            clusterWidthRow[detector->name()]->Fill(cluster->rowWidth());
            clusterWidthColumn[detector->name()]->Fill(cluster->columnWidth());
            clusterTot[detector->name()]->Fill(cluster->tot());
            clusterPositionGlobal[detector->name()]->Fill(cluster->globalX(), cluster->globalY());

            deviceClusters->push_back(cluster);
        }

        // Put the clusters on the clipboard
        if(deviceClusters->size() > 0) {
            clipboard->put(detector->name(), "clusters", (Objects*)deviceClusters);
        }
        LOG(DEBUG) << "Made " << deviceClusters->size() << " clusters for device " << detector->name();
    }

    return Success;
}

// Check if a pixel touches any of the pixels in a cluster
bool Timepix3Clustering::touching(Pixel* neighbour, Cluster* cluster) {

    bool Touching = false;

    for(auto pixel : (*cluster->pixels())) {
        int row_distance = abs(pixel->m_row - neighbour->m_row);
        int col_distance = abs(pixel->m_column - neighbour->m_column);

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
bool Timepix3Clustering::closeInTime(Pixel* neighbour, Cluster* cluster) {

    bool CloseInTime = false;

    Pixels* pixels = cluster->pixels();
    for(int iPix = 0; iPix < pixels->size(); iPix++) {

        double timeDifference = abs(neighbour->timestamp() - (*pixels)[iPix]->timestamp());
        if(timeDifference < timingCut)
            CloseInTime = true;
    }
    return CloseInTime;
}

void Timepix3Clustering::calculateClusterCentre(Cluster* cluster) {

    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->detectorID();
    double timestamp = (*pixels)[0]->timestamp();

    // Loop over all pixels
    for(int pix = 0; pix < pixels->size(); pix++) {
        double pixelToT = (*pixels)[pix]->m_adc;
        if(pixelToT == 0) {
            LOG(DEBUG) << "Pixel with ToT 0!";
            pixelToT = 1;
        }
        tot += pixelToT;
        row += ((*pixels)[pix]->m_row * pixelToT);
        column += ((*pixels)[pix]->m_column * pixelToT);
        if((*pixels)[pix]->timestamp() < timestamp)
            timestamp = (*pixels)[pix]->timestamp();
    }
    // Row and column positions are tot-weighted
    row /= tot;
    column /= tot;
    auto detector = get_detector(detectorID);

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(
        detector->pitchX() * (column - detector->nPixelsX() / 2), detector->pitchY() * (row - detector->nPixelsY() / 2), 0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = *(detector->localToGlobal()) * positionLocal;

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);
    cluster->setErrorX(0.004);
    cluster->setErrorY(0.004);
    cluster->setTimestamp(timestamp);
    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(), positionGlobal.Z());
    cluster->setClusterCentreLocal(positionLocal.X(), positionLocal.Y(), positionLocal.Z());
}

void Timepix3Clustering::finalise() {}
