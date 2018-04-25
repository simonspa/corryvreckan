#include "SpatialClustering.h"
#include "objects/Pixel.h"

using namespace corryvreckan;
using namespace std;

SpatialClustering::SpatialClustering(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {}

/*

 This algorithm clusters the input data, at the moment only if the detector type
 is defined as Timepix1. The clustering is based on touching neighbours, and
 uses
 no timing information whatsoever. It is based on filling a map and checking
 neighbours in the neighbouring row and column positions.

*/

void SpatialClustering::initialise() {

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
        clusterPositionGlobal[detector->name()] = new TH2F(name.c_str(),
                                                           name.c_str(),
                                                           400,
                                                           -detector->size().X() / 1.5,
                                                           detector->size().X() / 1.5,
                                                           400,
                                                           -detector->size().Y() / 1.5,
                                                           detector->size().Y() / 1.5);
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode SpatialClustering::run(Clipboard* clipboard) {

    // Loop over all detectors of this algorithm:
    for(auto& detector : get_detectors()) {
        LOG(TRACE) << "Executing loop for detector " << detector->name();

        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }

        // Make the cluster container and the maps for clustering
        Objects* deviceClusters = new Objects();
        map<Pixel*, bool> used;
        map<int, map<int, Pixel*>> hitmap;
        bool addedPixel;

        // Get the device dimensions
        int nRows = detector->nPixelsY();
        int nCols = detector->nPixelsX();

        // Fill the hitmap with pixels
        for(int iP = 0; iP < pixels->size(); iP++) {
            Pixel* pixel = (Pixel*)(*pixels)[iP];
            hitmap[pixel->row()][pixel->column()] = pixel;
        }

        // Loop over all pixels and make clusters
        for(int iP = 0; iP < pixels->size(); iP++) {

            // Get the pixel. If it is already used then do nothing
            Pixel* pixel = (Pixel*)(*pixels)[iP];
            if(used[pixel])
                continue;

            // New pixel => new cluster
            Cluster* cluster = new Cluster();
            cluster->addPixel(pixel);
            cluster->setTimestamp(pixel->timestamp());
            used[pixel] = true;
            addedPixel = true;
            // Somewhere to store found neighbours
            Pixels neighbours;

            // Now we check the neighbours and keep adding more hits while there are
            // connected pixels
            while(addedPixel) {

                addedPixel = false;
                for(int row = (pixel->row() - 1); row <= (pixel->row() + 1); row++) {
                    // If out of bounds for row
                    if(row < 0 || row >= nRows)
                        continue;
                    for(int col = (pixel->column() - 1); col <= (pixel->column() + 1); col++) {
                        // If out of bounds for column
                        if(col < 0 || col >= nCols)
                            continue;

                        // If no pixel in this position, or is already in a cluster, do
                        // nothing
                        if(!hitmap[row][col])
                            continue;
                        if(used[hitmap[row][col]])
                            continue;

                        // Otherwise add the pixel to the cluster and store it as a found
                        // neighbour
                        cluster->addPixel(hitmap[row][col]);
                        used[hitmap[row][col]] = true;
                        neighbours.push_back(hitmap[row][col]);
                    }
                }

                // If we have neighbours that have not yet been checked, continue
                // looking for more pixels
                if(neighbours.size() > 0) {
                    addedPixel = true;
                    pixel = neighbours.back();
                    neighbours.pop_back();
                }
            }

            // Finalise the cluster and save it
            calculateClusterCentre(detector, cluster);

            // Fill cluster histograms
            clusterSize[detector->name()]->Fill(cluster->size());
            clusterWidthRow[detector->name()]->Fill(cluster->rowWidth());
            clusterWidthColumn[detector->name()]->Fill(cluster->columnWidth());
            clusterTot[detector->name()]->Fill(cluster->tot());
            clusterPositionGlobal[detector->name()]->Fill(cluster->globalX(), cluster->globalY());

            deviceClusters->push_back(cluster);
        }

        clipboard->put(detector->name(), "clusters", deviceClusters);
        LOG(DEBUG) << "Put " << deviceClusters->size() << " clusters on the clipboard for detector " << detector->name()
                   << ". From " << pixels->size() << " pixels";
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void SpatialClustering::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}

/*
 Function to calculate the centre of gravity of a cluster.
 Sets the local and global cluster positions as well.
*/
void SpatialClustering::calculateClusterCentre(Detector* detector, Cluster* cluster) {

    LOG(DEBUG) << "== Making cluster centre";
    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->detectorID();
    LOG(DEBUG) << "- cluster has " << (*pixels).size() << " pixels";

    // Loop over all pixels
    for(int pix = 0; pix < pixels->size(); pix++) {
        tot += (*pixels)[pix]->adc();
        row += ((*pixels)[pix]->row() * (*pixels)[pix]->adc());
        column += ((*pixels)[pix]->column() * (*pixels)[pix]->adc());
        LOG(DEBUG) << "- pixel row, col: " << (*pixels)[pix]->row() << "," << (*pixels)[pix]->column();
    }

    // Row and column positions are tot-weighted
    row /= tot;
    column /= tot;

    LOG(DEBUG) << "- cluster row, col: " << row << "," << column;

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(detector->pitch().X() * (column - detector->nPixelsX() / 2.),
                                                        detector->pitch().Y() * (row - detector->nPixelsY() / 2.),
                                                        0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = detector->localToGlobal(positionLocal);

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);
    cluster->setErrorX(0.004);
    cluster->setErrorY(0.004);
    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(), positionGlobal.Z());
    cluster->setClusterCentreLocal(positionLocal.X(), positionLocal.Y(), positionLocal.Z());
}
