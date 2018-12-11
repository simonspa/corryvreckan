#include "ClusteringSpatial.h"
#include "objects/Pixel.hpp"

using namespace corryvreckan;
using namespace std;

ClusteringSpatial::ClusteringSpatial(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {}

/*

 This algorithm clusters the input data, at the moment only if the detector type
 is defined as Timepix1. The clustering is based on touching neighbours, and
 uses
 no timing information whatsoever. It is based on filling a map and checking
 neighbours in the neighbouring row and column positions.

*/

void ClusteringSpatial::initialise() {

    // Cluster plots
    std::string title = m_detector->name() + " Cluster size;cluster size;events";
    clusterSize = new TH1F("clusterSize", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster Width - Rows;cluster width [rows];events";
    clusterWidthRow = new TH1F("clusterWidthRow", title.c_str(), 25, 0, 25);
    title = m_detector->name() + " Cluster Width - Columns;cluster width [columns];events";
    clusterWidthColumn = new TH1F("clusterWidthColumn", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster Charge;cluster charge [ke];events";
    clusterTot = new TH1F("clusterTot", title.c_str(), 300, 0, 300);
    title = m_detector->name() + " Cluster Position (Global);x [mm];y [mm];events";
    clusterPositionGlobal = new TH2F("clusterPositionGlobal",
                                     title.c_str(),
                                     400,
                                     -m_detector->size().X() / 1.5,
                                     m_detector->size().X() / 1.5,
                                     400,
                                     -m_detector->size().Y() / 1.5,
                                     m_detector->size().Y() / 1.5);
}

StatusCode ClusteringSpatial::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the pixels
    auto pixels = clipboard->get<Pixels>(m_detector->name());
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }

    // Make the cluster container and the maps for clustering
    Clusters* deviceClusters = new Clusters();
    map<Pixel*, bool> used;
    map<int, map<int, Pixel*>> hitmap;
    bool addedPixel;

    // Get the device dimensions
    int nRows = m_detector->nPixels().Y();
    int nCols = m_detector->nPixels().X();

    // Pre-fill the hitmap with pixels
    for(auto pixel : (*pixels)) {
        hitmap[pixel->row()][pixel->column()] = pixel;
    }

    for(auto pixel : (*pixels)) {
        if(used[pixel]) {
            continue;
        }

        // New pixel => new cluster
        Cluster* cluster = new Cluster();
        cluster->addPixel(pixel);
        cluster->setTimestamp(pixel->timestamp());
        used[pixel] = true;
        addedPixel = true;
        // Somewhere to store found neighbours
        Pixels neighbours;

        // Now we check the neighbours and keep adding more hits while there are connected pixels
        while(addedPixel) {

            addedPixel = false;
            for(int row = pixel->row() - 1; row <= pixel->row() + 1; row++) {
                // If out of bounds for row
                if(row < 0 || row >= nRows) {
                    continue;
                }

                for(int col = pixel->column() - 1; col <= pixel->column() + 1; col++) {
                    // If out of bounds for column
                    if(col < 0 || col >= nCols) {
                        continue;
                    }

                    // If no pixel in this position, or is already in a cluster, do nothing
                    if(!hitmap[row][col]) {
                        continue;
                    }
                    if(used[hitmap[row][col]]) {
                        continue;
                    }

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
        calculateClusterCentre(cluster);

        // Fill cluster histograms
        clusterSize->Fill(static_cast<double>(cluster->size()));
        clusterWidthRow->Fill(cluster->rowWidth());
        clusterWidthColumn->Fill(cluster->columnWidth());
        clusterTot->Fill(cluster->tot() * 1e-3);
        clusterPositionGlobal->Fill(cluster->global().x(), cluster->global().y());

        deviceClusters->push_back(cluster);
    }

    clipboard->put(deviceClusters, m_detector->name());
    LOG(DEBUG) << "Put " << deviceClusters->size() << " clusters on the clipboard for detector " << m_detector->name()
               << ". From " << pixels->size() << " pixels";

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

/*
 Function to calculate the centre of gravity of a cluster.
 Sets the local and global cluster positions as well.
*/
void ClusteringSpatial::calculateClusterCentre(Cluster* cluster) {

    LOG(DEBUG) << "== Making cluster centre";
    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->detectorID();
    LOG(DEBUG) << "- cluster has " << (*pixels).size() << " pixels";

    // Loop over all pixels
    for(auto& pixel : (*pixels)) {
        tot += pixel->adc();
        row += (pixel->row() * pixel->adc());
        column += (pixel->column() * pixel->adc());
        LOG(DEBUG) << "- pixel row, col: " << pixel->row() << "," << pixel->column();
    }

    // Row and column positions are tot-weighted
    row /= (tot > 0 ? tot : 1);
    column /= (tot > 0 ? tot : 1);

    LOG(DEBUG) << "- cluster row, col: " << row << "," << column;

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(m_detector->pitch().X() * (column - m_detector->nPixels().X() / 2.),
                                                        m_detector->pitch().Y() * (row - m_detector->nPixels().Y() / 2.),
                                                        0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);

    // Set uncertainty on position from intrinstic detector resolution:
    cluster->setError(m_detector->resolution());

    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}
