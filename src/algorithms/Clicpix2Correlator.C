#include "Clicpix2Correlator.h"

using namespace corryvreckan;

Clicpix2Correlator::Clicpix2Correlator(Configuration config, Clipboard* clipboard)
: Algorithm(std::move(config), clipboard){}

template <typename T>
std::string makeString(T number) {
  std::ostringstream ss;
  ss << number;
  return ss.str();
}

void Clicpix2Correlator::initialise(Parameters* par){

  parameters = par;

  // Get the DUT ID
  dutID = parameters->DUT;

  // Initialise histograms
  hTrackDiffX["standard"] = new TH1F("hTrackDiffX_standard","hTrackDiffX_standard",4000,-20,20);
  hTrackDiffY["standard"] = new TH1F("hTrackDiffY_standard","hTrackDiffY_standard",4000,-20,20);

  hTrackDiffX["prevEvent"] = new TH1F("hTrackDiffX_prevEvent","hTrackDiffX_prevEvent",4000,-20,20);
  hTrackDiffY["prevEvent"] = new TH1F("hTrackDiffY_prevEvent","hTrackDiffY_prevEvent",4000,-20,20);

  // Rotatation histograms
  angleStart = 0;
  angleStep = 0.6;
  angleStop = 2.*M_PI;

  for(double angle=angleStart;angle<angleStop;angle+=angleStep){
    string name = "rotated" + makeString(angle);
    string histo = "hTrackDiffX_" + name;
    hTrackDiffX[name] = new TH1F(histo.c_str(),histo.c_str(),4000,-20,20);
  }


  // Initialise member variables
  m_eventNumber = 0;
}

StatusCode Clicpix2Correlator::run(Clipboard* clipboard){

  // Get the clicpix clusters in this event
  Clusters* clusters = (Clusters*)clipboard->get(dutID,"clusters");
  if(clusters == NULL){
    LOG(DEBUG) <<"No clusters for "<<dutID<<" on the clipboard";
    m_eventNumber++;
    return Success;
  }

  // Get the tracks
  Tracks* tracks = (Tracks*)clipboard->get("tracks");
  if(tracks == NULL){
    m_eventNumber++;
    return Success;
  }

  // Make local copies of these objects
  for(int iTrack=0; iTrack<tracks->size(); iTrack++){
    Track* track = (*tracks)[iTrack];
    Track* storageTrack = new Track(track);
    m_eventTracks[m_eventNumber].push_back(storageTrack);
  }
  for(int iCluster=0; iCluster<clusters->size(); iCluster++){
    Cluster* cluster = (*clusters)[iCluster];
    Cluster* storageCluster = new Cluster(cluster);
    m_eventClusters[m_eventNumber].push_back(storageCluster);
  }

  // Increment event counter
  m_eventNumber++;

  // Return value telling analysis to keep running
  return Success;
}

void Clicpix2Correlator::finalise(){

  LOG(DEBUG) <<"Analysed "<<m_eventNumber<<" events";

  // Now make all of the correlations. For each event loop over the clusters
  // and tracks and make correlations between the two

  // Will rotate the detector and look for correlations
  for(double angle=angleStart;angle<angleStop;angle+=angleStep){

    // Set the angle
    parameters->detector[dutID]->rotationX(angle);
    parameters->detector[dutID]->update();

    int event;
    for(event=0;event<m_eventNumber;event++){
      // Get the clusters and tracks
      Tracks tracks = m_eventTracks[m_eventNumber];
      Clusters clusters = m_eventClusters[m_eventNumber];

      // Loop over tracks and make correlations
      for(int iTrack=0;iTrack<tracks.size();iTrack++){

        // Get the track
        Track* track = tracks[iTrack];

        // Get the track intercept with the clicpix plane (global co-ordinates)
        PositionVector3D< Cartesian3D<double> > trackIntercept = parameters->detector[dutID]->getIntercept(track);

        // Loop over all clusters from this event
        for(int iCluster=0;iCluster<clusters.size();iCluster++){

          // Get the cluster
          Cluster* cluster = clusters[iCluster];

          // Get the distance between this cluster and the track intercept (global)
          double xcorr = cluster->globalX()-trackIntercept.X();
          double ycorr = cluster->globalY()-trackIntercept.Y();

          // Fill histograms on correlations
          string name = "rotated" + makeString(angle);
          hTrackDiffX[name]->Fill(xcorr);
          hTrackDiffY[name]->Fill(ycorr);

        }
      }
    }
  }
}
