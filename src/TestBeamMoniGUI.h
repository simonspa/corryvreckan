#ifndef TESTBEAM_MONI_GUI_H
#define TESTBEAM_MONI_GUI_H

// STL 
#include <string>
#include <vector>

// ROOT
#include <TQObject.h>
#include <RQ_OBJECT.h>
#include <TApplication.h>
#include <TRootEmbeddedCanvas.h>
#include <TGTab.h>
#include <TFile.h>
#include <TGFrame.h>
#include <TGTextEdit.h>
#include <TGTextView.h>
#include <TGTextEntry.h>
#include <TGNumberEntry.h>
#include <TGMsgBox.h>
#include <TGComboBox.h>
#include <TPaveLabel.h>

// Enumeration for the menu bar items
enum MenuIdentifiers {
  M_FILE_HISTO_OPEN,
  M_FILE_EVENT_OPEN,
  M_FILE_HISTO_CLOSE,
  M_FILE_EVENT_CLOSE,
  M_FILE_LOAD_CONFIG,
  M_FILE_SAVE_CANVAS,
  M_FILE_EXIT
};

enum TabIdentifiers {
  M_TAB_CLUSTERS,
  M_TAB_TRACKS,
  M_TAB_CONFIG,
  M_TAB_AMALGAMATION
};

enum TabClusterButtons {
  M_CLUSTER_HITS_PER_FRAME,
  M_CLUSTER_DIFFERENCES_X,
  M_CLUSTER_DIFFERENCES_Y,
  M_CLUSTER_CORRELATIONS_X,
  M_CLUSTER_CORRELATIONS_Y,
  M_CLUSTER_SIZE,
  M_CLUSTER_TOT_SPECTRUM,
  M_CLUSTER_LOCAL_POS,
  M_CLUSTER_GLOBAL_POS
};

enum TabTrackButtons {
  M_TRACK_PROTO_TRACKS,
  M_TRACK_FIT,
  M_TRACK_RESIDUALS_X,
  M_TRACK_RESIDUALS_Y,
  M_TRACK_RESIDUALS_ASSOCIATED_X,
  M_TRACK_RESIDUALS_ASSOCIATED_Y,
  M_TRACK_INTERCEPTS_LOCAL,
  M_TRACK_INTERCEPTS_GLOBAL,
  M_TRACK_INTERCEPTS_TIMESTAMPED,
  M_TRACK_INTERCEPTS_NONTIMESTAMPED
};

enum TabConfigButtons {
  M_CONFIG_THL_DIST,
  M_CONFIG_ADJUSTMENT,
  M_CONFIG_MASKED
};

enum TabAmalgamationButtons {
  M_AMALG_TDC
};

class TestBeamMoniGUI {
  
  RQ_OBJECT("TestBeamMoniGUI")
	 
public:
  // Constructor
  TestBeamMoniGUI(std::string histogramFile, 
                  std::string eventFile, 
                  std::string conditionsFile,
                  std::string configFolder);
  // Destructor
  virtual ~TestBeamMoniGUI();

  bool buttonOpenHistogramFile();
  bool buttonOpenEventFile();
  bool buttonOpenConfigFolder();
  
  void handleMenu(int id);
 
  // Describe the contents of the tabs
  void contentsClusters(TGCompositeFrame* tf);
  void contentsTracks(TGCompositeFrame* tf);
  void contentsConfig(TGCompositeFrame* tf);
  void contentsAmalgamation(TGCompositeFrame* tf);
	
  void updateTabClustersCanvas();
  void updateTabTracksCanvas();
  void updateTabConfigCanvas();
  void updateTabAmalgamationCanvas();

  void handle_txtBtnHitsPerFrame();
  void handle_txtBtnClusterDifferencesX();
  void handle_txtBtnClusterDifferencesY();
  void handle_txtBtnClusterDifferences(const char xy);
  void handle_txtBtnClusterCorrelationsX();
  void handle_txtBtnClusterCorrelationsY();
  void handle_txtBtnClusterCorrelations(const char xy);
  void handle_txtBtnClusterSize();
  void handle_txtBtnTOTSpectrum();
  void handle_txtBtnLocalClusterPositions();
  void handle_txtBtnGlobalClusterPositions();

  void handle_txtBtnProtoTracks();
  void handle_txtBtnTrackFit();
  void handle_txtBtnResidualsX();
  void handle_txtBtnResidualsY();
  void handle_txtBtnResiduals(const char xy, const bool assoc);
  void handle_txtBtnResidualsAssociatedX();
  void handle_txtBtnResidualsAssociatedY();
  void handle_txtBtnInterceptsGlobal(); 
  void handle_txtBtnInterceptsLocal(); 
  void handle_txtBtnIntercepts(const bool global);
  void handle_txtBtnInterceptsTimestampYes();
  void handle_txtBtnInterceptsTimestampNo();
  void handle_txtBtnInterceptsTimestamp(const bool yes); 

  void handle_txtBtnTHLDistributions();
  void handle_txtBtnAdjBits();
  void handle_txtBtnMasked();
  
  void handle_txtBtnTDC();
  
  void update();
  void doNothing() {}; 
  ClassDef(TestBeamMoniGUI, 0);

private:

  unsigned int m_w, m_h;

  std::string m_defaultPath;

  TGMainFrame* m_mainFrame;
  TGPopupMenu* m_menuFile;
  TGTab* m_tabFrame;
  
  TFile* m_currentHistogramFile;
  TFile* m_currentEventFile;
  std::string m_currentHistogramFileName;
  std::string m_currentEventFileName;
  std::string m_currentConfigFolder;
  std::string m_currentAlignmentFileName;
  TGGroupFrame* m_datasetFrameHistogramFile;
  TGGroupFrame* m_datasetFrameEventFile;
  TGGroupFrame* m_datasetFrameConfigFile;
  TGGroupFrame* m_datasetFrameAlignmentFile;
  TGLabel* m_datasetLabelHistogramFile;
  TGLabel* m_datasetLabelEventFile;
  TGLabel* m_datasetLabelConfigFile;
  TGLabel* m_datasetLabelAlignmentFile;

  TRootEmbeddedCanvas* m_ecClusters;
  TRootEmbeddedCanvas* m_ecTracks;
  TRootEmbeddedCanvas* m_ecConfig;
  TRootEmbeddedCanvas* m_ecAmalgamation;
 
  // "Active" buttons
  int m_clusters_currentCanvasButton;
  int m_tracks_currentCanvasButton;
  int m_config_currentCanvasButton;
  int m_amalg_currentCanvasButton;
  
  struct chipConfig {
    std::string chipId;
    int relaxdId, chipPos;
    std::vector<std::vector<int> > distributions;
    int startValue;
    int endValue;
    int nSteps;
    std::vector<int> bits;
    std::vector<std::vector<bool> > masked;
  };
  std::vector<chipConfig> configs;

  struct alignmentParameters {
    std::string chipId;
    double x, y, z;
  };
  std::vector<alignmentParameters> alignments;
  
  bool openHistogramFile(const char* filename);
  bool openEventFile(const char* filename);
  bool closeHistogramFile();
  bool closeEventFile();
  bool loadConfigurationData(const std::string folder);
  bool loadAlignmentFile(const std::string filename);
  bool saveCanvas();

  bool getDetectorNames(std::vector<std::string>& names);
  
  void setROOTColourPalette();
  void setROOTStyle();

  // Convert integer to string
  std::string toString(int num);
  // Convert double to string
  std::string toString(double num, int precision);
  // Custom operator for sorting planes by z position
  struct increasingZ {
    bool operator() (const std::pair<std::string, double>& chip1, 
                     const std::pair<std::string, double>& chip2) {
      double z1 = chip1.second;
      double z2 = chip2.second;
      return (z1 < z2);
    }
  };

};

#endif // TESTBEAM_MONI_GUI_H
