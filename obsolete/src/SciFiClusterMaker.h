#ifndef SCIFICLUSTERMAKER_H 
#define SCIFICLUSTERMAKER_H 1

// Include files

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "SciFiCluster.h"

/** @class SciFiClusterMaker SciFiClusterMaker.h
 *  
 *  @author Julien Rouvinet
 *  @date   2011-08-18
 *
 */

class SciFiClusterMaker : public Algorithm {
 public: 
  /// Standard constructor
  SciFiClusterMaker(); 
  SciFiClusterMaker(Parameters*,bool); 
  virtual ~SciFiClusterMaker( ); ///< Destructor

  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();
	void setClusterCentre(SciFiCluster* cluster);

 protected:

 private:

	
  bool display;
  Parameters *parameters;
  bool m_debug;
	int eventNum;

	TH1F* cluster_ADC;
	TH1F* channel_ADC;
	TH1F* cluster_size;
	TH1F* channel;
	TH1F* correlation_x;
	TH1F* timeStampedTracksPlot;
	TH1F* timeStampedTracksPlot_perEvent;
	TH1F* nTracksPlot;
	TH1F* nTracksPlot_perEvent;
	TH1F* timeStampedSciFiPlot_perEvent;
	TH1F* timeStampedSciFiPlot2_perEvent;
	TH1F* residualPlot;
	TH2F* scifiresidvschan;
        TH1F* scifitimeresid;
        TH1F* scifisyncdelay;

  TH1F* numberHitsPerFrame;
};

#endif // SCIFICLUSTERMAKER_H
