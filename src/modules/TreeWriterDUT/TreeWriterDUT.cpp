#include "TreeWriterDUT.h"
#include "core/utils/file.h"

#include <vector>

using namespace corryvreckan;
using namespace std;

TreeWriterDUT::TreeWriterDUT(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), m_detector(detector) {

    m_fileName = m_config.get<std::string>("file_name", "outputTuples.root");
    m_treeName = m_config.get<std::string>("tree_name", "tree");
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

void TreeWriterDUT::initialise() {
    LOG(DEBUG) << "Initialised TreeWriterDUT";

    // Create output file and directories
    auto path = createOutputFile(add_file_extension(m_fileName, "root"));
    m_outputFile = new TFile(path.c_str(), "RECREATE");
    LOG(DEBUG) << "Made and moved to output file: " << path;
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

StatusCode TreeWriterDUT::run(std::shared_ptr<Clipboard> clipboard) {
    // Counter for cluster event ID
    eventID++;

    // Clear data vectors before storing the cluster information for this event
    v_intercepts.clear();
    v_clusterSizeX.clear();
    v_clusterSizeY.clear();
    v_clusterEventID.clear();
    v_pixelX.clear();
    v_pixelY.clear();
    v_pixelToT.clear();
    v_pixelToA.clear();
    v_clusterNumPixels.clear();

    // Getting tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    if(tracks == nullptr) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return StatusCode::Success;
    }

    // Iterate through tracks found
    for(auto& track : (*tracks)) {
        // CHeck if we have associated clusters:
        auto associatedClusters = track->associatedClusters();
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
        trackIntercept = m_detector->getIntercept(track);

        // Calculate the intercept in local coordinates
        trackInterceptLocal = m_detector->globalToLocal(trackIntercept);
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
        auto pixels = cluster->pixels();

        // Iterate through all pixels in the cluster
        numPixels = 0;
        for(auto& pixel : pixels) {
            // Increase counter for number of pixels in the cluster
            numPixels++;

            // x position
            LOG(DEBUG) << "Gets pixel column = " << pixel->column();
            v_pixelX.push_back(pixel->column());

            // y position
            LOG(DEBUG) << "Gets pixel row = " << pixel->row();
            v_pixelY.push_back(pixel->row());

            // ToT
            LOG(DEBUG) << "Gets pixel raw value = " << pixel->raw();
            v_pixelToT.push_back(pixel->raw());

            // ToA
            LOG(DEBUG) << "Gets pixel timestamp = " << pixel->timestamp();
            v_pixelToA.push_back(pixel->timestamp());
        }
        v_clusterNumPixels.push_back(numPixels);
    }

    if(v_intercepts.empty()) {
        return StatusCode::NoData;
    }

    filledEvents++;
    if(filledEvents % 100 == 0) {
        LOG(DEBUG) << "Events with single associated cluster: " << filledEvents;
    }
    // Fill the tree with the information for this event
    m_outputTree->Fill();

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void TreeWriterDUT::finalise() {
    LOG(DEBUG) << "Finalise";
    auto directory = m_outputFile->mkdir("Directory");
    directory->cd();
    LOG(STATUS) << filledEvents << " events written to file " << m_fileName;

    auto orientation = m_detector->rotation();
    directory->WriteObject(&orientation, "DUTorientation");

    // Writing out outputfile
    m_outputFile->Write();
    delete(m_outputFile);
}
