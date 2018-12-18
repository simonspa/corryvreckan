#include "FileWriter.h"
#include "core/utils/file.h"

using namespace corryvreckan;
using namespace std;

FileWriter::FileWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {

    m_onlyDUT = m_config.get<bool>("onlyDUT", true);
    m_writePixels = m_config.get<bool>("writePixels", true);
    m_writeClusters = m_config.get<bool>("writeClusters", false);
    m_writeTracks = m_config.get<bool>("writeTracks", true);
    m_fileName = m_config.get<std::string>("fileName", "outputTuples.root");
}

/*

 This algorithm writes an output file and fills it with trees containing
 the requested data.

 Any object which inherits from Object can in principle be written
 to file. In order to enable this for a new type, the Object::Factory
 function must know how to return an instantiation of that type (see
 Object.C file to see how to do this). The new type can then simply
 be added to the object list and will be written out correctly.

 */

void FileWriter::initialise() {

    // Decide what objects will be written out
    if(m_writePixels)
        m_objectList.push_back("pixels");
    if(m_writeClusters)
        m_objectList.push_back("clusters");
    if(m_writeTracks)
        m_objectList.push_back("tracks");

    // Create output file and directories
    auto path = createOutputFile(add_file_extension(m_fileName, "root"), true);
    m_outputFile = new TFile(path.c_str(), "RECREATE");
    m_outputFile->cd();

    // Loop over all objects to be written to file, and set up the trees
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // Make a directory in the ouput folder
        m_outputFile->mkdir(objectType.c_str());
        m_outputFile->cd(objectType.c_str());

        // Section to set up object writing per detector (such as pixels, clusters)
        if(objectType == "pixels" || objectType == "clusters") {

            // Loop over all detectors and make trees for data
            for(auto& detector : get_detectors()) {

                // Get the detector ID and type
                string detectorID = detector->name();
                string detectorType = detector->type();

                // If only writing information for the DUT
                if(m_onlyDUT && !detector->isDUT())
                    continue;

                // Create the tree
                string objectID = detectorID + "_" + objectType;
                string treeName = detectorID + "_" + detectorType + "_" + objectType;
                m_outputTrees[objectID] = new TTree(treeName.c_str(), treeName.c_str());
                m_outputTrees[objectID]->Branch("time", &m_time);

                // Cast the Object as a specific type using a Factory
                // This will return a Timepix1Pixel*, Timepix3Pixel* etc.
                m_objects[objectID] = Object::Factory(detectorType, objectType);
                m_outputTrees[objectID]->Branch(objectType.c_str(), &m_objects[objectID]);
            }
        }
        // If not an object to be written per detector
        else {
            // Make the tree
            string treeName = objectType;
            m_outputTrees[objectType] = new TTree(treeName.c_str(), treeName.c_str());
            // Branch the tree to the timestamp and object
            m_outputTrees[objectType]->Branch("time", &m_time);
            m_objects[objectType] = Object::Factory(objectType);
            m_outputTrees[objectType]->Branch(objectType.c_str(), &m_objects[objectType]);
        }
    }

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode FileWriter::run(std::shared_ptr<Clipboard> clipboard) {

    // Loop over all objects to be written to file, and get the objects currently
    // held on the Clipboard
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // If this is written per device, loop over all devices
        if(objectType == "pixels" || objectType == "clusters") {

            // Loop over all detectors
            for(auto& detector : get_detectors()) {

                // Get the detector and object ID
                string detectorID = detector->name();
                string objectID = detectorID + "_" + objectType;

                // If only writing information for the DUT
                if(m_onlyDUT && !detector->isDUT())
                    continue;

                // Get the objects, if they don't exist then continue
                LOG(DEBUG) << "Checking for " << objectType << " on device " << detectorID;
                Objects* objects = clipboard->get(detectorID, objectType);
                if(objects == nullptr)
                    continue;
                LOG(DEBUG) << "Picked up " << objects->size() << " " << objectType << " from device " << detectorID;

                // Check if the output tree exists
                if(!m_outputTrees[objectID])
                    continue;

                // Fill the objects into the tree
                for(auto& object : (*objects)) {
                    m_objects[objectID] = object;
                    m_time = m_objects[objectID]->timestamp();
                    m_outputTrees[objectID]->Fill();
                }
            }
        } // If object is not written per device
        else {

            // Get the objects, if they don't exist then continue
            LOG(DEBUG) << "Checking for " << objectType;
            Objects* objects = clipboard->get(objectType);
            if(objects == nullptr)
                continue;
            LOG(DEBUG) << "Picked up " << objects->size() << " " << objectType;

            // Check if the output tree exists
            if(!m_outputTrees[objectType])
                continue;

            // Fill the objects into the tree
            for(auto& object : (*objects)) {
                m_objects[objectType] = object;
                m_time = m_objects[objectType]->timestamp();
                m_outputTrees[objectType]->Fill();
            }
            LOG(DEBUG) << "Written " << objectType << " to file";
        }
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void FileWriter::finalise() {

    // Write the trees to file
    // Loop over all objects to be written to file, and get the objects currently held on the Clipboard
    for(unsigned int itList = 0; itList < m_objectList.size(); itList++) {

        // Check the type of object
        string objectType = m_objectList[itList];

        // If this is written per device, loop over all devices
        if(objectType == "pixels" || objectType == "clusters") {

            // Loop over all detectors
            for(auto& detector : get_detectors()) {

                // Get the detector and object ID
                string detectorID = detector->name();
                string objectID = detectorID + "_" + objectType;

                // If there is no output tree then do nothing
                if(!m_outputTrees[objectID])
                    continue;

                // Move to the write output file
                m_outputFile->cd();
                m_outputFile->cd(objectType.c_str());
                m_outputTrees[objectID]->Write();

                // Clean up the tree and remove object pointer
                delete m_outputTrees[objectID];
                m_objects[objectID] = nullptr;
            }
        } // Write trees for devices which are not detector dependent
        else {

            // If there is no output tree then do nothing
            if(!m_outputTrees[objectType])
                continue;

            // Move to the write output file
            m_outputFile->cd();
            m_outputFile->cd(objectType.c_str());
            m_outputTrees[objectType]->Write();

            // Clean up the tree and remove object pointer
            delete m_outputTrees[objectType];
            m_objects[objectType] = nullptr;
        }
    }

    // Close the output file
    m_outputFile->Close();
    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
