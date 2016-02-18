#include "FileWriter.h"

FileWriter::FileWriter(bool debugging)
: Algorithm("FileWriter"){
  debug = debugging;
  m_writePixels = true;
  m_writeClusters = false;
  m_writeTracks = false;
  m_fileName = "outputTuples.root";
}

/*
 
 This algorithm writes an output file and fills it with trees containing
 the requested data. 
 
 Any object which inherits from TestBeamObject can in principle be written
 to file. In order to enable this for a new type, the TestBeamObject::Factory
 function must know how to return an instantiation of that type (see 
 TestBeamObject.C file to see how to do this). The new type can then simply
 be added to the object list and will be written out correctly.
 
 */


void FileWriter::initialise(Parameters* par){
 
  // Pick up the global parameters
  parameters = par;

  // Decide what objects will be written out
  if(m_writePixels) m_objectList.push_back("pixels");
  if(m_writeClusters) m_objectList.push_back("clusters");
  if(m_writeTracks) m_objectList.push_back("tracks");
  
  // Create output file and directories
  m_outputFile = new TFile(m_fileName.c_str(),"RECREATE");
  m_outputFile->cd();
  
  // Loop over all objects to be written to file, and set up the trees
  for(unsigned int itList=0;itList<m_objectList.size();itList++){
    
    // Check the type of object
    string objectType = m_objectList[itList];
    
    // Make a directory in the ouput folder
    m_outputFile->mkdir(objectType.c_str());
    m_outputFile->cd(objectType.c_str());
    
    // Section to set up object writing per detector (such as pixels, clusters)
    if(objectType == "pixels" || objectType == "clusters"){
      
      // Loop over all detectors and make trees for data
      for(int det = 0; det<parameters->nDetectors; det++){
        
        // Get the detector ID and type
        string detectorID = parameters->detectors[det];
        string detectorType = parameters->detector[detectorID]->type();
        
        // Create the tree
        string objectID = detectorID + "_" + objectType;
        string treeName = detectorID + "_" + detectorType + "_" + objectType;
        m_outputTrees[objectID] = new TTree(treeName.c_str(),treeName.c_str());
        m_outputTrees[objectID]->Branch("time", &m_time);
        
        // Cast the TestBeamObject as a specific type using a Factory
        // This will return a Timepix1Pixel*, Timepix3Pixel* etc.
        m_objects[objectID] = TestBeamObject::Factory(detectorType, objectType);
        m_outputTrees[objectID]->Branch(objectType.c_str(), &m_objects[objectID]);
        
      }
    }
    // If not an object to be written per detector
    else{
      // Make the tree
      string treeName = objectType;
      m_outputTrees[objectType] = new TTree(treeName.c_str(),treeName.c_str());
      // Branch the tree to the timestamp and object
      m_outputTrees[objectType]->Branch("time", &m_time);
      m_objects[objectType] = TestBeamObject::Factory(objectType);
      m_outputTrees[objectType]->Branch(objectType.c_str(), &m_objects[objectType]);
    }
  }
  
  // Initialise member variables
  m_eventNumber = 0;
}

StatusCode FileWriter::run(Clipboard* clipboard){

  // Loop over all objects to be written to file, and get the objects currently
  // held on the Clipboard
  for(unsigned int itList=0;itList<m_objectList.size();itList++){
    
    // Check the type of object
    string objectType = m_objectList[itList];

    // If this is written per device, loop over all devices
    if(objectType == "pixels" || objectType == "clusters"){

      // Loop over all detectors
      for(int det = 0; det<parameters->nDetectors; det++){
        
        // Get the detector and object ID
        string detectorID = parameters->detectors[det];
        string objectID = detectorID + "_" + objectType;
        
        // Get the objects, if they don't exist then continue
        if(debug) tcout<<"Checking for "<<objectType<<" on device "<<detectorID<<endl;
        TestBeamObjects* objects = clipboard->get(detectorID,objectType);
        if(objects == NULL) continue;
        if(debug) tcout<<"Picked up "<<objects->size()<<" "<<objectType<<" from device "<<detectorID<<endl;
        
        // Check if the output tree exists
        if(!m_outputTrees[objectID]) continue;
        
        // Fill the objects into the tree
        for(int itObject=0;itObject<objects->size();itObject++){
          m_objects[objectID] = (*objects)[itObject];
          m_time = m_objects[objectID]->timestamp();
          m_outputTrees[objectID]->Fill();
        }
      }
    } // If object is not written per device
    else{
      
      // Get the objects, if they don't exist then continue
      if(debug) tcout<<"Checking for "<<objectType<<endl;
      TestBeamObjects* objects = clipboard->get(objectType);
      if(objects == NULL) continue;
      if(debug) tcout<<"Picked up "<<objects->size()<<" "<<objectType<<endl;
      
      // Check if the output tree exists
      if(!m_outputTrees[objectType]) continue;
      
      // Fill the objects into the tree
      for(int itObject=0;itObject<objects->size();itObject++){
        m_objects[objectType] = (*objects)[itObject];
        m_time = m_objects[objectType]->timestamp();
        m_outputTrees[objectType]->Fill();
      }
      if(debug) tcout<<"Written "<<objectType<<" to file"<<endl;

    }
  }

   // Increment event counter
  m_eventNumber++;
  
  // Return value telling analysis to keep running
  return Success;
}

void FileWriter::finalise(){
  
  // Write the trees to file
  // Loop over all objects to be written to file, and get the objects currently
  // held on the Clipboard
  for(unsigned int itList=0;itList<m_objectList.size();itList++){
    
    // Check the type of object
    string objectType = m_objectList[itList];
    
    // If this is written per device, loop over all devices
    if(objectType == "pixels" || objectType == "clusters"){
      
      // Loop over all detectors
      for(int det = 0; det<parameters->nDetectors; det++){
        
        // Get the detector and object ID
        string detectorID = parameters->detectors[det];
        string objectID = detectorID + "_" + objectType;
        
        // If there is no output tree then do nothing
        if(!m_outputTrees[objectID]) continue;
        
        // Move to the write output file
        m_outputFile->cd();
        m_outputFile->cd(objectType.c_str());
        m_outputTrees[objectID]->Write();
        
        // Clean up the tree and remove object pointer
        delete m_outputTrees[objectID];
        m_objects[objectID] = 0;
        
      }
    }// Write trees for devices which are not detector dependent
    else{
      
      // If there is no output tree then do nothing
      if(!m_outputTrees[objectType]) continue;
      
      // Move to the write output file
      m_outputFile->cd();
      m_outputFile->cd(objectType.c_str());
      m_outputTrees[objectType]->Write();
      
      // Clean up the tree and remove object pointer
      delete m_outputTrees[objectType];
      m_objects[objectType] = 0;

    }
  }
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
