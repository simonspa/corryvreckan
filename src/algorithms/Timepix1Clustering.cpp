#include "Timepix1Clustering.h"
#include "objects/Pixel.h"

using namespace corryvreckan;
using namespace std;

Timepix1Clustering::Timepix1Clustering(Configuration config, Clipboard* clipboard)
    : Algorithm(std::move(config), clipboard) {}

/*

 This algorithm clusters the input data, at the moment only if the detector type
 is defined as Timepix1. The clustering is based on touching neighbours, and
 uses
 no timing information whatsoever. It is based on filling a map and checking
 neighbours in the neighbouring row and column positions.

*/

void Timepix1Clustering::initialise(Parameters* par) {

    parameters = par;

    // Initialise histograms per device
    for(int det = 0; det < parameters->nDetectors; det++) {

        // Check if they are a Timepix1
        string detectorID = parameters->detectors[det];
        if(parameters->detector[detectorID]->type() != "ATLASpix" &&
           parameters->detector[detectorID]->type() != "Timepix1" && parameters->detector[detectorID]->type() != "CLICpix")
            continue;
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode Timepix1Clustering::run(Clipboard* clipboard) {

    // Loop over all Timepix1 and make plots
    for(int det = 0; det < parameters->nDetectors; det++) {

        // Check if they are a Timepix1
        string detectorID = parameters->detectors[det];
        if(parameters->detector[detectorID]->type() != "ATLASpix" &&
           parameters->detector[detectorID]->type() != "Timepix1" && parameters->detector[detectorID]->type() != "CLICpix")
            continue;

        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detectorID, "pixels");
        if(pixels == NULL) {
            LOG(DEBUG) << "Detector " << detectorID << " does not have any pixels on the clipboard";
            continue;
        }

        // Make the cluster container and the maps for clustering
        TestBeamObjects* deviceClusters = new TestBeamObjects();
        map<Pixel*, bool> used;
        map<int, map<int, Pixel*>> hitmap;
        bool addedPixel;

        // Get the device dimensions
        int nRows = parameters->detector[detectorID]->nPixelsY();
        int nCols = parameters->detector[detectorID]->nPixelsX();

        // Fill the hitmap with pixels
        for(int iP = 0; iP < pixels->size(); iP++) {
            Pixel* pixel = (Pixel*)(*pixels)[iP];
            hitmap[pixel->m_row][pixel->m_column] = pixel;
            (pixel->m_adc)++; // temp fix for clicpix data
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
            used[pixel] = true;
            addedPixel = true;
            // Somewhere to store found neighbours
            Pixels neighbours;

            // Now we check the neighbours and keep adding more hits while there are
            // connected pixels
            while(addedPixel) {

                addedPixel = false;
                for(int row = (pixel->m_row - 1); row <= (pixel->m_row + 1); row++) {
                    // If out of bounds for row
                    if(row < 0 || row >= nRows)
                        continue;
                    for(int col = (pixel->m_column - 1); col <= (pixel->m_column + 1); col++) {
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
            calculateClusterCentre(cluster);
            deviceClusters->push_back(cluster);
        }

        clipboard->put(detectorID, "clusters", deviceClusters);
        LOG(DEBUG) << "Put " << deviceClusters->size() << " clusters on the clipboard for detector " << detectorID
                   << ". From " << pixels->size() << " pixels";
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void Timepix1Clustering::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}

/*
 Function to calculate the centre of gravity of a cluster.
 Sets the local and global cluster positions as well.
*/
void Timepix1Clustering::calculateClusterCentre(Cluster* cluster) {

    LOG(DEBUG) << "== Making cluster centre";
    // Empty variables to calculate cluster position
    double row(0), column(0), tot(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->m_detectorID;
    LOG(DEBUG) << "- cluster has " << (*pixels).size() << " pixels";

    // Loop over all pixels
    for(int pix = 0; pix < pixels->size(); pix++) {
        tot += (*pixels)[pix]->m_adc;
        row += ((*pixels)[pix]->m_row * (*pixels)[pix]->m_adc);
        column += ((*pixels)[pix]->m_column * (*pixels)[pix]->m_adc);
        LOG(DEBUG) << "- pixel row, col: " << (*pixels)[pix]->m_row << "," << (*pixels)[pix]->m_column;
    }

    // Row and column positions are tot-weighted
    row /= tot;
    column /= tot;

    LOG(DEBUG) << "- cluster row, col: " << row << "," << column;

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(
        parameters->detector[detectorID]->pitchX() * (column - parameters->detector[detectorID]->nPixelsX() / 2.),
        parameters->detector[detectorID]->pitchY() * (row - parameters->detector[detectorID]->nPixelsY() / 2.),
        0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal =
        *(parameters->detector[detectorID]->m_localToGlobal) * positionLocal;

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setTot(tot);
    cluster->setError(0.004);
    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(), positionGlobal.Z());
    cluster->setClusterCentreLocal(positionLocal.X(), positionLocal.Y(), positionLocal.Z());
}
