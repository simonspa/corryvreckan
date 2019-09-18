#include "ClusteringSpatial.h"
#include "objects/Pixel.hpp"

using namespace corryvreckan;
using namespace std;

ClusteringSpatial::ClusteringSpatial(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    useTriggerTimestamp = m_config.get<bool>("use_trigger_timestamp", false);
    chargeWeighting = m_config.get<bool>("charge_weighting", true);
}

void ClusteringSpatial::initialise() {

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
    auto pixels = clipboard->getData<Pixel>(m_detector->name());
    if(pixels == nullptr) {
        LOG(DEBUG) << "Detector " << m_detector->name() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }

    // Make the cluster container and the maps for clustering
    auto deviceClusters = std::make_shared<ClusterVector>();
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
            if(!clipboard->getEvent()->triggerList().empty()) {
                double trigger_ts = clipboard->getEvent()->triggerList().begin()->second;
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
        PixelVector neighbours;

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

        // Fill cluster histograms
        clusterSize->Fill(static_cast<double>(cluster->size()));

        clusterWidthRow->Fill(cluster->rowWidth());
        clusterWidthColumn->Fill(cluster->columnWidth());
        clusterCharge->Fill(cluster->charge());
        clusterSeedCharge->Fill(cluster->getSeedPixel()->charge());
        clusterPositionGlobal->Fill(cluster->global().x(), cluster->global().y());
        clusterPositionLocal->Fill(cluster->local().x(), cluster->local().y());
        clusterTimes->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "ns")));
        LOG(DEBUG) << "cluster local: " << cluster->local();
        deviceClusters->push_back(cluster);
    }

    clipboard->putData(deviceClusters, m_detector->name());
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
    double column_sum(0), column_sum_chargeweighted(0);
    double row_sum(0), row_sum_chargeweighted(0);
    bool found_charge_zero = false;

    // Get the pixels on this cluster
    auto pixels = cluster->pixels();
    string detectorID = pixels.front()->detectorID();
    LOG(DEBUG) << "- cluster has " << pixels.size() << " pixels";

    // Loop over all pixels
    for(auto& pixel : pixels) {
        // If charge == 0 (use epsilon to avoid errors in floating-point arithmetics):
        if(pixel->charge() < std::numeric_limits<double>::epsilon()) {
            // apply arithmetic mean if a pixel has zero charge
            found_charge_zero = true;
        }
        charge += pixel->charge();

        // We need both column_sum and column_sum_chargeweighted
        // as we don't know a priori if there will be a pixel with
        // charge==0 such that we have to fall back to the arithmetic mean.
        column_sum += pixel->column();
        row_sum += pixel->row();
        column_sum_chargeweighted += (pixel->column() * pixel->charge());
        row_sum_chargeweighted += (pixel->row() * pixel->charge());

        LOG(DEBUG) << "- pixel col, row: " << pixel->column() << "," << pixel->row();
    }

    if(chargeWeighting && !found_charge_zero) {
        // Charge-weighted centre-of-gravity for cluster centre:
        // (here it's safe to divide by the charge as it cannot be zero due to !found_charge_zero)
        column = column_sum_chargeweighted / charge;
        row = row_sum_chargeweighted / charge;
    } else {
        // Arithmetic cluster centre:
        column = column_sum / static_cast<double>(cluster->size());
        row = row_sum / static_cast<double>(cluster->size());
    }

    LOG(DEBUG) << "- cluster col, row: " << column << "," << row << " at time "
               << Units::display(cluster->timestamp(), "us");

    // Create object with local cluster position
    auto positionLocal = m_detector->getLocalPosition(column, row);

    // Calculate global cluster position
    auto positionGlobal = m_detector->localToGlobal(positionLocal);

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
