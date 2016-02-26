#include "Timepix3MaskCreator.h"
#include <fstream>
#include <istream>


Timepix3MaskCreator::Timepix3MaskCreator(bool debugging)
: Algorithm("Timepix3MaskCreator"){
  debug = debugging;
}


void Timepix3MaskCreator::initialise(Parameters* par){
 
  parameters = par;
//  
//  // Make histograms for each Timepix3
//  for(int det = 0; det<parameters->nDetectors; det++){
//    
//    // Check if they are a Timepix3
//    string detectorID = parameters->detectors[det];
//    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
//    
//    // Simple hit map to look for hit pixels
//    string name = "pixelhits_"+detectorID;
//    pixelhits[detectorID] = new TH2F(name.c_str(),name.c_str(),256,0,256,256,0,256);
//    
//  }

  
}

StatusCode Timepix3MaskCreator::run(Clipboard* clipboard){

  // Loop over all Timepix3 and for each device perform the clustering
  for(int det = 0; det<parameters->nDetectors; det++){
   
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
		
    // Get the pixels
    Pixels* pixels = (Pixels*)clipboard->get(detectorID,"pixels");
    if(pixels == NULL){
      tcout<<"Detector "<<detectorID<<" does not have any pixels on the clipboard"<<endl;
      return Failure;
    }
    if(debug) tcout<<"Picked up "<<pixels->size()<<" pixels for device "<<detectorID<<endl;
    
    // Loop over all pixels
    for(int iP=0;iP<pixels->size();iP++){
      Pixel* pixel = (*pixels)[iP];
      
      // Enter another pixel hit for this channel
      int channelID = pixel->m_row + 256*pixel->m_column;      
      pixelhits[detectorID][channelID]++;
    }
  }

  return Success;
}

void Timepix3MaskCreator::finalise(){
  
  // Loop through all registered detectors
  for(int det = 0; det<parameters->nDetectors; det++){
   
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[det];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    
    // Get the trimdac file
    string trimdacfile = parameters->detector[detectorID]->maskFile();
    
    // Calculate what the mean number of hits was
    double meanHits = 0;
    for(int col=0;col<256;col++){
      for(int row=0;row<256;row++){
        int channelID =row + 256*col;
        meanHits+=pixelhits[detectorID][channelID];
      }
    }
    meanHits /= (256.*256.);
    
    // Make the new file name
    string newtrimdacfile = trimdacfile;
    newtrimdacfile.replace(newtrimdacfile.end()-4,newtrimdacfile.end(),"_masked.txt");
    
    // Open the old mask file
    ifstream trimdacs;
    trimdacs.open(trimdacfile.c_str());
    
    // Open the new mask file for writing
    ofstream newtrimdacs;
    newtrimdacs.open(newtrimdacfile.c_str());
    
    // Copy the header from the old to the new file
    string line;
    getline(trimdacs,line);
    newtrimdacs << line << endl;
    int t_col, t_row, t_trim, t_mask, t_tpen;
    
    // Loop again and mask any pixels which are noisy
    for(int col=0;col<256;col++){
      for(int row=0;row<256;row++){
        int channelID =row + 256*col;
        if(pixelhits[detectorID][channelID] > 100){
          trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
          newtrimdacs << t_col << "\t" << t_row << "\t" << t_trim << "\t" << "1" << "\t" << t_tpen << endl;
          tcout<<"Masking pixel "<<col<<","<<row<<" on detector "<<detectorID<<endl;
          tcout<<"Number of counts: "<<pixelhits[detectorID][channelID]<<endl;
        }else{
          // Just copy the existing line
          trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
          newtrimdacs << t_col << "\t" << t_row << "\t" << t_trim << "\t" << t_mask << "\t" << t_tpen << endl;
        }
      }
    }

    // Close the files when finished
    trimdacs.close();
    newtrimdacs.close();
    
    // This is a bit of a fudge. If the old trimdac file was a software-generated file (with name
    // CHIPID_trimdac_masked.txt) then the new file will have an additional _masked in the name.
    // In fact we want to replace the old file. So we now check if this is the case, and move the
    // new file where we want it
    if(trimdacfile.find("trimdac_masked") != string::npos){
      int result = rename(newtrimdacfile.c_str(),trimdacfile.c_str());
      if(result == 0) tcout<<"Trimdac file "<<trimdacfile<<" updated"<<endl;
      if(result != 0) tcout<<"Could not update trimdac file "<<trimdacfile<<endl;
    }
    
  }

  
}
