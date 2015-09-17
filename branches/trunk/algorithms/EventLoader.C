#include "EventLoader.h"
#include "Pixel.h"

#include <dirent.h>
#include <sstream>
#include <string>

EventLoader::EventLoader(bool debugging)
: Algorithm("EventLoader"){
  
  debug = debugging;
  m_inputDirectory = "testDir";
  m_nFiles = 0;
  m_fileNumber = 0;
  
}


void EventLoader::initialise(Parameters* par){
 
  parameters = par;
  // Open the input directory and store all filenames
  DIR* directory = opendir(m_inputDirectory.c_str());
  dirent* file;
  while (file = readdir(directory)){
    if (string(file->d_name).find(".txt") != string::npos){
      m_files.push_back(m_inputDirectory + "/" + (string)file->d_name);
      m_nFiles++;
    }
  }
  
  
  
  
}

int EventLoader::run(Clipboard* clipboard){

  cout<<"\rLooping over files: "<<m_fileNumber<<"/"<<m_nFiles<<flush;
  // If there are no more files to open, finish the event loop
  if(!m_currentFile.is_open() && (m_fileNumber == m_nFiles)){
    cout<<endl;
    return 0;
  }
  
  // If no files are open, look at the next input file
  if(!m_currentFile.is_open()){
    m_currentFile.open(m_files[m_fileNumber]);
    m_fileNumber++;
  }
  
  // Make the temporary containers for the data
  vector<string> devices;
  map<string, Pixels*> deviceData;
  
  
  // Get the data from the input file
  // The first line should always be a header
  string line; int row, col, tot; int pos;
  long int time=0;
  string deviceID;
  while(1){
    // If we are at the end of the file, close
    if( m_currentFile.eof() ){
      m_currentFile.close();
      break;
    }
    // Note where we are in the file in case we need to come back
    pos = m_currentFile.tellg();
    // Read the next line
    getline(m_currentFile,line);
    // Check if it is a header
    if (line.find("#") != string::npos){
      // Get the header information
      long int headerTime;
      readHeader(line, deviceID, headerTime);
			// If this is the first frame, set the current time
      if(time == 0) time = headerTime;
      // If the new header is not part of this event, stop looking for more data
      if(headerTime > time){
        // Reset position in the input file, to start reading there next time
        m_currentFile.seekg(pos,m_currentFile.beg);
        break;
      }
      // Register new device
      devices.push_back(deviceID);
      deviceData[deviceID] = new Pixels();
    }
    // If not a header, store the data
    else{
      istringstream input(line);
      input >> row >> col >> tot;
      Pixel* pixel = new Pixel();
      pixel->m_adc = tot;
      pixel->m_column = col;
      pixel->m_row = row;
      pixel->m_detectorID = deviceID;
      deviceData[deviceID]->push_back(pixel);
    }
  }
  
  // Save the data to the clipboard
  int nDevices = devices.size();
  for(int device=0;device<nDevices;device++){
    string deviceName = devices[device];
    clipboard->put(deviceName,"pixels",(TestBeamObjects*)deviceData[deviceName]);
  }
  
  return 1;
}

void EventLoader::finalise(){
  
}

void EventLoader::readHeader(string header, string& deviceID, long int& time){
  
  // Example header
//# Start time (string) : Oct 04 Oct 04 00:06:0.0.000004170 # Start time : 3600000004170 # Acq time : 0.015000 # ChipboardID : Mim-osa04 # DACs : 5 100 255 127 127 0 385 7 130 130 80 56 128 128 # Mpx type : 3 # Timepix clock : 40 # Eventnr 497 # RelaxD 3 devs 4 TPX DAQ = 0x110402 = START_HW STOP_HW MPX_PAR_READ COMPRESS=1
  
  int timeStart = header.find("Start time : ")+13;
  int timeStop = header.find(" # Acq time");
  
  string timeString = header.substr(timeStart,timeStop-timeStart);

  unsigned idStart = header.find("ChipboardID : ")+14;
  unsigned idStop = header.find(" # DACs");
  
  deviceID = header.substr(idStart,idStop-idStart);
  
  time = stol( timeString );
  
}






