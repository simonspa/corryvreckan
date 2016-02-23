#include "FileReader.h"
#include "Timepix3Pixel.h"
#include "Timepix3Cluster.h"
#include "Timepix3Track.h"

FileReader::FileReader(bool debugging)
: Algorithm("FileReader"){
  debug = debugging;
  m_onlyDUT = true;
  m_readPixels = true;
  m_readClusters = false;
  m_readTracks = true;
  m_fileName = "outputTuples.root";
  m_timeWindow = 1.;
  m_currentTime = 0.;
}

/*
 
 This algorithm reads an input file containing trees with data previously
 written out by the FileWriter.
 
 Any object which inherits from TestBeamObject can in principle be read
 from file. In order to enable this for a new type, the TestBeamObject::Factory
 function must know how to return an instantiation of that type (see
 TestBeamObject.C file to see how to do this). The new type can then simply
 be added to the object list and will be read in correctly. This is the same
 as for the FileWriter, so if the data has been written then the reading will
 run without problems.
 
 */

void FileReader::initialise(Parameters* par){
 
  // Pick up the global parameters
  parameters = par;
  m_fileName = parameters->inputTupleFile;

  // Decide what objects will be read in
  if(m_readPixels) m_objectList.push_back("pixels");
  if(m_readClusters) m_objectList.push_back("clusters");
  if(m_readTracks) m_objectList.push_back("tracks");
  
  // Get input file
  tcout<<"Opening file "<<m_fileName<<endl;
  m_inputFile = new TFile(m_fileName.c_str(),"READ");
  m_inputFile->cd();
  
  // Loop over all objects to be read from file, and get the trees
  for(unsigned int itList=0;itList<m_objectList.size();itList++){
    
    // Check the type of object
    string objectType = m_objectList[itList];
    
    // Section to set up object reading per detector (such as pixels, clusters)
    if(objectType == "pixels" || objectType == "clusters"){
      
      // Loop over all detectors and search for data
      for(int det = 0; det<parameters->nDetectors; det++){
        
        // Get the detector ID and type
        string detectorID = parameters->detectors[det];
        string detectorType = parameters->detector[detectorID]->type();
        
        // If only reading information for the DUT
        if(m_onlyDUT && detectorID != parameters->DUT) continue;

        if(debug) tcout<<"Looking for "<<objectType<<" for device "<<detectorID<<endl;

        // Get the tree
        string objectID = detectorID + "_" + objectType;
        string treePath = objectType + "/" + detectorID + "_" + detectorType + "_" + objectType;
        m_inputTrees[objectID] = (TTree*)gDirectory->Get(treePath.c_str());
        
        // Set the branch addresses
        m_inputTrees[objectID]->SetBranchAddress("time", &m_time);
        
        // Cast the TestBeamObject as a specific type using a Factory
        // This will return a Timepix1Pixel*, Timepix3Pixel* etc.
        m_objects[objectID] = TestBeamObject::Factory(detectorType, objectType);
        m_inputTrees[objectID]->SetBranchAddress(objectType.c_str(), &m_objects[objectID]);
        m_currentPosition[objectID] = 0;
        
      }
    }
    // If not an object to be written per pixel
    else{
      // Make the tree
      string treePath = objectType + "/" + objectType;
      m_inputTrees[objectType] = (TTree*)gDirectory->Get(treePath.c_str());
      // Branch the tree to the timestamp and object
      m_inputTrees[objectType]->SetBranchAddress("time", &m_time);
      m_objects[objectType] = TestBeamObject::Factory(objectType);
      m_inputTrees[objectType]->SetBranchAddress(objectType.c_str(), &m_objects[objectType]);
    }
  }
  
  // Initialise member variables
  m_eventNumber = 0;
}

StatusCode FileReader::run(Clipboard* clipboard){
  
  cout<<"\rRunning over event "<<m_eventNumber<<flush;
  
  bool newEvent = true;
  // Loop over all objects read from file, and place the objects on the Clipboard
  bool dataLoaded = false;
	for(unsigned int itList=0;itList<m_objectList.size();itList++){
    
    // Check the type of object
    string objectType = m_objectList[itList];
    
    // If this is written per device, loop over all devices
    if(objectType == "pixels" || objectType == "clusters"){
      
      // Loop over all detectors
      for(int det = 0; det<parameters->nDetectors; det++){
        
        // Get the detector and object ID
        string detectorID = parameters->detectors[det];
        string detectorType = parameters->detector[detectorID]->type();
        string objectID = detectorID + "_" + objectType;
        
        // If only writing information for the DUT
        if(m_onlyDUT && detectorID != parameters->DUT) continue;

        // If there is no data for this device, continue
        if(!m_inputTrees[objectID]) continue;
        
        // Create the container that will go on the clipboard
        TestBeamObjects* objectContainer = new TestBeamObjects();
        if(debug) tcout<<"Looking for "<<objectType<<" on detector "<<detectorID<<endl;
        
        // Continue looping over this device while there is still data
        while(m_currentPosition[objectID]<m_inputTrees[objectID]->GetEntries()){
          
          // Get the new event from the tree
          m_inputTrees[objectID]->GetEvent(m_currentPosition[objectID]);
          
          // If the event is outwith the current time window, stop loading data
          if( (m_time - m_currentTime) > m_timeWindow ){
            if(newEvent){
              m_currentTime = m_time;
              newEvent = false;
            }else{
              break;
            }
          }
          dataLoaded = true;
          m_currentPosition[objectID]++;
          
          // Make a copy of the object from the tree, and place it in the object container
          TestBeamObject* object = TestBeamObject::Factory(detectorType, objectType, m_objects[objectID]);
          objectContainer->push_back(object);
          
        }
        
        // Put the data on the clipboard
        clipboard->put(detectorID,objectType,objectContainer);
        if(debug) tcout<<"Picked up "<<objectContainer->size()<<" "<<objectType<<" from device "<<detectorID<<endl;
        
      }
    } // If object is not written per device
    else{
     
      // If there is no data for this device, continue
      if(!m_inputTrees[objectType]) continue;
      
      // Create the container that will go on the clipboard
      TestBeamObjects* objectContainer = new TestBeamObjects();
      if(debug) tcout<<"Looking for "<<objectType<<endl;
      
      // Continue looping over this device while there is still data
      while(m_currentPosition[objectType]<m_inputTrees[objectType]->GetEntries()){
        
        // Get the new event from the tree
        m_inputTrees[objectType]->GetEvent(m_currentPosition[objectType]);
        
        // If the event is outwith the current time window, stop loading data
        if( (m_time - m_currentTime) > m_timeWindow ){
          if(newEvent){
            m_currentTime = m_time;
            newEvent = false;
          }else{
            break;
          }
        }
        dataLoaded = true;
        m_currentPosition[objectType]++;
        
        // Make a copy of the object from the tree, and place it in the object container
        TestBeamObject* object = TestBeamObject::Factory(objectType, m_objects[objectType]);
        objectContainer->push_back(object);
        
      }
      
      // Put the data on the clipboard
      clipboard->put(objectType,objectContainer);
      if(debug) tcout<<"Picked up "<<objectContainer->size()<<" "<<objectType<<endl;

    }
  }

  // If no data was loaded then do nothing
  if(!dataLoaded && m_eventNumber!=0){
    cout<<endl;
    return Failure;
  }
  
  // Increment event counter
  m_eventNumber++;
  
  // Return value telling analysis to keep running
  return Success;

}

void FileReader::finalise(){
  
  // Close the input file
  m_inputFile->Close();
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
