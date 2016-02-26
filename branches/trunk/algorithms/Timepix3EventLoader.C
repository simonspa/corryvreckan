#include "Timepix3EventLoader.h"

#include <dirent.h>
#include <sstream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <bitset>

Timepix3EventLoader::Timepix3EventLoader(bool debugging)
: Algorithm("Timepix3EventLoader"){
  debug = debugging;
  applyTimingCut = false;
  m_currentTime = 0.;
  m_minNumberOfPlanes = 1;
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
      
      // Check if this device is to be masked
      if(parameters->masked.count(detectorID) != 0) continue;
      
      // Get all of the files for this chip
      while (file = readdir(dataDir)){
        string filename = dataDirName+"/"+file->d_name;

        // Check if file has extension .dat
        if (string(file->d_name).find("1.dat") != string::npos){
          m_datafiles[detectorID].push_back(filename.c_str());
          m_nFiles[detectorID]++;

          // Initialise null values for later
          m_currentFile[detectorID] = NULL;
          m_fileNumber[detectorID] = 0;
          m_syncTime[detectorID] = 0;
          m_clearedHeader[detectorID] = false;
        }
        
        // If not a data file, it might be a trimdac file, with the list of masked pixels etc.
        if (string(file->d_name).find("trimdac") != string::npos){
          // If we have already found the masked trimdac file, use it for preference
          if(trimdacfile.find("masked") != string::npos) continue;
          trimdacfile = filename;
        }
        
      }
      
      // If files were stored, register the detector (check that it has alignment data)
      if(m_nFiles.count(detectorID) > 0){
        
        tcout<<"Registering detector "<<detectorID<<endl;
        if(parameters->detector.count(detectorID) == 0){tcout<<"Detector "<<detectorID<<" has no alignment/conditions loaded. Please check that it is in the alignment file"<<endl; return;}
//        parameters->registerDetector(detectorID);
        
        // Now that we have all of the data files and mask files for this detector, pass the mask file to parameters
        tcout<<"Set mask file "<<trimdacfile<<endl;
        parameters->detector[detectorID]->setMaskFile(trimdacfile);
        
        // Apply the pixel masking
        maskPixels(detectorID,trimdacfile);
      }

    }
  }
}

StatusCode Timepix3EventLoader::run(Clipboard* clipboard){
  
  // This will loop through each timepix3 registered, and load data from each of them. This can
  // be done in one of two ways: by taking all data in the time interval (t,t+delta), or by
  // loading a fixed number of pixels (ie. 2000 at a time)
  
  int endOfFiles = 0; int devices = 0; int loadedData = 0;
  cout<<"\rCurrent time: "<<parameters->currentTime<<flush;

  // Loop through all registered detectors
  for(int det = 0; det<parameters->nDetectors; det++){
    
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    
    // Make a new container for the data
    Pixels* deviceData = new Pixels();
    SpidrSignals* spidrData = new SpidrSignals();

		// Load the next chunk of data
    if(debug) tcout<<"Loading data from "<<detectorID<<endl;
    bool data = loadData(detectorID,deviceData,spidrData);
    
    // If data was loaded then put it on the clipboard
    if(data){
      loadedData++;
      if(debug) tcout<<"Loaded "<<deviceData->size()<<" pixels for device "<<detectorID<<endl;
      clipboard->put(detectorID,"pixels",(TestBeamObjects*)deviceData);
    }
    clipboard->put(detectorID,"SpidrSignals",(TestBeamObjects*)spidrData);
    
    // Check if all devices have reached the end of file
    devices++;
    if(feof(m_currentFile[detectorID])) endOfFiles++;
  }
  
  // Increment the event time
  parameters->currentTime += parameters->eventLength;
  
  // If all files are finished, tell the event loop to stop
  if(endOfFiles == devices) return Failure;
  
  // If no/not enough data in this event then tell the event loop to directly skip to the next event
  if(loadedData < m_minNumberOfPlanes) return NoData;
  
  // Otherwise tell event loop to keep running
  return Success;
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
bool Timepix3EventLoader::loadData(string detectorID, Pixels* devicedata, SpidrSignals* spidrData){

//  if(detectorID == "W0019_F07") debug = true;
//  if(detectorID != "W0019_F07") debug = false;
  
  if(debug) tcout<<"Loading data for device "<<detectorID<<endl;

  // Check if current file is open
  if(m_currentFile[detectorID] == NULL || feof(m_currentFile[detectorID])){
  if(debug) tcout<<"No current file open "<<endl;

    // If all files are finished, return
    if(m_fileNumber[detectorID] == m_datafiles[detectorID].size()){
      if(debug) tcout<<"All files have been analysed. There were "<<m_datafiles[detectorID].size()<<endl;
      return false;
    }
    
    // Open a new file
    m_currentFile[detectorID] = fopen(m_datafiles[detectorID][m_fileNumber[detectorID]].c_str(),"rb");
    tcout<<"Loading file "<<m_datafiles[detectorID][m_fileNumber[detectorID]]<<endl;
   
    // Mark that this file is done
    m_fileNumber[detectorID]++;
    
    // Skip the header - first read how big it is
    uint32_t headerID;
    if (fread(&headerID, sizeof(headerID), 1, m_currentFile[detectorID]) == 0) {
      tcout << "Cannot read header ID for device " << detectorID << endl;
      return false;
    }
    
    // Skip the rest of the file header
    uint32_t headerSize;
    if (fread(&headerSize, sizeof(headerSize), 1, m_currentFile[detectorID]) == 0) {
      tcout << "Cannot read header size for device " << detectorID << endl;
      return false;
    }
    
		// Finally skip the header
    rewind(m_currentFile[detectorID]);
    fseek(m_currentFile[detectorID], headerSize, SEEK_SET);
  }
  
  // Now read the data packets.
  ULong64_t pixdata = 0; UShort_t thr = 0;
  int npixels=0;
  bool fileNotFinished = false;
  
  // Read till the end of file (or till break)
  while (!feof(m_currentFile[detectorID])) {
    
    // Read one 64-bit chunk of data
    const int retval = fread(&pixdata, sizeof(ULong64_t), 1, m_currentFile[detectorID]);
    if(debug) bitset<64> packetContent(pixdata);
    if(debug) tcout<<hex<<pixdata<<dec<<endl;
    if(debug) tcout<<pixdata<<endl;
    if (retval == 0) continue;
    
    // Get the header (first 4 bits) and do things depending on what it is
    // 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
    const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
    
    // Use header 0x4 to get the long timestamps (called syncTime here)
    if(header == 0x4){
      
      // The 0x4 header tells us that it is part of the timestamp
      // There is a second 4-bit header that says if it is the most
      // or least significant part of the timestamp
      const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

      // This is a bug fix. There appear to be errant packets with garbage data - source to be tracked down.
      // Between the data and the header the intervening bits should all be 0, check if this is the case
      const UChar_t intermediateBits = ((pixdata & 0x00FFF00000000000) >> 44) & 0xFFF;
      if(intermediateBits != 0x000) continue;
      
      // 0x4 is the least significant part of the timestamp
      if(header2 == 0x4){
        // The data is shifted 16 bits to the right, then 12 to the left in order to match the timestamp format (net 4 right)
        m_syncTime[detectorID] = (m_syncTime[detectorID] & 0xFFFFF00000000000) + ((pixdata & 0x0000FFFFFFFF0000) >> 4);
      }
			// 0x5 is the most significant part of the timestamp
      if(header2 == 0x5){
        // The data is shifted 16 bits to the right, then 44 to the left in order to match the timestamp format (net 28 left)
        m_syncTime[detectorID] = (m_syncTime[detectorID] & 0x00000FFFFFFFFFFF) + ((pixdata & 0x00000000FFFF0000) << 28);
        if( m_syncTime[detectorID] < 0x0000010000000000 && !m_clearedHeader[detectorID]) m_clearedHeader[detectorID] = true;
      }
      
    }
    
    // Header 0x06 and 0x07 are the start and stop signals for power pulsing
    if(header == 0x0){
      
      // Get the second part of the header
      const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

      // 0x6 is power on
      if(header2 == 0x6){
        const uint64_t time( (pixdata & 0x0000000FFFFFFFFF) << 12 );
        SpidrSignal* signal = new SpidrSignal("powerOn",time);
        spidrData->push_back(signal);
        if(debug) tcout<<"Turned on power! Time: "<<(double)time/(4096. * 40000000.)<<endl;
       }
      // 0x7 is power off
      if(header2 == 0x7){
        const uint64_t time( (pixdata & 0x0000000FFFFFFFFF) << 12 );
        SpidrSignal* signal = new SpidrSignal("powerOff",time);
        spidrData->push_back(signal);
        if(debug) tcout<<"Turned off power! Time: "<<(double)time/(4096. * 40000000.)<<endl;
      }
    }
    
    // In data taking during 2015 there was sometimes still data left in the buffers at the start of
    // a run. For that reason we keep skipping data until this "header" data has been cleared, when
    // the heart beat signal starts from a low number (~few seconds max)
    if(!m_clearedHeader[detectorID]) continue;
    
    // Header 0xA and 0xB indicate pixel data
    if(header == 0xA || header == 0xB) {
      
      // Decode the pixel information from the relevant bits
      const UShort_t dcol = ((pixdata & 0x0FE0000000000000) >> 52);
      const UShort_t spix = ((pixdata & 0x001F800000000000) >> 45);
      const UShort_t pix  = ((pixdata & 0x0000700000000000) >> 44);
      const UShort_t col = (dcol + pix / 4);
      const UShort_t row = (spix + (pix & 0x3));
      
      // Check if this pixel is masked
      if(parameters->detector[detectorID]->masked(col,row)) continue;

      // Get the rest of the data from the pixel
      const UShort_t pixno = col * 256 + row;
      const UInt_t data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
      const unsigned int tot = (data & 0x00003FF0) >> 4;
      const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
      const uint64_t ftoa(data & 0x0000000F);
      const uint64_t toa((data & 0x0FFFC000) >> 14);
      
      // Calculate the timestamp.
      long long int time = (((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8) + (m_syncTime[detectorID] & 0xFFFFFC0000000000);
      if(debug) tcout<<"Pixel time "<<(double)time/(4096. * 40000000.)<<endl;
      if(debug) tcout<<"Sync time "<<(double)m_syncTime[detectorID]/(4096. * 40000000.)<<endl;
      
      // Add the timing offset from the coniditions file (if any)
      time += (long long int)(parameters->detector[detectorID]->timingOffset() * 4096. * 40000000.);

      // The time from the pixels has a maximum value of ~26 seconds. We compare the pixel time
      // to the "heartbeat" signal (which has an overflow of ~4 years) and check if the pixel
      // time has wrapped back around to 0
      
      // If the counter overflow happens before reading the new heartbeat
      while( abs(m_syncTime[detectorID]-time) > 0x0000020000000000 ){
        time += 0x0000040000000000;
      }

      // If events are loaded based on time intervals, take all hits where the time is within this window
      
      // Ignore pixels if they arrive before the current event window
      if( parameters->eventLength != 0. && ((double)time/(4096. * 40000000.)) < (parameters->currentTime) ){
        continue;
      }
      
      // Stop looking at data if the pixel is after the current event window (and rewind the file
      // reader so that we start with this pixel next event)
      if( parameters->eventLength != 0. &&
         ((double)time/(4096. * 40000000.)) > (parameters->currentTime + parameters->eventLength) ){
        fseek(m_currentFile[detectorID], -1 * sizeof(ULong64_t), SEEK_CUR);
        fileNotFinished = true;
        break;
      }
      
      // Otherwise create a new pixel object
      Pixel* pixel = new Pixel(detectorID,row,col,(int)tot,time);
      devicedata->push_back(pixel);
      npixels++;

    }
    
    // Stop when we reach some large number of pixels (if events not based on time)
    if(parameters->eventLength == 0. && npixels == 2000){
      fileNotFinished = true;
      break;
    }
    
  }
  
  // If no data was loaded, return false
  if(npixels == 0) return false;
  
  return true;

}

void Timepix3EventLoader::finalise(){
  
}







