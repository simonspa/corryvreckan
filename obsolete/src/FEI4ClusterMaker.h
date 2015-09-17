#ifndef FEI4CLUSTERMAKER_H 
#define FEI4CLUSTERMAKER_H 1

// Include files

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "FEI4Cluster.h"
#include "FEI4RowColumnEntry.h"

//#include "FEI4Cluster.h"

/** @class FEI4ClusterMaker FEI4ClusterMaker.h
 *  
 *  @author Daniel Hynds
 *  @date   2011-10-03
 *
 */

class FEI4ClusterMaker : public Algorithm {
 public: 
  /// Constructor
  FEI4ClusterMaker(); 
  FEI4ClusterMaker(Parameters*, bool);
  /// Destructor
  virtual ~FEI4ClusterMaker( ); 

  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();

 protected:

 private:

  bool display;
  Parameters *parameters;
  bool m_debug;
  int ntracksinacceptance;
  int ntracksinacceptancewithcluster;

  TH1F* tot_spectrum;
  TH1F* tot_spectrum_1pix;
  TH1F* tot_spectrum_2pix;
  TH1F* lv1id_raw;
  TH1F* bcid_raw;
  TH1F* cluster_size;
  TH1F* cluster_row_width;
  TH1F* cluster_col_width;
  TH1F* non_empty_triggers;
  TH1F* num_triggers;
  TH1F* col_number;
  TH1F* row_number;

  TH1F* rowposonerow;
  TH1F* rowpostworow;
  TH1F* rowposthreerow;
  TH1F* colposonecol;
  TH1F* colpostwocol;
  TH1F* colposthreecol;

  TH2F* superpixelonepixel;
  TH2F* superpixeltwopixel;
  TH2F* superpixelthreepixel;
  TH2F* superpixelfourpixel;

  TH1F* track_time_stamp;
  TH1F* track_time_stamp_minus_cluster_time_stamp;

	TH1F* residuals_y;
	TH1F* residuals_x;
	TH1F* residuals_x_1pix;
	TH1F* residuals_x_2pix;
  TH1F* track_intercept_x;
  TH1F* track_intercept_y;
  TH1F* track_correlation_y;
  TH1F* track_correlation_x;
  TH1F* track_correlation_x_1col;
  TH1F* track_correlation_x_2col;
  TH1F* track_correlation_x_3col;
  TH1F* track_correlation_x_4col;
  TH1F* track_correlation_y_global;
  TH1F* track_correlation_x_global;
  TH2F* track_correlation_y_vs_x;
  TH2F* track_correlation_x_vs_x;
  TH2F* track_correlation_y_vs_y;
  TH2F* track_correlation_x_vs_y;
  TH2F* track_local_x_vs_cluster_local_x;
  TH2F* track_local_y_vs_cluster_local_y;
  TH2F* track_local_x_vs_cluster_local_y;
  TH2F* track_local_y_vs_cluster_local_x;
  TH1F* cluster_local_x;
  TH1F* cluster_local_y;
  TH2F* track_intercept_x_vs_y;    
  TH2F* associated_track_intercept_x_vs_y;
  TH1F* correlation_y;
  TH1F* correlation_x;
  TH1F* local_y;
  TH1F* local_x;
  TH1F* numberHitsPerFrame;
  int frame_num;
    
  FEI4RowColumnEntries makeCluster(FEI4RowColumnEntries);
  void setClusterCenter(FEI4Cluster*);
  void setClusterGlobalPosition(FEI4Cluster*);
  void setClusterWidth(FEI4Cluster*);
};
#endif // FEI4CLUSTERMAKER_H
