#include "Timepix3Clustering.h"

using namespace corryvreckan;
using namespace std;

Timepix3Clustering::Timepix3Clustering(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    timingCut = m_config.get<double>("timingCut", 0.0000001); // 100 ns
}

void Timepix3Clustering::initialise() {

    timingCutInt = (timingCut * 4096. * 40000000.);
}

// Sort function for pixels from low to high times
bool sortByTime(Pixel* pixel1, Pixel* pixel2) {
    return (pixel1->m_timestamp < pixel2->m_timestamp);
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

            // Make the new cluster object
            Cluster* cluster = new Cluster();
            LOG(DEBUG) << "==== New cluster";

            // Keep adding hits to the cluster until no more are found
            cluster->addPixel(pixel);
            long long int clusterTime = pixel->m_timestamp;
            used[pixel] = true;
            LOG(DEBUG) << "Adding pixel: " << pixel->m_row << "," << pixel->m_column;
            int nPixels = 0;
            while(cluster->size() != nPixels) {

                nPixels = cluster->size();
                // Loop over all pixels
                for(int iNeighbour = (iP + 1); iNeighbour < totalPixels; iNeighbour++) {
                    Pixel* neighbour = (*pixels)[iNeighbour];
                    // Check if they are compatible in time with the cluster pixels
                    if((neighbour->m_timestamp - clusterTime) > timingCutInt)
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
                    clusterTime = neighbour->m_timestamp;
                    used[neighbour] = true;
                    LOG(DEBUG) << "Adding pixel: " << neighbour->m_row << "," << neighbour->m_column;
                }
            }

            // Finalise the cluster and save it
            calculateClusterCentre(cluster);
            deviceClusters->push_back(cluster);
        }

        // Put the clusters on the clipboard
        if(deviceClusters->size() > 0)
            clipboard->put(detector->name(), "clusters", (TestBeamObjects*)deviceClusters);
        LOG(DEBUG) << "Made " << deviceClusters->size() << " clusters for device " << detector->name();
    }

    return Success;
}

// Check if a pixel touches any of the pixels in a cluster
bool Timepix3Clustering::touching(Pixel* neighbour, Cluster* cluster) {

    bool Touching = false;
    Pixels* pixels = cluster->pixels();
    for(int iPix = 0; iPix < pixels->size(); iPix++) {

        if(abs((*pixels)[iPix]->m_row - neighbour->m_row) <= 1 &&
           abs((*pixels)[iPix]->m_column - neighbour->m_column) <= 1) {
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

        long long int timeDifference = abs(neighbour->m_timestamp - (*pixels)[iPix]->m_timestamp);
        if(timeDifference < timingCutInt)
            CloseInTime = true;
    }
    return CloseInTime;
}

void Timepix3Clustering::calculateClusterCentre(Cluster* cluster) {

    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0);
    long long int timestamp;

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->m_detectorID;
    timestamp = (*pixels)[0]->m_timestamp;

    // Loop over all pixels
    for(int pix = 0; pix < pixels->size(); pix++) {
        tot += (*pixels)[pix]->m_adc;
        row += ((*pixels)[pix]->m_row * (*pixels)[pix]->m_adc);
        column += ((*pixels)[pix]->m_column * (*pixels)[pix]->m_adc);
        if((*pixels)[pix]->m_timestamp < timestamp)
            timestamp = (*pixels)[pix]->m_timestamp;
    }
    // Row and column positions are tot-weighted
    row /= tot;
    column /= tot;

    auto detector = get_detector(detectorID);

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(
        detector->pitchX() * (column - detector->nPixelsX() / 2), detector->pitchY() * (row - detector->nPixelsY() / 2), 0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = *(detector->m_localToGlobal) * positionLocal;

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);
    cluster->setError(0.004);
    cluster->setTimestamp(timestamp);
    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(), positionGlobal.Z());
    cluster->setClusterCentreLocal(positionLocal.X(), positionLocal.Y(), positionLocal.Z());
}

void Timepix3Clustering::finalise() {}
