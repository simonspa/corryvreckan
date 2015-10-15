#include "Timepix3EventLoader.h"

#include <dirent.h>
#include <sstream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdint.h>

Timepix3EventLoader::Timepix3EventLoader(bool debugging)
: Algorithm("Timepix3EventLoader"){
  debug = debugging;
  applyTimingCut = false;
  m_currentTime = 0;
}


void Timepix3EventLoader::initialise(Parameters* par){
 
  parameters = par;
  // Take input directory from global parameters
  m_inputDirectory = parameters->inputDirectory;
  
  // File structure is RunX/ChipID/files.dat
  
  // Open the root directory
  DIR* directory = opendir(m_inputDirectory.c_str());
  if (directory == NULL){tcout<<"Directory "<<m_inputDirectory<<" does not exist"<<endl; return;}
  dirent* entry; dirent* file;

  // Read the entries in the folder
  while (entry = readdir(directory)){
    
    // If these are folders then the name is the chip ID
    if (entry->d_type == DT_DIR){
      
      // Open the folder for this device
      string detectorID = entry->d_name;
      string dataDirName = m_inputDirectory+"/"+entry->d_name;
      DIR* dataDir = opendir(dataDirName.c_str());
      string trimdacfile;
      
      // Get all of the files for this chip
      while (file = readdir(dataDir)){
        string filename = dataDirName+"/"+file->d_name;

        // Check if file has extension .dat
        if (string(file->d_name).find(".dat") != string::npos){
          m_datafiles[detectorID].push_back(filename.c_str());
          m_nFiles[detectorID]++;

          // Initialise null values for later
          m_currentFile[detectorID] = NULL;
          m_fileNumber[detectorID] = 0;
        }
        
        // If not a data file, it might be a trimdac file, with the list of masked pixels etc.
        if (string(file->d_name).find("trimdac") != string::npos){
          trimdacfile = filename;
        }
        
      }
      
      // If files were stored, register the detector
      if(m_nFiles.count(detectorID) > 0){
        
        tcout<<"Registering detector "<<detectorID<<endl;
        parameters->registerDetector(detectorID);
        
        // Now that we have all of the data files and mask files for this detector, pass the mask file to parameters
        tcout<<"Set mask file "<<trimdacfile<<endl;
        parameters->detector[detectorID]->setMaskFile(trimdacfile);
        
        // Apply the pixel masking
        maskPixels(detectorID,trimdacfile);
      }

    }
  }
}

int Timepix3EventLoader::run(Clipboard* clipboard){
  
  tcout<<"Current time is "<<parameters->currentTime<<endl;
  bool loadedData = false;
  // Loop through all registered detectors
  for(int det = 0; det<parameters->nDetectors; det++){
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    // Make a new container for the data
    Timepix3Pixels* deviceData = new Timepix3Pixels();
		// Load the next chunk of data
    tcout<<"Loading data from "<<detectorID<<endl;
    bool data = loadData(detectorID,deviceData);
    // If data was loaded then put it on the clipboard
    if(data){
      loadedData = true;
      tcout<<"Loaded "<<deviceData->size()<<" pixels for device "<<detectorID<<endl;
      clipboard->put(detectorID,"pixels",(TestBeamObjects*)deviceData);
    }
  }
  
  // Increment the event time
  parameters->currentTime += parameters->eventLength;
  
  // If no data was loaded, tell the event loop to stop
  if(!loadedData) return 2;
  
  // Otherwise tell event loop to keep running
  return 1;
}

// Function to load the pixel mask file
void Timepix3EventLoader::maskPixels(string detectorID, string trimdacfile){
  
  // Open the mask file
  ifstream trimdacs;
  trimdacs.open(trimdacfile.c_str());
  
  // Ignore the file header
  string line;
  getline(trimdacs,line);
  int t_col, t_row, t_trim, t_mask, t_tpen;

  // Loop through the pixels in the file and apply the mask
  for(int col=0;col<256;col++){
    for(int row=0;row<256;row++){
      trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
      if(t_mask) parameters->detector[detectorID]->maskChannel(t_col,t_row);
    }
  }
  
  // Close the files when finished
  trimdacs.close();

}

// Function to load data for a given device, into the relevant container
bool Timepix3EventLoader::loadData(string detectorID, Timepix3Pixels* devicedata ){

  // Check if current file is open
  if(m_currentFile[detectorID] == NULL){
    // Open a new file
    m_currentFile[detectorID] = fopen(m_datafiles[detectorID][m_fileNumber[detectorID]].c_str(),"rb");
    // Mark that this file is done
    m_fileNumber[detectorID]++;
    // Skip the header
    uint32_t headerID;
    if (fread(&headerID, sizeof(headerID), 1, m_currentFile[detectorID]) == 0) {
      tcout << "[Error] Cannot read header ID for device " << detectorID << endl;
      return false;
    }
    // Skip the rest of the file header
    uint32_t headerSize;
    if (fread(&headerSize, sizeof(headerSize), 1, m_currentFile[detectorID]) == 0) {
      tcout << "[Error] Cannot read header size for device " << detectorID << endl;
      return false;
    }
		// Finally skip the header
    rewind(m_currentFile[detectorID]);
    fseek(m_currentFile[detectorID], headerSize, SEEK_SET);
  }
  
  // Now read the data packets.
  ULong64_t pixdata = 0; UShort_t thr = 0;
  int npixels=0;
  // Read till the end of file (or till break)
  while (!feof(m_currentFile[detectorID])) {
    const int retval = fread(&pixdata, sizeof(ULong64_t), 1, m_currentFile[detectorID]);
//    bitset<64> packetContent(pixdata);
//    tcout<<hex<<pixdata<<dec<<endl;
//    tcout<<pixdata<<endl;
    if (retval == 0) continue;
    const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
    
    unsigned int headerInt = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
    tcout<<hex<<headerInt<<dec<<endl;
//    bitset<64> headerContent(header);
//    tcout<<"Header is "<<headerContent<<endl;

    if (header == 0xA || header == 0xB) {
      const UShort_t dcol = ((pixdata & 0x0FE0000000000000) >> 52);
      const UShort_t spix = ((pixdata & 0x001F800000000000) >> 45);
      const UShort_t pix  = ((pixdata & 0x0000700000000000) >> 44);
      const UShort_t col = (dcol + pix / 4);
      const UShort_t row = (spix + (pix & 0x3));
      const UShort_t pixno = col * 256 + row;
      const UInt_t data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
      const unsigned int tot = (data & 0x00003FF0) >> 4;

      const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
      const uint64_t ftoa(data & 0x0000000F);
      const uint64_t toa((data & 0x0FFFC000) >> 14);
      // Calculate the timestamp.
       uint64_t time = ((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8;
      
      // Check if this pixel is masked
      if(parameters->detector[detectorID]->masked(col,row)) continue;

//      tcout<<"Pixel time "<<(double)time<<endl;
      time += (long long int)(parameters->detector[detectorID]->timingOffset() * 4096. * 40000000.);
      tcout<<"Pixel time is "<<((double)time/(4096. * 40000000.))<<endl;
      bitset<48> timeInt(time);
      tcout<<" or "<<timeInt<<endl;
      
      // If events are loaded based on time intervals, take all hits where the time is within this window
      if( parameters->eventLength != 0. &&
         ((double)time/(4096. * 40000000.)) > (parameters->currentTime + parameters->eventLength) ){
        fseek(m_currentFile[detectorID], -1 * sizeof(ULong64_t), SEEK_CUR);
        break;
      }
      
      Timepix3Pixel* pixel = new Timepix3Pixel(detectorID,row,col,(int)tot,time);
      devicedata->push_back(pixel);
      npixels++;


    }
    // Stop when we reach some large number of pixels (if events not based on time)
    if(parameters->eventLength == 0. && npixels == 2000) break;
    
  }
  
  if(npixels == 0) return false;
  
  return true;

}

void Timepix3EventLoader::finalise(){
  
}







