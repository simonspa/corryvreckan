#include "DataOutput.h"
#include <vector>

using namespace corryvreckan;
using namespace std;

DataOutput::DataOutput(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {

    m_fileName = m_config.get<std::string>("fileName", "outputTuples.root");
    m_treeName = m_config.get<std::string>("treeName", "tree");
}

/*

 This algorithm writes an output file and fills it with trees containing
 the following information for the DUT:

 1)size in x of each cluster,
 2)size in y of each cluster,
 3)the event ID of each cluster,
 4)a list of the X positions of the pixels of each cluster,
 5)a list of the Y positions of the pixels of each cluster,
 6)a list of the ToT of the pixels of each cluster,
 7)a list of the timestamp/ToA of the pixels of each cluster,
 the number of pixels in each cluster (so that in the analysis the lists
 of pixel X positions etc. can be split into their respective clusters).
 8)the intercept with the DUT for each track

 */

void DataOutput::initialise() {
    LOG(DEBUG) << "Initialised DataOutput";

    // Create output file and directories
    m_outputFile = new TFile(m_fileName.c_str(), "RECREATE");
    LOG(DEBUG) << "Made and moved to output file: " << m_fileName;
    gDirectory->Delete("tree;*");
    m_outputTree = new TTree(m_treeName.c_str(), m_treeName.c_str());
    LOG(DEBUG) << "Created tree: " << m_treeName;

    eventID = 0;
    filledEvents = 0;

    // Create the output branches
    m_outputTree->Branch("EventID", &v_clusterEventID);
    m_outputTree->Branch("clusterSizeX", &v_clusterSizeX);
    m_outputTree->Branch("clusterSizeY", &v_clusterSizeY);
    m_outputTree->Branch("pixelX", &v_pixelX);
    m_outputTree->Branch("pixelY", &v_pixelY);
    m_outputTree->Branch("pixelToT", &v_pixelToT);
    m_outputTree->Branch("pixelToA", &v_pixelToA);
    m_outputTree->Branch("clusterNumPixels", &v_clusterNumPixels);
    // Branch for track intercepts
    m_outputTree->Branch("intercepts", &v_intercepts);
}

StatusCode DataOutput::run(Clipboard* clipboard) {
    // Counter for cluster event ID
    eventID++;

    // Get the DUT
    auto DUT = get_detector(m_config.get<std::string>("DUT"));
    v_intercepts.clear();

    // Clear data vectors before storing the cluster information for
    // this event
    v_clusterSizeX.clear();
    v_clusterSizeY.clear();
    v_clusterEventID.clear();
    v_pixelX.clear();
    v_pixelY.clear();
    v_pixelToT.clear();
    v_pixelToA.clear();
    v_clusterNumPixels.clear();

    // Getting tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Iterate through tracks found
    for(auto& track : (*tracks)) {
        // CHeck if we have associated clusters:
        Clusters associatedClusters = track->associatedClusters();
        if(associatedClusters.empty()) {
            LOG(TRACE) << "No associated clusters, skipping track.";
            continue;
        }
        // Drop events with more than one cluster associated
        else if(associatedClusters.size() > 1) {
            LOG(DEBUG) << "More than one associated cluster, dropping track.";
            continue;
        }

        LOG(DEBUG) << "Found track with associated cluster";

        // Get track intercept with DUT in global coordinates
        trackIntercept = DUT->getIntercept(track);

        // Calculate the intercept in local coordinates
        trackInterceptLocal = *(DUT->globalToLocal()) * trackIntercept;
        v_intercepts.push_back(trackInterceptLocal);

        Cluster* cluster = associatedClusters.front();

        // x size
        LOG(DEBUG) << "Gets column width = " << cluster->columnWidth();
        v_clusterSizeX.push_back(cluster->columnWidth());

        // y size
        LOG(DEBUG) << "Gets row width = " << cluster->rowWidth();
        v_clusterSizeY.push_back(cluster->rowWidth());

        // eventID
        v_clusterEventID.push_back(eventID);
        LOG(DEBUG) << "Gets cluster eventID = " << eventID;

        // Get the pixels in the current cluster
        Pixels* pixels = cluster->pixels();

        // Iterate through all pixels in the cluster
        numPixels = 0;
        for(auto& pixel : (*pixels)) {
            // Increase counter for number of pixels in the cluster
            numPixels++;

            // x position
            LOG(DEBUG) << "Gets pixel column = " << pixel->m_column;
            v_pixelX.push_back(pixel->m_column);

            // y position
            LOG(DEBUG) << "Gets pixel row = " << pixel->m_row;
            v_pixelY.push_back(pixel->m_row);

            // ToT
            LOG(DEBUG) << "Gets pixel tot = " << pixel->m_adc;
            v_pixelToT.push_back(pixel->m_adc);

            // ToA
            LOG(DEBUG) << "Gets pixel timestamp = " << pixel->m_timestamp;
            v_pixelToA.push_back(pixel->m_timestamp);
        }
        v_clusterNumPixels.push_back(numPixels);
    }

    if(v_intercepts.empty()) {
        return NoData;
    }

    filledEvents++;
    if(filledEvents % 100 == 0) {
        LOG(DEBUG) << "Events with single associated cluster: " << filledEvents;
    }
    // Fill the tree with the information for this event
    m_outputTree->Fill();

    // Return value telling analysis to keep running
    return Success;
}

void DataOutput::finalise() {
    LOG(DEBUG) << "Finalise";
    // DUT angles for output
    auto DUT = get_detector(m_config.get<std::string>("DUT"));
    auto directory = m_outputFile->mkdir("Directory");
    directory->cd();

    auto orientation = DUT->rotation();
    directory->WriteObject(&orientation, "DUTorientation");

    // Writing out outputfile
    m_outputFile->Write();
    delete(m_outputFile);
}
