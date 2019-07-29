#include "ClusteringSpatial.h"
#include "objects/Pixel.hpp"

using namespace corryvreckan;
using namespace std;

ClusteringSpatial::ClusteringSpatial(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    useTriggerTimestamp = m_config.get<bool>("use_trigger_timestamp", false);
}

void ClusteringSpatial::initialise() {

    // Cluster plots
    std::string title = m_detector->name() + " Cluster size;cluster size;events";
    clusterSize = new TH1F("clusterSize", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster seed;cluster seed;events";
    clusterSeed = new TH1F("clusterSeed", title.c_str(), 256, 0, 256);
    title = m_detector->name() + " Cluster Width - Rows;cluster width [rows];events";
    clusterWidthRow = new TH1F("clusterWidthRow", title.c_str(), 25, 0, 25);
    title = m_detector->name() + " Cluster Width - Columns;cluster width [columns];events";
    clusterWidthColumn = new TH1F("clusterWidthColumn", title.c_str(), 100, 0, 100);
    title = m_detector->name() + " Cluster Charge;cluster charge [ke];events";
    clusterCharge = new TH1F("clusterCharge", title.c_str(), 300, 0, 300);
    title = m_detector->name() + " Cluster Position (Global);x [mm];y [mm];events";
    clusterPositionGlobal = new TH2F("clusterPositionGlobal",
                                     title.c_str(),
                                     400,
                                     -m_detector->size().X() / 1.5,
                                     m_detector->size().X() / 1.5,
                                     400,
                                     -m_detector->size().Y() / 1.5,
                                     m_detector->size().Y() / 1.5);
    title = m_detector->name() + " Cluster Position (Local);x [px];y [px];events";
    clusterPositionLocal = new TH2F("clusterPositionLocal",
                                    title.c_str(),
                                    m_detector->nPixels().X(),
                                    -m_detector->nPixels().X() / 2.,
                                    m_detector->nPixels().X() / 2.,
                                    m_detector->nPixels().Y(),
                                    -m_detector->nPixels().Y() / 2.,
                                    m_detector->nPixels().Y() / 2.);

    title = ";cluster timestamp [ns]; # events";
    clusterTimes = new TH1F("clusterTimes", title.c_str(), 3e6, 0, 3e9);
}

StatusCode ClusteringSpatial::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the pixels
    Pixels* pixels = reinterpret_cast<Pixels*>(clipboard->get(m_detector->name(), "pixels"));
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }

    // Make the cluster container and the maps for clustering
    Objects* deviceClusters = new Objects();
    map<Pixel*, bool> used;
    map<int, map<int, Pixel*>> hitmap;
    bool addedPixel;

    // Get the device dimensions
    int nRows = m_detector->nPixels().Y();
    int nCols = m_detector->nPixels().X();

    // Pre-fill the hitmap with pixels
    for(auto pixel : (*pixels)) {
        hitmap[pixel->column()][pixel->row()] = pixel;
    }

    for(auto pixel : (*pixels)) {
        if(used[pixel]) {
            continue;
        }

        // New pixel => new cluster
        Cluster* cluster = new Cluster();
        cluster->addPixel(pixel);

        if(useTriggerTimestamp) {
            if(!clipboard->get_event()->triggerList().empty()) {
                double trigger_ts = clipboard->get_event()->triggerList().begin()->second;
                LOG(DEBUG) << "Using trigger timestamp " << Units::display(trigger_ts, "us") << " as cluster timestamp.";
                cluster->setTimestamp(trigger_ts);
            } else {
                LOG(WARNING) << "No trigger available. Use pixel timestamp " << Units::display(pixel->timestamp(), "us")
                             << " as cluster timestamp.";
                cluster->setTimestamp(pixel->timestamp());
            }
        } else {
            // assign pixel timestamp
            LOG(DEBUG) << "Pixel has timestamp " << Units::display(pixel->timestamp(), "us")
                       << ", set as cluster timestamp. ";
            cluster->setTimestamp(pixel->timestamp());
        }

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
                    if(!hitmap[col][row]) {
                        continue;
                    }
                    if(used[hitmap[col][row]]) {
                        continue;
                    }

                    // Otherwise add the pixel to the cluster and store it as a found
                    // neighbour
                    cluster->addPixel(hitmap[col][row]);
                    used[hitmap[col][row]] = true;
                    neighbours.push_back(hitmap[col][row]);
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

        // Get seed pixel
        auto seedPixel = cluster->getSeedPixel();

        // Fill cluster histograms
        clusterSize->Fill(static_cast<double>(cluster->size()));
        clusterSeed->Fill(seedPixel->charge());
        clusterWidthRow->Fill(cluster->rowWidth());
        clusterWidthColumn->Fill(cluster->columnWidth());
        clusterCharge->Fill(cluster->charge() * 1e-3); //  1e-3 because unit is [ke]
        clusterPositionGlobal->Fill(cluster->global().x(), cluster->global().y());
        clusterPositionLocal->Fill(cluster->local().x(), cluster->local().y());
        clusterTimes->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "ns")));
        LOG(DEBUG) << "cluster local: " << cluster->local();
        deviceClusters->push_back(cluster);
    }

    clipboard->put(m_detector->name(), "clusters", deviceClusters);
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
    double column(0), row(0), charge(0);

    // Get the pixels on this cluster
    Pixels* pixels = cluster->pixels();
    string detectorID = (*pixels)[0]->detectorID();
    LOG(DEBUG) << "- cluster has " << (*pixels).size() << " pixels";

    // Loop over all pixels
    for(auto& pixel : (*pixels)) {
        charge += pixel->charge();
        column += (pixel->column() * pixel->charge());
        row += (pixel->row() * pixel->charge());

        LOG(DEBUG) << "- pixel col, row: " << pixel->column() << "," << pixel->row();
    }

    // Column and row positions are charge-weighted
    // If charge == 0 (use epsilon to avoid errors in floating-point arithmetics)
    // calculate simple arithmetic mean
    column /= (charge > std::numeric_limits<double>::epsilon() ? charge : 1);
    row /= (charge > std::numeric_limits<double>::epsilon() ? charge : 1);

    LOG(DEBUG) << "- cluster col, row: " << column << "," << row << " at time "
               << Units::display(cluster->timestamp(), "us");

    // Create object with local cluster position
    PositionVector3D<Cartesian3D<double>> positionLocal(m_detector->pitch().X() * (column - m_detector->nPixels().X() / 2.),
                                                        m_detector->pitch().Y() * (row - m_detector->nPixels().Y() / 2.),
                                                        0);
    // Calculate global cluster position
    PositionVector3D<Cartesian3D<double>> positionGlobal = m_detector->localToGlobal(positionLocal);

    // Set the cluster parameters
    cluster->setRow(row);
    cluster->setColumn(column);
    cluster->setCharge(charge);

    // Set uncertainty on position from intrinstic detector resolution:
    cluster->setError(m_detector->resolution());

    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}
