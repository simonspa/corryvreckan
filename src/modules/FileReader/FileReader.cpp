#include "FileReader.h"

using namespace corryvreckan;
using namespace std;

FileReader::FileReader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_readPixels = m_config.get<bool>("readPixels", true);
    m_readClusters = m_config.get<bool>("readClusters", false);
    m_readTracks = m_config.get<bool>("readTracks", false);
    m_fileName = m_config.get<std::string>("fileName", "outputTuples.root");
    m_timeWindow = m_config.get<double>("timeWindow", static_cast<double>(Units::convert(1., "s")));
    m_readMCParticles = m_config.get<bool>("readMCParticles", false);
    // checking if DUT parameter is in the configuration file, if so then check if should only output the DUT
    m_onlyDUT = m_config.get<bool>("onlyDUT", false);
    m_currentTime = 0.;
}

/*

 This algorithm reads an input file containing trees with data previously
 written out by the FileWriter.

 Any object which inherits from Object can in principle be read
 from file. In order to enable this for a new type, the Object::Factory
 function must know how to return an instantiation of that type (see
 Object.C file to see how to do this). The new type can then simply
 be added to the object list and will be read in correctly. This is the same
 as for the FileWriter, so if the data has been written then the reading will
 run without problems.

 */

void FileReader::initialise() {
    // Decide what objects will be read in
    if(m_readPixels)
        m_objectList.push_back("pixels");
    if(m_readClusters)
        m_objectList.push_back("clusters");
    if(m_readTracks)
        m_objectList.push_back("tracks");
    if(m_readMCParticles)
        m_objectList.push_back("mcparticles");

    // Get input file
    LOG(INFO) << "Opening file " << m_fileName;
    m_inputFile = new TFile(m_fileName.c_str(), "READ");
    if(!m_inputFile->IsOpen()) {
        throw ModuleError("Cannot open input file \"" + m_fileName + "\".");
    }
    m_inputFile->cd();

    // Loop over all objects to be read from file, and get the trees
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // Section to set up object reading per detector (such as pixels, clusters)
        if(objectType == "pixels" || objectType == "clusters" || objectType == "mcparticles") {

            // Loop over all detectors and search for data
            for(auto& detector : get_detectors()) {

                // Get the detector ID and type
                string detectorID = detector->name();
                string detectorType = detector->type();

                // If only reading information for the DUT
                if(m_onlyDUT && !detector->isDUT()) {
                    continue;
                }

                // Get the tree
                string objectID = detectorID + "_" + objectType;
                string treePath = objectType + "/" + detectorID + "_" + detectorType + "_" + objectType;
                LOG(DEBUG) << "Looking for " << objectType << " for device " << detectorID << ", tree path " << treePath;

                m_inputTrees[objectID] = static_cast<TTree*>(gDirectory->Get(treePath.c_str()));
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
                // Set the branch addresses
                m_inputTrees[objectID]->SetBranchAddress("time", &m_time);

                // Cast the Object as a specific type using a Factory
                m_objects[objectID] = Object::Factory(detectorType, objectType);
                m_inputTrees[objectID]->SetBranchAddress(objectType.c_str(), &m_objects[objectID]);
#pragma GCC diagnostic pop
                m_currentPosition[objectID] = 0;
            }
        }
        // If not an object to be written per pixel
        else {
            // Make the tree
            string treePath = objectType + "/" + objectType;
            m_inputTrees[objectType] = static_cast<TTree*>(gDirectory->Get(treePath.c_str()));
// Branch the tree to the timestamp and object
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
            m_inputTrees[objectType]->SetBranchAddress("time", &m_time);
            m_objects[objectType] = Object::Factory(objectType);
            m_inputTrees[objectType]->SetBranchAddress(objectType.c_str(), &m_objects[objectType]);
#pragma GCC diagnostic pop
        }
    }

    LOG(STATUS) << "Successfully opened data file \"" << m_fileName << "\"";
    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode FileReader::run(std::shared_ptr<Clipboard> clipboard) {

    LOG_PROGRESS(INFO, "file_reader") << "Running over event " << m_eventNumber;

    bool newEvent = true;
    // Loop over all objects read from file, and place the objects on the
    // Clipboard
    bool dataLoaded = false;
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // If this is written per device, loop over all devices
        if(objectType == "pixels" || objectType == "clusters" || objectType == "mcparticles") {

            // Loop over all detectors
            for(auto& detector : get_detectors()) {

                // Get the detector and object ID
                string detectorID = detector->name();
                string detectorType = detector->type();
                string objectID = detectorID + "_" + objectType;

                // If only writing information for the DUT
                if(m_onlyDUT && !detector->isDUT()) {
                    continue;
                }

                // If there is no data for this device, continue
                if(!m_inputTrees[objectID]) {
                    continue;
                }

                // Create the container that will go on the clipboard
                Objects* objectContainer = new Objects();
                LOG(DEBUG) << "Looking for " << objectType << " on detector " << detectorID;

                // Continue looping over this device while there is still data
                while(m_currentPosition[objectID] < m_inputTrees[objectID]->GetEntries()) {

                    // Get the new event from the tree
                    m_inputTrees[objectID]->GetEvent(m_currentPosition[objectID]);

                    // If the event is outwith the current time window, stop loading data
                    if((m_time - m_currentTime) > m_timeWindow) {
                        if(newEvent) {
                            m_currentTime = m_time;
                            newEvent = false;
                        } else {
                            break;
                        }
                    }
                    dataLoaded = true;
                    m_currentPosition[objectID]++;

                    // Make a copy of the object from the tree, and place it in the object
                    // container
                    Object* object = Object::Factory(detectorType, objectType, m_objects[objectID]);
                    objectContainer->push_back(object);
                }

                // Put the data on the clipboard
                clipboard->put(detectorID, objectType, objectContainer);
                LOG(DEBUG) << "Picked up " << objectContainer->size() << " " << objectType << " from device " << detectorID;
            }
        } // If object is not written per device
        else {

            // If there is no data for this device, continue
            if(!m_inputTrees[objectType])
                continue;

            // Create the container that will go on the clipboard
            Objects* objectContainer = new Objects();
            LOG(DEBUG) << "Looking for " << objectType;

            // Continue looping over this device while there is still data
            while(m_currentPosition[objectType] < m_inputTrees[objectType]->GetEntries()) {

                // Get the new event from the tree
                m_inputTrees[objectType]->GetEvent(m_currentPosition[objectType]);

                // If the event is outwith the current time window, stop loading data
                if((m_time - m_currentTime) > m_timeWindow) {
                    if(newEvent) {
                        m_currentTime = m_time;
                        newEvent = false;
                    } else {
                        break;
                    }
                }
                dataLoaded = true;
                m_currentPosition[objectType]++;

                // Make a copy of the object from the tree, and place it in the object
                // container
                Object* object = Object::Factory(objectType, m_objects[objectType]);
                objectContainer->push_back(object);
            }

            // Put the data on the clipboard
            clipboard->put(objectType, objectContainer);
            LOG(DEBUG) << "Picked up " << objectContainer->size() << " " << objectType;
        }
    }

    // If no data was loaded then do nothing
    if(!dataLoaded && m_eventNumber != 0) {
        return Failure;
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return Success;
}

void FileReader::finalise() {

    // Close the input file
    m_inputFile->Close();

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
