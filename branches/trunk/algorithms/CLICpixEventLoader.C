#include "CLICpixEventLoader.h"

CLICpixEventLoader::CLICpixEventLoader(bool debugging)
: Algorithm("CLICpixEventLoader"){
  debug = debugging;
  m_filename = "";
}


void CLICpixEventLoader::initialise(Parameters* par){
 
  parameters = par;
  
  // File structure is RunX/CLICpix/RunX.dat

  // Take input directory from global parameters
  string inputDirectory = parameters->inputDirectory + "/CLICpix";
  
  // Open the root directory
  DIR* directory = opendir(inputDirectory.c_str());
  if (directory == NULL){tcout<<"Directory "<<inputDirectory<<" does not exist"<<endl; return;}
  dirent* entry; dirent* file;
  
  // Read the entries in the folder
  while (entry = readdir(directory)){
    // Check for the data file
    string filename = inputDirectory + "/" + entry->d_name;
    if(filename.find(".dat") != string::npos){
      m_filename = filename;
    }
  }
  
  // If no data was loaded, give a warning
  if(m_filename.length() == 0) tcout<<"Warning: no data file was found for CLICpix in "<<inputDirectory<<endl;
  
  // Open the data file for later
  m_file.open(m_filename.c_str());
}


StatusCode CLICpixEventLoader::run(Clipboard* clipboard){
  
  // Assume that the CLICpix is the DUT (if running this algorithm
  string detectorID = parameters->DUT;
		
  // If have reached the end of file, close it and exit program running
  if(m_file.eof()){
    m_file.close();
    return Failure;
  }
  
  // Otherwise load a new frame
  
  // Pixel container, shutter information
  Pixels* pixels;
  long double shutterStartTime, shutterStopTime;
  string data;
  
  // Read file and load data
	while(getline(m_file,data)){
    
    // If line is empty then we have finished this event, stop looping
    if(data.length() == 0) break;
    
    // Check if this is a header/shutter/power info
    if(data.find("PWR_RISE") != string::npos || data.find("PWR_FALL") != string::npos) continue;
    if(data.find("SHT_RISE") != string::npos){
      // Read the shutter start time
      long int timeInt; string name;
      istringstream header(data);
      header >> name >> timeInt;
      shutterStartTime = (double)timeInt/(4096. * 40000000.);
      continue;
    }
    if(data.find("SHT_FALL") != string::npos){
      // Read the shutter stop time
      long int timeInt; string name;
      istringstream header(data);
      header >> name >> timeInt;
      shutterStopTime = (double)timeInt/(4096. * 40000000.);
      continue;
    }
    
    // Otherwise load data
    int row, col, tot;
    long int time;
    istringstream pixelData(data);
    pixelData >> col >> row >> tot;
    Pixel* pixel = new Pixel(detectorID,row,col,tot);
    pixels->push_back(pixel);

  }
  
  // Now set the event time so that the Timepix3 data is loaded correctly
  parameters->currentTime = shutterStartTime;
  parameters->eventLength = (shutterStopTime-shutterStartTime);

  // Put the data on the clipboard
  clipboard->put(detectorID,"pixels",(TestBeamObjects*)pixels);

  // Return value telling analysis to keep running
  return Success;
}

void CLICpixEventLoader::finalise(){
  
  if(debug) tcout<<"Analysed "<<m_eventNumber<<" events"<<endl;
  
}
