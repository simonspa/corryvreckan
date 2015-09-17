#include "Alignment.h"

#include <TVirtualFitter.h>

Alignment::Alignment(bool debugging)
: Algorithm("Alignment"){
  debug = debugging;
}

// Global container declarations
Timepix3Tracks globalTracks;
string detectorToAlign;
Parameters* globalParameters;
int detNum;

void Alignment::initialise(Parameters* par){
 
  parameters = par;
  
}

int Alignment::run(Clipboard* clipboard){
 
  // Get the tracks
  Timepix3Tracks* tracks = (Timepix3Tracks*)clipboard->get("Timepix3","tracks");
  if(tracks == NULL){
    return 1;
  }
  
  // Make a local copy and store it 
  for(int iTrack=0; iTrack<tracks->size(); iTrack++){
    Timepix3Track* track = (*tracks)[iTrack];
    Timepix3Track* alignmentTrack = new Timepix3Track(track);
    m_alignmenttracks.push_back(alignmentTrack);
  }
  
  return 1;

}

void Alignment::finalise(){
  
  // Make the fitting object
  TVirtualFitter* residualFitter = TVirtualFitter::Fitter(0,50);
  
  // Tell it what to minimise
  residualFitter->SetFCN(SumDistance2);
  
  // Set the global parameters
  globalTracks = m_alignmenttracks;
  globalParameters = parameters;

  // Set the printout arguments of the fitter
  Double_t arglist[10];
  arglist[0] = 3;
  residualFitter->ExecuteCommand("SET PRINT",arglist,1);
  
  // Set some fitter parameters
  arglist[0] = 1000; // number of function calls
  arglist[1] = 0.001; // tolerance
  
  // Loop over all planes. For each plane, set the plane alignment parameters which will be varied, and
  // then minimise the track chi2 (sum of biased residuals). This means that tracks are refitted with
  // each minimisation step.
  
  int det = 0;
  for(int ndet = 0; ndet<parameters->nDetectors; ndet++){
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[ndet];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    // Do not align the reference plane
    if(detectorID == parameters->reference) continue;
    if(detectorID == "W0013_F09") continue;
    if(detectorID == "W0013_G02") continue;
    // Say that this is the detector we align
    detectorToAlign = detectorID;
    detNum=det;
    // Add the parameters to the fitter (z displacement not allowed to move!)
    residualFitter->SetParameter(det*6+0,(detectorID+"_displacementX").c_str(),parameters->detector[detectorID]->displacementX(),0.01,-50,50);
    residualFitter->SetParameter(det*6+1,(detectorID+"_displacementY").c_str(),parameters->detector[detectorID]->displacementY(),0.01,-50,50);
    residualFitter->SetParameter(det*6+2,(detectorID+"_displacementZ").c_str(),parameters->detector[detectorID]->displacementZ(),0,-10,500);
    residualFitter->SetParameter(det*6+3,(detectorID+"_rotationX").c_str(),parameters->detector[detectorID]->rotationX(),0.01,-6.30,6.30);
    residualFitter->SetParameter(det*6+4,(detectorID+"_rotationY").c_str(),parameters->detector[detectorID]->rotationY(),0.01,-6.30,6.30);
    residualFitter->SetParameter(det*6+5,(detectorID+"_rotationZ").c_str(),parameters->detector[detectorID]->rotationZ(),0.01,-6.30,6.30);
    
    // Fit this plane (minimising global track chi2)
		residualFitter->ExecuteCommand("MIGRAD",arglist,2);
    
    // Now that this device is fitted, set parameter errors to 0 so that they are not fitted again
    residualFitter->SetParameter(det*6+0,(detectorID+"_displacementX").c_str(),residualFitter->GetParameter(det*6+0),0,-50,50);
    residualFitter->SetParameter(det*6+1,(detectorID+"_displacementY").c_str(),residualFitter->GetParameter(det*6+1),0,-50,50);
    residualFitter->SetParameter(det*6+2,(detectorID+"_displacementZ").c_str(),residualFitter->GetParameter(det*6+2),0,-10,500);
    residualFitter->SetParameter(det*6+3,(detectorID+"_rotationX").c_str(),residualFitter->GetParameter(det*6+3),0,-6.30,6.30);
    residualFitter->SetParameter(det*6+4,(detectorID+"_rotationY").c_str(),residualFitter->GetParameter(det*6+4),0,-6.30,6.30);
    residualFitter->SetParameter(det*6+5,(detectorID+"_rotationZ").c_str(),residualFitter->GetParameter(det*6+5),0,-6.30,6.30);
    // Set the alignment parameters of this plane to be the optimised values from the alignment
    parameters->detector[detectorID]->displacementX(residualFitter->GetParameter(det*6+0));
    parameters->detector[detectorID]->displacementY(residualFitter->GetParameter(det*6+1));
    parameters->detector[detectorID]->displacementZ(residualFitter->GetParameter(det*6+2));
    parameters->detector[detectorID]->rotationX(residualFitter->GetParameter(det*6+3));
    parameters->detector[detectorID]->rotationY(residualFitter->GetParameter(det*6+4));
    parameters->detector[detectorID]->rotationZ(residualFitter->GetParameter(det*6+5));
    parameters->detector[detectorID]->update();
    det++;
  }
  
  det = 0;
  // Now list the new alignment parameters
  for(int ndet = 0; ndet<parameters->nDetectors; ndet++){
    // Check if they are a Timepix3
    string detectorID = parameters->detectors[ndet];
    if(parameters->detector[detectorID]->type() != "Timepix3") continue;
    // Do not align the reference plane
    if(detectorID == parameters->reference) continue;
    if(detectorID == "W0013_F09") continue;
    if(detectorID == "W0013_G02") continue;
    
    // Get the alignment parameters
    double displacementX = residualFitter->GetParameter(det*6+0);
    double displacementY = residualFitter->GetParameter(det*6+1);
    double displacementZ = residualFitter->GetParameter(det*6+2);
    double rotationX = residualFitter->GetParameter(det*6+3);
    double rotationY = residualFitter->GetParameter(det*6+4);
    double rotationZ = residualFitter->GetParameter(det*6+5);

    tcout<<" Detector "<<detectorID<<" new alignment parameters: T("<<displacementX<<","<<displacementY<<","<<displacementZ<<") R("<<rotationX<<","<<rotationY<<","<<rotationZ<<")"<<endl;
    
    det++;
  }
  
  // Write the output alignment file
  parameters->writeConditions();
}

void SumDistance2(Int_t &npar, Double_t *grad, Double_t &result, Double_t *par, Int_t flag) {
  
//  cout<<"Parameter size "<<par.size()<<endl;
  // Pick up new alignment conditions
  globalParameters->detector[detectorToAlign]->displacementX(par[detNum*6 + 0]);
  globalParameters->detector[detectorToAlign]->displacementY(par[detNum*6 + 1]);
  globalParameters->detector[detectorToAlign]->displacementZ(par[detNum*6 + 2]);
  globalParameters->detector[detectorToAlign]->rotationX(par[detNum*6 + 3]);
  globalParameters->detector[detectorToAlign]->rotationY(par[detNum*6 + 4]);
  globalParameters->detector[detectorToAlign]->rotationZ(par[detNum*6 + 5]);
  // Apply new alignment conditions
  globalParameters->detector[detectorToAlign]->update();

//  cout<<"displacement x "<<par[detNum*6 + 0]<<endl;
//  cout<<"displacement y "<<par[detNum*6 + 1]<<endl;
//  cout<<"displacement z "<<par[detNum*6 + 2]<<endl;
//  cout<<"rotation x "<<par[detNum*6 + 3]<<endl;
//  cout<<"rotation y "<<par[detNum*6 + 4]<<endl;
//  cout<<"rotation z "<<par[detNum*6 + 5]<<endl;
//  cout<<"Updated alignmnet parameters for detector "<<detectorToAlign<<endl;
  
  // The chi2 value to be returned
  result = 0.;

  // Loop over all tracks
  for(int iTrack=0; iTrack<globalTracks.size(); iTrack++){
		// Get the track
    Timepix3Track* track = globalTracks[iTrack];
    // Get all clusters on the track
    Timepix3Clusters trackClusters = track->clusters();
    // Find the cluster that needs to have its position recalculated
    for(int iTrackCluster=0; iTrackCluster<trackClusters.size(); iTrackCluster++){
      Timepix3Cluster* trackCluster = trackClusters[iTrackCluster];
      string detectorID = trackCluster->detectorID();
      // Recalculate the global position from the local
      PositionVector3D<Cartesian3D<double> > positionLocal(trackCluster->localX(),trackCluster->localY(),trackCluster->localZ());
      PositionVector3D<Cartesian3D<double> > positionGlobal = *(globalParameters->detector[detectorID]->m_localToGlobal) * positionLocal;
      trackCluster->setClusterCentre(positionGlobal.X(), positionGlobal.Y(),positionGlobal.Z());
    }
    // Refit the track
    track->fit();
    // Add the new chi2
    result += track->chi2();
  }
//  cout<<"Chi2: "<<result<<endl;
}

