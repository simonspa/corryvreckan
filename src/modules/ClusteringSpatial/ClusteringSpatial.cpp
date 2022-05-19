/**
 * @file
 * @brief Implementation of module ClusteringSpatial
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ClusteringSpatial.h"
#include "objects/Pixel.hpp"

using namespace corryvreckan;
using namespace std;

ClusteringSpatial::ClusteringSpatial(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<bool>("use_trigger_timestamp", false);
    config_.setDefault<bool>("charge_weighting", true);
    config_.setDefault<bool>("reject_by_roi", false);

    useTriggerTimestamp = config_.get<bool>("use_trigger_timestamp");
    chargeWeighting = config_.get<bool>("charge_weighting");
    rejectByROI = config_.get<bool>("reject_by_roi");
}

void ClusteringSpatial::initialize() {

    // Cluster plots
    std::string title = m_detector->getName() + " Cluster size;cluster size;events";
    clusterSize = new TH1F("clusterSize", title.c_str(), 100, -0.5, 99.5);
    title = m_detector->getName() + " Cluster seed charge;cluster seed charge [e];events";
    clusterSeedCharge = new TH1F("clusterSeedCharge", title.c_str(), 256, -0.5, 255.5);
    title = m_detector->getName() + " Cluster Width - Rows;cluster width [rows];events";
    clusterWidthRow = new TH1F("clusterWidthRow", title.c_str(), 25, -0.5, 24.5);
    title = m_detector->getName() + " Cluster Width - Columns;cluster width [columns];events";
    clusterWidthColumn = new TH1F("clusterWidthColumn", title.c_str(), 100, -0.5, 99.5);
    title = m_detector->getName() + " Cluster Charge;cluster charge [e];events";
    clusterCharge = new TH1F("clusterCharge", title.c_str(), 5000, -0.5, 49999.5);
    title = m_detector->getName() + " Cluster Position (Global);x [mm];y [mm];events";
    clusterPositionGlobal = new TH2F("clusterPositionGlobal",
                                     title.c_str(),
                                     400,
                                     -m_detector->getSize().X() / 1.5,
                                     m_detector->getSize().X() / 1.5,
                                     400,
                                     -m_detector->getSize().Y() / 1.5,
                                     m_detector->getSize().Y() / 1.5);
    title = m_detector->getName() + " Cluster Position (Local);x [px];y [px];events";
    clusterPositionLocal = new TH2F("clusterPositionLocal",
                                    title.c_str(),
                                    m_detector->nPixels().X(),
                                    -0.5,
                                    m_detector->nPixels().X() - 0.5,
                                    m_detector->nPixels().Y(),
                                    -0.5,
                                    m_detector->nPixels().Y() - 0.5);

    title = ";cluster timestamp [ns]; # events";
    clusterTimes = new TH1F("clusterTimes", title.c_str(), 3e6, 0, 3e9);
    title = m_detector->getName() + " Cluster multiplicity;clusters;events";
    clusterMultiplicity = new TH1F("clusterMultiplicity", title.c_str(), 50, -0.5, 49.5);
}

StatusCode ClusteringSpatial::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the pixels
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    if(pixels.empty()) {
        LOG(DEBUG) << "Detector " << m_detector->getName() << " does not have any pixels on the clipboard";
        return StatusCode::Success;
    }

    // Make the cluster container and the maps for clustering
    ClusterVector deviceClusters;
    map<std::shared_ptr<Pixel>, bool> used;
    map<int, map<int, std::shared_ptr<Pixel>>> hitmap;
    bool addedPixel;

    // Get the device dimensions
    int nRows = m_detector->nPixels().Y();
    int nCols = m_detector->nPixels().X();

    // Pre-fill the hitmap with pixels
    for(auto pixel : pixels) {
        hitmap[pixel->column()][pixel->row()] = pixel;
    }

    for(auto pixel : pixels) {
        if(used[pixel]) {
            continue;
        }

        // New pixel => new cluster
        auto cluster = std::make_shared<Cluster>();
        cluster->addPixel(&*pixel);

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
        // Somewhere to store found neighbors
        PixelVector neighbors;

        // Now we check the neighbors and keep adding more hits while there are connected pixels
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
                    // neighbor
                    cluster->addPixel(&*hitmap[col][row]);
                    used[hitmap[col][row]] = true;
                    neighbors.push_back(hitmap[col][row]);
                }
            }

            // If we have neighbors that have not yet been checked, continue
            // looking for more pixels
            if(neighbors.size() > 0) {
                addedPixel = true;
                pixel = neighbors.back();
                neighbors.pop_back();
            }
        }

        // Finalise the cluster and save it
        calculateClusterCentre(cluster.get());

        // check if the cluster is within ROI
        if(rejectByROI && !m_detector->isWithinROI(cluster.get())) {
            LOG(DEBUG) << "Rejecting cluster outside of " << m_detector->getName() << " ROI";
            continue;
        }

        // Fill cluster histograms
        clusterSize->Fill(static_cast<double>(cluster->size()));

        clusterWidthRow->Fill(static_cast<double>(cluster->rowWidth()));
        clusterWidthColumn->Fill(static_cast<double>(cluster->columnWidth()));
        clusterCharge->Fill(cluster->charge());
        clusterSeedCharge->Fill(cluster->getSeedPixel()->charge());
        clusterPositionGlobal->Fill(cluster->global().x(), cluster->global().y());
        clusterPositionLocal->Fill(cluster->column(), cluster->row());
        clusterTimes->Fill(static_cast<double>(Units::convert(cluster->timestamp(), "ns")));
        LOG(DEBUG) << "cluster local: " << cluster->local();

        deviceClusters.push_back(cluster);
    }

    clusterMultiplicity->Fill(static_cast<double>(deviceClusters.size()));

    clipboard->putData(deviceClusters, m_detector->getName());
    LOG(DEBUG) << "Put " << deviceClusters.size() << " clusters on the clipboard for detector " << m_detector->getName()
               << ". From " << pixels.size() << " pixels";

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
        // If charge == 0 (use epsilon to avoid errors in floating-point arithmetic):
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

    // Set uncertainty on position from intrinstic detector spatial resolution:
    cluster->setError(m_detector->getSpatialResolution());

    cluster->setDetectorID(detectorID);
    cluster->setClusterCentre(positionGlobal);
    cluster->setClusterCentreLocal(positionLocal);
}
