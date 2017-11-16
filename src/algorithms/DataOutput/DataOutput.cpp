#include "DataOutput.h"
#include <vector>

using namespace corryvreckan;
using namespace std;

DataOutput::DataOutput(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {

    m_fileName = m_config.get<std::string>("fileName", "outputTuples.root");
    m_useToA = m_config.get<bool>("useToA", true);
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

 Any object which inherits from TestBeamObject can in principle be written
 to file. In order to enable this for a new type, the TestBeamObject::Factory
 function must know how to return an instantiation of that type (see
 TestBeamObject.C file to see how to do this). The new type can then simply
 be added to the object list and will be written out correctly.

 */

void DataOutput::initialise() {
    LOG(DEBUG) << "Initialised DataOutput";

    // Decide what objects will be written out
    m_objectList.push_back("clusters");
    m_objectList.push_back("tracks");

    LOG(DEBUG) << "Pushed back list of objects";

    // Create output file and directories
    m_outputFile = new TFile(m_fileName.c_str(), "RECREATE");
    LOG(DEBUG) << "Made and moved to output file: " << m_fileName;
    gDirectory->Delete("tree;*");
    m_outputTree = new TTree(m_treeName.c_str(), m_treeName.c_str());
    LOG(DEBUG) << "Created tree: " << m_treeName;

    // Loop over all objects to be written to file, and set up the trees
    eventID = 0;
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // Section to set up cluster writing per detector
        if(objectType == "clusters") {

            // Loop over all detectors
            for(auto& detector : get_detectors()) {

                // Get the detector ID and type
                string detectorID = detector->name();
                string detectorType = detector->type();

                // Writing information for the DUT only
                if(detectorID != m_config.get<std::string>("DUT"))
                    continue;

                // Create the output branches
                m_outputTree->Branch("EventID", &v_clusterEventID);
                m_outputTree->Branch("clusterSizeX", &v_clusterSizeX);
                m_outputTree->Branch("clusterSizeY", &v_clusterSizeY);
                m_outputTree->Branch("pixelX", &v_pixelX);
                m_outputTree->Branch("pixelY", &v_pixelY);
                m_outputTree->Branch("pixelToT", &v_pixelToT);
                m_outputTree->Branch("pixelToA", &v_pixelToA);
                m_outputTree->Branch("clusterNumPixels", &v_clusterNumPixels);
            }
        }

        else {
            // Branch for track intercepts
            m_outputTree->Branch("intercepts", &v_intercepts);
        }
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode DataOutput::run(Clipboard* clipboard) {
    // Counter for cluster event ID
    eventID++;

    // Loop over all objects to be written to file, and get the objects
    // currently held on the Clipboard
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // Loop over devices for cluster information
        if(objectType == "clusters") {

            // Loop over all detectors
            for(auto& detector : get_detectors()) {

                // Get the detector ID
                string detectorID = detector->name();

                // Only writing information for the DUT
                if(detectorID != m_config.get<std::string>("DUT"))
                    continue;

                // Get the clusters, if they don't exist then continue
                LOG(DEBUG) << "Checking for " << objectType << " on device " << detectorID;

                // Get the clusters in this event
                Clusters* clusters = (Clusters*)clipboard->get(detectorID, "clusters");
                if(clusters == NULL) {
                    LOG(DEBUG) << "No clusters on the clipboard";
                    return Success;
                } else {
                    LOG(DEBUG) << "Found clusters on the clipboard";
                }

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
                tmp_int = 0;
                tmp_double = 0.0;

                // Iterate through all clusters in this event
                Clusters::iterator itCluster;
                for(itCluster = clusters->begin(); itCluster != clusters->end(); itCluster++) {
                    // Reset number of pixels in the cluster counter
                    numPixels = 0;

                    // Get the clusters in this event
                    Cluster* cluster = (*itCluster);

                    // x size
                    tmp_double = cluster->columnWidth();
                    LOG(DEBUG) << "Gets column width = " << tmp_double;
                    v_clusterSizeX.push_back(tmp_double);

                    // y size
                    tmp_double = cluster->rowWidth();
                    LOG(DEBUG) << "Gets row width = " << tmp_double;
                    v_clusterSizeY.push_back(tmp_double);

                    // eventID
                    v_clusterEventID.push_back(eventID);
                    LOG(DEBUG) << "Gets cluster eventID = " << eventID;

                    // Get the pixels in the current cluster
                    Pixels* pixels = cluster->pixels();

                    // Iterate through all pixels in the cluster
                    Pixels::iterator itPixel;
                    for(itPixel = pixels->begin(); itPixel != pixels->end(); itPixel++) {
                        // Increase counter for number of pixels in the cluster
                        numPixels++;

                        // Get the pixels
                        Pixel* pixel = (*itPixel);

                        // x position
                        tmp_int = pixel->m_column;
                        LOG(DEBUG) << "Gets pixel column = " << tmp_int;
                        v_pixelX.push_back(tmp_int);

                        // y position
                        tmp_int = pixel->m_row;
                        LOG(DEBUG) << "Gets pixel row = " << tmp_int;
                        v_pixelY.push_back(tmp_int);

                        // ToT
                        tmp_int = pixel->m_adc;
                        LOG(DEBUG) << "Gets pixel tot = " << tmp_int;
                        v_pixelToT.push_back(tmp_int);

                        // ToA
                        tmp_longint = pixel->m_timestamp;
                        LOG(DEBUG) << "Gets pixel timestamp = " << tmp_longint;
                        v_pixelToA.push_back(tmp_longint);
                    }
                    v_clusterNumPixels.push_back(numPixels);
                }
                // Fill the tree with the information for this event
                m_outputTree->Fill();
            }
        }
        // For track intercepts
        else {
            v_intercepts.clear();
            // collecting detectors
            for(auto& detector : get_detectors()) {
                string detectorID = detector->name();
                LOG(DEBUG) << "Detector name = " << detectorID;
                // Only writing information for the DUT
                if(detectorID != m_config.get<std::string>("DUT"))
                    continue;

                // Getting tracks from the clipboard
                Tracks* tracks = (Tracks*)clipboard->get("tracks");
                if(tracks == NULL) {
                    LOG(DEBUG) << "No tracks on the clipboard";
                    return Success;
                }
                LOG(DEBUG) << "Found tracks";

                // Iterate through tracks found
                Tracks::iterator itTrack;
                for(itTrack = tracks->begin(); itTrack != tracks->end(); itTrack++) {
                    // Get the track
                    Track* track = (*itTrack);
                    if(!track) {
                        continue;
                    }

                    // Get track intercept with DUT in global coordinates
                    trackIntercept = detector->getIntercept(track);

                    // Calculate the intercept in local coordinates
                    trackInterceptLocal = *(detector->globalToLocal()) * trackIntercept;

                    v_intercepts.push_back(trackInterceptLocal);
                }
                m_outputTree->Fill();
            }
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void DataOutput::finalise() {
    LOG(DEBUG) << "Finalise";

    // Write out the output file
    m_outputFile->Write();
    delete(m_outputFile);
}
