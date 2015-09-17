#include <iostream>
#include <iomanip>
#include <fstream>

// STL
#include <sstream>
#include <utility>
#include <algorithm>

// Boost
#include <boost/algorithm/string.hpp>

// ROOT
#include <TROOT.h>
#include <TMath.h>

#include <TColor.h>
#include <TStyle.h>

#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>

#include <TGWindow.h>
#include <TGButtonGroup.h>
#include <TGTextBuffer.h>
#include <TGClient.h>
#include <TGImageMap.h>
#include <TGMenu.h>
#include <TGFileDialog.h>
#include <TGLabel.h>
#include <TGWidget.h>
#include <TGInputDialog.h>

#include <TString.h>

#include <TCanvas.h>
#include <TGaxis.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPaveText.h>

#include <TTree.h>

#include "TestBeamDataSummary.h"
#include "TestBeamMoniGUI.h"

// Based on
//    * VeloMoniGUI
//    * ROOT macros by Christos Hadjivasiliou

//=============================================================================
// Constructor
//=============================================================================
TestBeamMoniGUI::TestBeamMoniGUI(std::string histogramFile, 
                                 std::string eventFile,
                                 std::string conditionsFile,
                                 std::string configFolder) :
  m_w(1200), m_h(800),
  m_mainFrame(0),
  m_menuFile(0),
  m_tabFrame(0),
  m_currentHistogramFile(0),
  m_currentEventFile(0),
  m_currentHistogramFileName(""),
  m_currentEventFileName(""),
  m_datasetFrameHistogramFile(0),
  m_datasetFrameEventFile(0),
  m_datasetLabelHistogramFile(0),
  m_datasetLabelEventFile(0),
  m_ecClusters(0),
  m_ecTracks(0),
  m_ecConfig(0) {
 
  int qq;
  gVirtualX->GetWindowSize(gVirtualX->GetDefaultRootWindow(), qq, qq, m_w, m_h);
	
  // Set the default paths to the histograms.
  m_defaultPath = "";

  // Creat the main frame
  // const TGWindow* p = gClient->GetRoot();
  m_mainFrame = new TGMainFrame(gClient->GetRoot(), m_w, m_h);

  // Create the top (horizontal) frame
  TGHorizontalFrame* topMenuFrame = new TGHorizontalFrame(m_mainFrame, 
                                                          m_w, 50, kFixedWidth);

  // Set the icon and logo
  const bool showLogo = false;
  if (showLogo) {
    const std::string logofilename = "velo-logo.jpg";
    m_mainFrame->SetIconPixmap(logofilename.c_str());
    m_mainFrame->SetIconName("Testbeam");
    TGImageMap* logo = new TGImageMap(topMenuFrame, logofilename.c_str());
    topMenuFrame->AddFrame(logo, new TGLayoutHints(kLHintsTop | kLHintsRight, 10, 0, 0, 0));
  }

  setROOTColourPalette();
  setROOTStyle();

  // Create the menu bar
  m_menuFile = new TGPopupMenu(gClient->GetRoot());
  m_menuFile->AddEntry("Open Histogram File", M_FILE_HISTO_OPEN);
  m_menuFile->AddEntry("Close Histogram File", M_FILE_HISTO_CLOSE); 
  m_menuFile->AddSeparator();
  m_menuFile->AddEntry("Open Event File", M_FILE_EVENT_OPEN);
  m_menuFile->AddEntry("Close Event File", M_FILE_EVENT_CLOSE); 
  m_menuFile->AddSeparator();
  m_menuFile->AddEntry("Load Configuration", M_FILE_LOAD_CONFIG);
  m_menuFile->AddSeparator();
  m_menuFile->AddEntry("Save Current Canvas", M_FILE_SAVE_CANVAS);
  m_menuFile->AddSeparator();
  m_menuFile->AddEntry("Exit", M_FILE_EXIT);
  m_menuFile->Connect("Activated(Int_t)", "TestBeamMoniGUI", this, 
                      "handleMenu(Int_t)");
  m_menuFile->DisableEntry(M_FILE_HISTO_CLOSE);
  m_menuFile->DisableEntry(M_FILE_EVENT_CLOSE);

  TGLayoutHints* menuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 0, 0);
  TGLayoutHints* menuBarLayout = new TGLayoutHints(kLHintsLeft, 0, 0, 0, 5);

  TGMenuBar* menuBar = new TGMenuBar(topMenuFrame, 1, 1, kRaisedFrame);
  menuBar->AddPopup("Options", m_menuFile, menuBarItemLayout);
  topMenuFrame->AddFrame(menuBar, menuBarLayout);
  
  // Frames to specify the files under scrutiny
  m_datasetFrameHistogramFile = new TGGroupFrame(topMenuFrame, "Histograms");
  m_datasetFrameHistogramFile->SetTitlePos(TGGroupFrame::kCenter);
  m_datasetLabelHistogramFile = new TGLabel(m_datasetFrameHistogramFile, "File not yet specified");
  m_datasetLabelHistogramFile->SetTextColor(0x006699);
  m_datasetLabelHistogramFile->SetTextJustify(kTextLeft);
  m_datasetFrameHistogramFile->AddFrame(m_datasetLabelHistogramFile,
                                        new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
                             
  m_datasetFrameEventFile = new TGGroupFrame(topMenuFrame, "Amalgamated Events");
  m_datasetFrameEventFile->SetTitlePos(TGGroupFrame::kCenter);
  m_datasetLabelEventFile = new TGLabel(m_datasetFrameEventFile, "File not yet specified");
  m_datasetLabelEventFile->SetTextColor(0x006699);
  m_datasetLabelEventFile->SetTextJustify(kTextLeft);
  m_datasetFrameEventFile->AddFrame(m_datasetLabelEventFile,
                                    new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));

  m_datasetFrameAlignmentFile = new TGGroupFrame(topMenuFrame, "Alignment");
  m_datasetFrameAlignmentFile->SetTitlePos(TGGroupFrame::kCenter);
  m_datasetLabelAlignmentFile = new TGLabel(m_datasetFrameAlignmentFile, "File not yet specified");
  m_datasetLabelAlignmentFile->SetTextColor(0x006699);
  m_datasetLabelAlignmentFile->SetTextJustify(kTextLeft);
  m_datasetFrameAlignmentFile->AddFrame(m_datasetLabelAlignmentFile,
                                        new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
                           
  m_datasetFrameConfigFile = new TGGroupFrame(topMenuFrame, "Configuration");
  m_datasetFrameConfigFile->SetTitlePos(TGGroupFrame::kCenter);
  m_datasetLabelConfigFile = new TGLabel(m_datasetFrameConfigFile, "Folder not yet specified");
  m_datasetLabelConfigFile->SetTextColor(0x006699);
  m_datasetLabelConfigFile->SetTextJustify(kTextLeft);
  m_datasetFrameConfigFile->AddFrame(m_datasetLabelConfigFile,
                                    new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
  
  topMenuFrame->AddFrame(m_datasetFrameHistogramFile,
                         new TGLayoutHints(kLHintsCenterX, 0, 0, 0, 0));
  topMenuFrame->AddFrame(m_datasetFrameEventFile,
                         new TGLayoutHints(kLHintsCenterX, 0, 0, 0, 0));
  topMenuFrame->AddFrame(m_datasetFrameAlignmentFile,
                         new TGLayoutHints(kLHintsCenterX, 0, 0, 0, 0));
  topMenuFrame->AddFrame(m_datasetFrameConfigFile,
                         new TGLayoutHints(kLHintsCenterX, 0, 0, 0, 0));

  m_currentHistogramFile = 0;
  m_currentHistogramFileName = "";
  if (histogramFile != "") {
    if (openHistogramFile(histogramFile.c_str())) {
      std::string text = m_currentHistogramFileName.substr(m_currentHistogramFileName.find_last_of("/\\") + 1);
      m_datasetLabelHistogramFile->SetTextColor(0x006699);
      m_datasetLabelHistogramFile->SetText(text.c_str());
      m_datasetFrameHistogramFile->SetTitlePos(TGGroupFrame::kCenter);
      m_datasetFrameHistogramFile->Resize(m_datasetFrameHistogramFile->GetDefaultSize());
      m_menuFile->EnableEntry(M_FILE_HISTO_CLOSE);
    }
  }
  m_currentEventFile = 0;
  m_currentEventFileName = "";
  if (eventFile != "") {
    if (openEventFile(eventFile.c_str())) {
      std::string text = m_currentEventFileName.substr(m_currentEventFileName.find_last_of("/\\") + 1);
      m_datasetLabelEventFile->SetTextColor(0x006699);
      m_datasetLabelEventFile->SetText(text.c_str());
      m_datasetFrameEventFile->SetTitlePos(TGGroupFrame::kCenter);
      m_datasetFrameEventFile->Resize(m_datasetFrameEventFile->GetDefaultSize());
      m_menuFile->EnableEntry(M_FILE_EVENT_CLOSE);
    }
  }

  m_currentAlignmentFileName = "";  
  if (conditionsFile != "") {
    if (!loadAlignmentFile(conditionsFile)) {
      std::cerr << "Could not load alignment from " << conditionsFile << "\n";
    } else {
      std::string text = m_currentAlignmentFileName.substr(m_currentAlignmentFileName.find_last_of("/\\") + 1);
      m_datasetLabelAlignmentFile->SetTextColor(0x006699);
      m_datasetLabelAlignmentFile->SetText(text.c_str());
      m_datasetFrameAlignmentFile->SetTitlePos(TGGroupFrame::kCenter);
      m_datasetFrameAlignmentFile->Resize(m_datasetFrameAlignmentFile->GetDefaultSize());
    }
  }

  m_currentConfigFolder = "";  
  if (configFolder != "") {
    if (!loadConfigurationData(configFolder)) {
      std::cerr << "Could not load configuration data from " << configFolder << "\n";
    } else {
      m_datasetLabelConfigFile->SetTextColor(0x006699);
      m_datasetLabelConfigFile->SetText(m_currentConfigFolder.c_str());
      m_datasetFrameConfigFile->SetTitlePos(TGGroupFrame::kCenter);
      m_datasetFrameConfigFile->Resize(m_datasetFrameConfigFile->GetDefaultSize());
    }
  }
  
  // Definition of the tab menus
  m_tabFrame = new TGTab(m_mainFrame, m_w, m_h - 250);
  m_tabFrame->Connect("Selected(Int_t)", "TestBeamMoniGUI", this, 
                      "update()");
  TGCompositeFrame* frame = m_tabFrame->AddTab("Clusters");
  contentsClusters(frame);
  frame = m_tabFrame->AddTab("Tracks");
  contentsTracks(frame);
  frame = m_tabFrame->AddTab("Configuration");
  contentsConfig(frame);
  frame = m_tabFrame->AddTab("Amalgamation");
  contentsAmalgamation(frame);
  
  // Add the GUI horizontal and tab frames to the main frame
  TGLayoutHints* topMenuLayout =
    new TGLayoutHints(kLHintsCenterX, 10, 10, 10, 0);
  TGLayoutHints* tabFrameLayout = 
    new TGLayoutHints(kLHintsCenterY | kLHintsExpandX | kLHintsExpandY, 10, 10, 0, 10);
  m_mainFrame->AddFrame(topMenuFrame, topMenuLayout);
  m_mainFrame->AddFrame(m_tabFrame, tabFrameLayout);
  
  m_mainFrame->SetWindowName( "Timepix Testbeam GUI");
  m_mainFrame->MapSubwindows();
  m_mainFrame->MapWindow();
  m_mainFrame->SetWMSize(m_w, m_h);
  m_mainFrame->SetWMSizeHints(0, 0, m_w, m_h, 0, 0);
  m_mainFrame->Resize(m_mainFrame->GetDefaultSize());

}

//=============================================================================
// Destructor
//=============================================================================
TestBeamMoniGUI::~TestBeamMoniGUI() {

  m_mainFrame->Cleanup();
  delete m_mainFrame;
  m_mainFrame = 0;

}

//=============================================================================
// Handle the functionality associated to the menu bar items
//=============================================================================
void TestBeamMoniGUI::handleMenu(int id) {

  std::string command = "";
  switch (id) {

    case M_FILE_HISTO_OPEN: 
      // Bring the pop-up window to open a file
      if (buttonOpenHistogramFile()) {
        // Update the label and resize to see it happening
        std::string text = m_currentHistogramFileName.substr(m_currentHistogramFileName.find_last_of("/\\") + 1);
        m_datasetLabelHistogramFile->SetTextColor(0x006699);
        m_datasetLabelHistogramFile->SetText(text.c_str());
        m_datasetFrameHistogramFile->SetTitlePos(TGGroupFrame::kCenter);
        m_datasetFrameHistogramFile->Resize(m_datasetFrameHistogramFile->GetDefaultSize());
        m_mainFrame->Resize();
        // Enable the "Close file" entry in the "File" menu
        m_menuFile->EnableEntry(M_FILE_HISTO_CLOSE);
      }
      break;
						
    case M_FILE_EVENT_OPEN:
      // Bring the pop-up window to open a file
      if (buttonOpenEventFile()) {
        // Update the label and resize to see it happening
        std::string text = m_currentEventFileName.substr(m_currentEventFileName.find_last_of("/\\") + 1);
        m_datasetLabelEventFile->SetTextColor(0x006699);
        m_datasetLabelEventFile->SetText(text.c_str());
        m_datasetFrameEventFile->SetTitlePos(TGGroupFrame::kCenter);
        m_datasetFrameEventFile->Resize(m_datasetFrameEventFile->GetDefaultSize());
        m_mainFrame->Resize();
        // Enable the "Close file" entry in the "File" menu
        m_menuFile->EnableEntry(M_FILE_EVENT_CLOSE);
      }
      break;

    case M_FILE_HISTO_CLOSE:
      // Close the file currently open
      if (closeHistogramFile()) {
        // Update the label
        m_datasetLabelHistogramFile->SetText("No file currently open");
        m_datasetLabelHistogramFile->SetTextColor(0x006699);
        m_datasetFrameHistogramFile->SetTitlePos(TGGroupFrame::kCenter);
        m_datasetFrameHistogramFile->Resize(m_datasetFrameHistogramFile->GetDefaultSize());
        m_mainFrame->Resize();
        // Disable the "Close file" entry in the "File" menu
        m_menuFile->DisableEntry(M_FILE_HISTO_CLOSE);
      }
      break;    

    case M_FILE_EVENT_CLOSE:
      // Close the file currently open
      if (closeEventFile()) {
        // Update the label
        m_datasetLabelEventFile->SetText("No file currently open");
        m_datasetLabelEventFile->SetTextColor(0x006699);
        m_datasetFrameEventFile->SetTitlePos(TGGroupFrame::kCenter);
        m_datasetFrameEventFile->Resize(m_datasetFrameEventFile->GetDefaultSize());
        m_mainFrame->Resize();
        // Disable the "Close file" entry in the "File" menu
        m_menuFile->DisableEntry(M_FILE_EVENT_CLOSE);
      }
      break;
      
    case M_FILE_LOAD_CONFIG:
      if (buttonOpenConfigFolder()) {
        std::cout << "Successfully loaded configuration data." << std::endl;
        m_datasetLabelConfigFile->SetTextColor(0x006699);
        m_datasetLabelConfigFile->SetText(m_currentConfigFolder.c_str());
        m_datasetFrameConfigFile->SetTitlePos(TGGroupFrame::kCenter);
        m_datasetFrameConfigFile->Resize(m_datasetFrameConfigFile->GetDefaultSize());
        m_mainFrame->Resize();
      }
      break;
      
    case M_FILE_SAVE_CANVAS:
      if (!saveCanvas()) {
        std::cerr << "Could not save current canvas." << std::endl;
      }
      break;
      
    case M_FILE_EXIT :
      // Terminate the GUI. Close everything.
      gApplication->Terminate(0);
      break;

    default:
      std::cerr << "ERROR: default case. Seems like menu item "
                << id << " has not been implemented!\n";
      break;
  }

}

//=============================================================================
// Pop out the "Open ROOT file" dialog window
//=============================================================================
bool TestBeamMoniGUI::buttonOpenHistogramFile() {

  static TString dir(m_defaultPath.c_str());
  const char* filetypes[] = {"ROOT files", "*.root", 0, 0};

  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  fi.fIniDir = StrDup(dir);
  new TGFileDialog(gClient->GetRoot(), m_mainFrame, kFDOpen, &fi);

  if (fi.fFilename) {
    // Close the file currently open, if any
    closeHistogramFile();
    // Actually open the file
    return openHistogramFile(fi.fFilename);
  }
  return false;

}

bool TestBeamMoniGUI::buttonOpenEventFile() {

  static TString dir(m_defaultPath.c_str());
  const char* filetypes[] = {"ROOT files", "*.root", 0, 0};

  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  fi.fIniDir = StrDup(dir);
  new TGFileDialog(gClient->GetRoot(), m_mainFrame, kFDOpen, &fi);

  if (fi.fFilename) {
    // Close the file currently open, if any
    closeEventFile();
    // Actually open the file
    return openEventFile(fi.fFilename);
  }
  return false;

}

bool TestBeamMoniGUI::buttonOpenConfigFolder() {

  static TString dir(m_defaultPath.c_str());
  const char* filetypes[] = {"Directories", "*", 0, 0};

  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  fi.fIniDir = StrDup(dir);
  new TGFileDialog(gClient->GetRoot(), m_mainFrame, kFDOpen, &fi);

  if (fi.fFilename) {
    return loadConfigurationData(fi.fFilename);
  }
  return false;

}

//=============================================================================
// Open ROOT files
//=============================================================================
bool TestBeamMoniGUI::openHistogramFile(const char* fileName) {

  m_currentHistogramFile = new TFile(fileName);
  if (!m_currentHistogramFile->IsZombie()) {
    std::cout << "New histogram file opened: " << fileName << std::endl;
    m_currentHistogramFileName = fileName;
    return true;
  }
  std::cerr << "Histogram file " << fileName << " not found!" << std::endl;
  m_currentHistogramFileName = "";
  m_currentHistogramFile = 0;
  return false;
  
}

bool TestBeamMoniGUI::openEventFile(const char* fileName) {

  m_currentEventFile = new TFile(fileName);
  if (!m_currentEventFile->IsZombie()) {
    std::cout << "New event file opened: " << fileName << std::endl;
    m_currentEventFileName = fileName;
    return true;
  }
  std::cerr << "Event file " << fileName << " not found!" << std::endl;
  m_currentEventFileName = "";
  m_currentEventFile = 0;
  return false;
  
}


//=============================================================================
// Close the ROOT file currently open
//=============================================================================
bool TestBeamMoniGUI::closeHistogramFile() {

  if (m_currentHistogramFile != 0) {
    m_currentHistogramFile->Close();
    m_currentHistogramFile = 0;
    m_currentHistogramFileName = "";
    return true;
  }
  return false;

}

bool TestBeamMoniGUI::closeEventFile() {

  if (m_currentEventFile != 0) {
    m_currentEventFile->Close();
    m_currentEventFile = 0;
    m_currentEventFileName = "";
    return true;
  }
  return false;

}

bool TestBeamMoniGUI::loadConfigurationData(std::string path) {

  configs.clear();
  if (path.size() < 1) return false;
  if (path[path.size() - 1] != '/') path.append("/");
  std::string filename = path + "DetectorNames.txt";
  std::ifstream infile;
  infile.open(filename.c_str(), std::ios::in);
  if (!infile.is_open()) {
    std::cerr << "Could not open " << filename << std::endl;
    m_currentConfigFolder = "";
    return false;
  }
  m_currentConfigFolder = path;
  
  std::vector<int> relaxds;
  std::cout << "Chip    Relaxd ID    Chip Position\n";
  while (!infile.eof()) {
    std::string chipId = "";
    int relaxdId = 0;
    int chipPos = 0;
    infile >> chipId >> relaxdId >> chipPos;
    if (infile.eof()) break;
    chipConfig newConfig;
    newConfig.chipId = chipId;
    newConfig.relaxdId = relaxdId;
    newConfig.chipPos = chipPos;
    newConfig.startValue = 0;
    newConfig.startValue = 0;
    newConfig.nSteps = 0;
    configs.push_back(newConfig);
    bool exists = false;
    for (unsigned int i = relaxds.size(); i--;) {
      if (relaxds[i] == relaxdId) {
        exists = true;
        break;
      }
    }
    if (!exists) relaxds.push_back(relaxdId);
    std::cout << chipId << "    " << relaxdId << "    " << chipPos << "\n";
  }
  infile.close();
  
  for (unsigned int i = 0; i < relaxds.size(); ++i) {
    filename = path + "distributions_relaxd" + toString(relaxds[i]) + ".txt";
    FILE* ifile = fopen(filename.c_str(), "r");
    if (!ifile) {
      std::cerr << "File " << filename << " not found." << std::endl;
      continue;
    } 
    // Read distributions    
    char tmpString[100];
    fgets(tmpString, 100, ifile);
    std::cout << tmpString;
    fgets(tmpString, 100, ifile);
    std::cout << tmpString;
    int startValue = 0;
    int endValue = 0;
    int step = 1;
    fscanf(ifile, "%s %d", &tmpString[0], &startValue);
    fscanf(ifile, "%s %d", &tmpString[0], &endValue);
    fscanf(ifile, "%s %d", &tmpString[0], &step);
    if (step != 0) {
      const int nSteps = (endValue - startValue) / step;
      while (!feof(ifile)) {
        int chip = -1;
        fscanf(ifile, "%s %d", &tmpString[0], &chip);
        if (chip < 0) break;
        fgets(tmpString, 100, ifile);
        fgets(tmpString, 100, ifile);
        std::vector<std::vector<int> > entries;
        entries.resize(nSteps);
        for (int j = 0; j < nSteps; ++j) {
          entries[j].resize(3);
          fscanf(ifile, "%d %d %d", &entries[j][0], &entries[j][1], &entries[j][2]);
        }
        for (unsigned int j = configs.size(); j--;) {
          if (configs[j].relaxdId == relaxds[i] &&
              configs[j].chipPos == chip) {
            configs[j].distributions = entries;
            configs[j].startValue = startValue;
            configs[j].endValue = endValue;
            configs[j].nSteps = nSteps;
            break;
          }
        }
      }
    }
    fclose(ifile);
  }
  
  for (unsigned int i = 0; i < relaxds.size(); ++i) {
    filename = path + "THLadjust_relaxd" + toString(relaxds[i]) + ".txt.dacs";
    FILE* idac = fopen(filename.c_str(), "r");
    if (!idac) {
      std::cerr << "File " << filename << " not found." << std::endl;
      continue;
    }
    char tmpString[100];
    int nChips = -1;
    while (!feof(idac)) {
      ++nChips;
      fgets(tmpString, 100, idac);
      for (int j = 0; j < 14; ++j) fgets(tmpString, 100, idac);
    }
    fclose(idac);
    if (nChips <= 0) continue;
    std::cout << "RelaxD" << relaxds[i] << ": " << nChips << " chips\n";
    filename = path + "THLadjust_relaxd" + toString(relaxds[i]) + ".txt";
    FILE* ifile = fopen(filename.c_str(), "r");
    if (!ifile) {
      std::cerr << "File " << filename << " not found." << std::endl;
      continue;
    }
    std::vector<std::vector<int> > adjBits;
    adjBits.resize(nChips);
    const int nBits = 16;
    for (int j = 0; j < nChips; ++j) {
      adjBits[j].resize(nBits);
      for (int k = 0; k < nBits; ++k) {
        adjBits[j][k] = 0;
      }
    }
    const int nRows = 256;
    const int nCols = 256;
    int value;
    if (nChips == 1) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[0][value] += 1;
        }
      }
    } else if (nChips == 2) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[0][value] += 1;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[1][value] += 1;
        }
      }
    } else if (nChips == 3) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[0][value] += 1;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[1][value] += 1;
        }
      }
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[2][value] += 1;
        }
      }
    } else if (nChips == 4) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[0][value] += 1;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[1][value] += 1;
        }
      }
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[2][value] += 1;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          adjBits[3][value] += 1;
        }
      }
    }
    fclose(ifile);
    for (unsigned int j = configs.size(); j--;) {
      for (int k = 0; k < nChips; ++k) {
        if (configs[j].relaxdId == relaxds[i] &&
            configs[j].chipPos == k) {
          configs[j].bits = adjBits[k];
          break;
        }
      }
    }
  }

  for (unsigned int i = 0; i < relaxds.size(); ++i) {
    filename = path + "masked_relaxd" + toString(relaxds[i]) + ".txt.dacs";
    FILE* idac = fopen(filename.c_str(), "r");
    if (!idac) {
      std::cerr << "File " << filename << " not found." << std::endl;
      continue;
    }
    char tmpString[100];
    int nChips = -1;
    while (!feof(idac)) {
      ++nChips;
      fgets(tmpString, 100, idac);
      for (int j = 0; j < 14; ++j) fgets(tmpString, 100, idac);
    }
    fclose(idac);
    if (nChips <= 0) continue;
    std::cout << "RelaxD" << relaxds[i] << ": " << nChips << " chips\n";
    filename = path + "masked_relaxd" + toString(relaxds[i]) + ".txt";
    FILE* ifile = fopen(filename.c_str(), "r");
    if (!ifile) {
      std::cerr << "File " << filename << " not found." << std::endl;
      continue;
    }
    std::vector<std::vector<std::vector<bool> > > maskedPixels;
    maskedPixels.resize(nChips);
    const int nRows = 256;
    const int nCols = 256;
    for (int j = 0; j < nChips; ++j) {
      maskedPixels[j].resize(nRows);
      for (int k = 0; k < nRows; ++k) {
        maskedPixels[j][k].resize(nCols);
        for (int l = 0; l < nCols; ++l) {
          maskedPixels[j][k][l] = false;
        }
      }
    }
    int value;
    if (nChips == 1) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[0][j][k] = true;
        }
      }
    } else if (nChips == 2) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[0][j][k] = true;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[1][j][k] = true;
        }
      }
    } else if (nChips == 3) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[0][j][k] = true;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[1][j][k] = true;
        }
      }
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[2][j][k] = true;
        }
      }
    } else if (nChips == 4) {
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[0][j][k] = true;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[1][j][k] = true;
        }
      }
      for (int j = 0; j < nRows; ++j) {
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[2][j][k] = true;
        }
        for (int k = 0; k < nCols; ++k) {
          fscanf(ifile, "%i", &value);
          if (value == 0) maskedPixels[3][j][k] = true;
        }
      }
    }
    fclose(ifile);
    for (unsigned int j = configs.size(); j--;) {
      for (int k = 0; k < nChips; ++k) {
        if (configs[j].relaxdId == relaxds[i] &&
            configs[j].chipPos == k) {
          configs[j].masked = maskedPixels[k];
          break;
        }
      }
    }
  }
  return true;
  
}

bool TestBeamMoniGUI::loadAlignmentFile(const std::string filename) {

  alignments.clear();
  std::ifstream infile;
  infile.open(filename.c_str(), std::ios::in);
  if (!infile.is_open()) {
    std::cerr << "Could not open " << filename << std::endl;
    m_currentAlignmentFileName = "";
    return false;
  }
  m_currentAlignmentFileName = filename;
  
  std::cout << "Chip    z [mm]\n";
  std::string line;
  while (std::getline(infile, line)) {
    if (line.length() < 1) continue;
    if (line[0] == '#') continue;
    std::stringstream streamline(line);
    std::string chipId = "";
    double x = 0., y = 0., z = 0.;
    double rx = 0., ry = 0., rz = 0.;
    streamline >> chipId >> x >> y >> z >> rx >> ry >> rz;
    alignmentParameters newAlignment;
    newAlignment.chipId = chipId;
    newAlignment.z = z;
    alignments.push_back(newAlignment);
    std::cout << chipId << "    " << z << "\n";
  }
  infile.close();
  return true;
  
}

bool TestBeamMoniGUI::saveCanvas() {

  std::string plotName = "";
  TCanvas* currentCanvas = 0;
  const int iTab = m_tabFrame->GetCurrent();
  switch (iTab) {
    case M_TAB_CLUSTERS:
      currentCanvas = m_ecClusters->GetCanvas();
      switch (m_clusters_currentCanvasButton) {
        case M_CLUSTER_HITS_PER_FRAME:
          plotName = "HitsPerFrame";
          break;
        case M_CLUSTER_DIFFERENCES_X:
          plotName = "ClusterDifferencesX";
          break;
        case M_CLUSTER_DIFFERENCES_Y:
          plotName = "ClusterDifferencesY";
          break;
        case M_CLUSTER_CORRELATIONS_X:
          plotName = "ClusterCorrelationsX";
          break;
        case M_CLUSTER_CORRELATIONS_Y:
          plotName = "ClusterCorrelationsY";
          break;
        case M_CLUSTER_SIZE:
          plotName = "ClusterSizeDistributions";
          break;
        case M_CLUSTER_TOT_SPECTRUM:
          plotName = "TOTSpectra";
          break;
        case M_CLUSTER_LOCAL_POS:
          plotName = "ClusterLocalPositions";
          break;
        case M_CLUSTER_GLOBAL_POS:
          plotName = "ClusterGlobalPositions";
          break;          
      }
      break;
    case M_TAB_TRACKS:
      currentCanvas = m_ecTracks->GetCanvas();
      switch (m_tracks_currentCanvasButton) {
        case M_TRACK_PROTO_TRACKS:
          plotName = "ProtoTracks";
          break;
        case M_TRACK_FIT:
          plotName = "TrackFit";
          break;
        case M_TRACK_RESIDUALS_X:
          plotName = "ResidualsX";
          break;
        case M_TRACK_RESIDUALS_Y:
          plotName = "ResidualsY";
          break;
        case M_TRACK_RESIDUALS_ASSOCIATED_X:
          plotName = "ResidualsAssociatedX";
          break;
        case M_TRACK_RESIDUALS_ASSOCIATED_Y:
          plotName = "ResidualsAssociatedY";
          break;
        case M_TRACK_INTERCEPTS_LOCAL:
          plotName = "TrackInterceptsLocal";
          break;
        case M_TRACK_INTERCEPTS_GLOBAL:
          plotName = "TrackInterceptsGlobal";
          break;
      }
      break;
    case M_TAB_CONFIG:
      currentCanvas = m_ecConfig->GetCanvas();
      switch (m_config_currentCanvasButton) {
        case M_CONFIG_THL_DIST:
          plotName = "Equalization";
          break;
        case M_CONFIG_ADJUSTMENT:
          plotName = "AdjustmentBits";
          break;
        case M_CONFIG_MASKED:
          plotName = "MaskedPixels";
          break;
      }
      break;
    case M_TAB_AMALGAMATION:
      currentCanvas = m_ecAmalgamation->GetCanvas();
      switch (m_amalg_currentCanvasButton) {
        case M_AMALG_TDC:
          plotName = "TDC";
          break;
      }
      break;
    default:
      return false;
      break;
  }
  
  if (plotName == "" || !currentCanvas) return false;
  plotName += ".pdf";
  currentCanvas->SaveAs(plotName.c_str());
  return true;
  
}

//=============================================================================
// Update the currently selected tab
//=============================================================================
void TestBeamMoniGUI::update() {

  const int iTab = m_tabFrame->GetCurrent();
  switch (iTab) {
    case M_TAB_CLUSTERS:
      updateTabClustersCanvas();
      break;
    case M_TAB_TRACKS:
      updateTabTracksCanvas();
      break;
    case M_TAB_CONFIG:
      updateTabConfigCanvas();
      break;
    case M_TAB_AMALGAMATION:
      updateTabAmalgamationCanvas();
      break;
    default:
      break;
  }

}

//=============================================================================
// Describe the contents of the tab "Clusters"
//=============================================================================
void TestBeamMoniGUI::contentsClusters(TGCompositeFrame* frame) {

  TGHorizontalFrame* localFrame = new TGHorizontalFrame(frame, m_w, 100, kFixedWidth);
  TGLayoutHints* layout = new TGLayoutHints(kLHintsLeft, 10, 10, 10, 1);

  // Hits per frame
  TGTextButton* btnHitsPerFrame = new TGTextButton(localFrame, "Hits / Frame", 0);
  btnHitsPerFrame->SetToolTipText("Number of hits per frame", 250);
  btnHitsPerFrame->Connect("Clicked()", "TestBeamMoniGUI", this, 
                           "handle_txtBtnHitsPerFrame()");
  localFrame->AddFrame(btnHitsPerFrame, layout);  
  
  // Global cluster differences
  TGTextButton* btnClusterDiffX = new TGTextButton(localFrame, "Differences X", 0);
  btnClusterDiffX->SetToolTipText("Global cluster differences", 250);
  btnClusterDiffX->Connect("Clicked()", "TestBeamMoniGUI", this, 
                           "handle_txtBtnClusterDifferencesX()");
  localFrame->AddFrame(btnClusterDiffX, layout);  
  TGTextButton* btnClusterDiffY = new TGTextButton(localFrame, "Differences Y", 0);
  btnClusterDiffY->SetToolTipText("Global cluster differences", 250);
  btnClusterDiffY->Connect("Clicked()", "TestBeamMoniGUI", this, 
                           "handle_txtBtnClusterDifferencesY()");
  localFrame->AddFrame(btnClusterDiffY, layout);  

  // Cluster correlations
  TGTextButton* btnClusterCorrelationsX = new TGTextButton(localFrame, "Correlations X", 0);
  btnClusterCorrelationsX->SetToolTipText("Cluster correlations", 250);
  btnClusterCorrelationsX->Connect("Clicked()", "TestBeamMoniGUI", this, 
                                   "handle_txtBtnClusterCorrelationsX()");
  localFrame->AddFrame(btnClusterCorrelationsX, layout);  
  TGTextButton* btnClusterCorrelationsY = new TGTextButton(localFrame, "Correlations Y", 0);
  btnClusterCorrelationsY->SetToolTipText("Cluster correlations", 250);
  btnClusterCorrelationsY->Connect("Clicked()", "TestBeamMoniGUI", this, 
                                   "handle_txtBtnClusterCorrelationsY()");
  localFrame->AddFrame(btnClusterCorrelationsY, layout);  

	  
  // Cluster size distribution
  TGTextButton* btnClusterSize = new TGTextButton(localFrame, "Cluster Size", 0);
  btnClusterSize->SetToolTipText("Cluster size distribution", 250);
  btnClusterSize->Connect("Clicked()", "TestBeamMoniGUI", this, 
                          "handle_txtBtnClusterSize()");
  localFrame->AddFrame(btnClusterSize, layout);  

  // TOT spectra
  TGTextButton* btnTOT = new TGTextButton(localFrame, "ToT Spectrum", 0);
  btnTOT->SetToolTipText("ToT distribution", 250);
  btnTOT->Connect("Clicked()", "TestBeamMoniGUI", this, 
                  "handle_txtBtnTOTSpectrum()");
  localFrame->AddFrame(btnTOT, layout);  

  // Local cluster positions
  TGTextButton* btnLocalPos = new TGTextButton(localFrame, "Local Positions", 0);
  btnLocalPos->SetToolTipText("Local cluster positions", 250);
  btnLocalPos->Connect("Clicked()", "TestBeamMoniGUI", this, 
                  "handle_txtBtnLocalClusterPositions()");
  localFrame->AddFrame(btnLocalPos, layout);  

  // Global cluster positions
  TGTextButton* btnGlobalPos = new TGTextButton(localFrame, "Global Positions", 0);
  btnGlobalPos->SetToolTipText("Global cluster positions", 250);
  btnGlobalPos->Connect("Clicked()", "TestBeamMoniGUI", this, 
                  "handle_txtBtnGlobalClusterPositions()");
  localFrame->AddFrame(btnGlobalPos, layout);  
      
  frame->AddFrame(localFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

  m_clusters_currentCanvasButton = M_CLUSTER_HITS_PER_FRAME;
 	
  // Canvas on which to draw the plots
  TGCompositeFrame* bottomFrame = new TGCompositeFrame(frame, 60, 20, kVerticalFrame);
  m_ecClusters = new TRootEmbeddedCanvas("ecClusters", bottomFrame, m_w - 100, m_h - 300);
  m_ecClusters->GetCanvas()->SetFillColor(0);
  m_ecClusters->GetCanvas()->SetBorderMode(0);
  TGLayoutHints* canvasLayout = new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10);
  bottomFrame->AddFrame(m_ecClusters, canvasLayout);
  TGLayoutHints* bottomFrameLayout = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0);
  frame->AddFrame(bottomFrame, bottomFrameLayout);
  
}

//=============================================================================
// Describe the contents of the tab "Tracks"
//=============================================================================
void TestBeamMoniGUI::contentsTracks(TGCompositeFrame* frame) {

  TGHorizontalFrame* localFrame = new TGHorizontalFrame(frame, m_w, 100, kFixedWidth);
  TGLayoutHints* layout = new TGLayoutHints(kLHintsLeft, 10, 10, 10, 1);

  // Proto-tracks
  TGTextButton* btnProtoTracks = new TGTextButton(localFrame, "Proto-Tracks", 0);
  btnProtoTracks->SetToolTipText("Proto-tracks", 250);
  btnProtoTracks->Connect("Clicked()", "TestBeamMoniGUI", this, 
                          "handle_txtBtnProtoTracks()");
  localFrame->AddFrame(btnProtoTracks, layout);  

  // Track fit
  TGTextButton* btnTrackFit = new TGTextButton(localFrame, "Track Fit", 0);
  btnTrackFit->SetToolTipText("Track fit", 250);
  btnTrackFit->Connect("Clicked()", "TestBeamMoniGUI", this, 
                       "handle_txtBtnTrackFit()");
  localFrame->AddFrame(btnTrackFit, layout);  

  // Residuals
  TGTextButton* btnResidualsX = new TGTextButton(localFrame, "Residuals X", 0);
  btnResidualsX->SetToolTipText("Track residuals", 250);
  btnResidualsX->Connect("Clicked()", "TestBeamMoniGUI", this, 
                         "handle_txtBtnResidualsX()");
  localFrame->AddFrame(btnResidualsX, layout);  
  TGTextButton* btnResidualsY = new TGTextButton(localFrame, "Residuals Y", 0);
  btnResidualsY->SetToolTipText("Track residuals", 250);
  btnResidualsY->Connect("Clicked()", "TestBeamMoniGUI", this, 
                         "handle_txtBtnResidualsY()");
  localFrame->AddFrame(btnResidualsY, layout);  
  TGTextButton* btnResAssocX = new TGTextButton(localFrame, 
                                                "Residuals X (assoc.)", 0);
  btnResAssocX->SetToolTipText("Track residuals (associated clusters)", 250);
  btnResAssocX->Connect("Clicked()", "TestBeamMoniGUI", this, 
                        "handle_txtBtnResidualsAssociatedX()");
  localFrame->AddFrame(btnResAssocX, layout);  
  TGTextButton* btnResAssocY = new TGTextButton(localFrame, 
                                                "Residuals Y (assoc.)", 0);
  btnResAssocY->SetToolTipText("Track residuals (associated clusters)", 250);
  btnResAssocY->Connect("Clicked()", "TestBeamMoniGUI", this, 
                        "handle_txtBtnResidualsAssociatedY()");
  localFrame->AddFrame(btnResAssocY, layout);  
  TGTextButton* btnInterceptsLocal = new TGTextButton(localFrame, 
                                                      "Local intercepts", 0);
  btnInterceptsLocal->SetToolTipText("Track intercepts (local coordinates)", 250);
  btnInterceptsLocal->Connect("Clicked()", "TestBeamMoniGUI", this, 
                              "handle_txtBtnInterceptsLocal()");
  localFrame->AddFrame(btnInterceptsLocal, layout);  
  TGTextButton* btnInterceptsGlobal = new TGTextButton(localFrame, 
                                                      "Global intercepts", 0);
  btnInterceptsGlobal->SetToolTipText("Track intercepts (local coordinates)", 250);
  btnInterceptsGlobal->Connect("Clicked()", "TestBeamMoniGUI", this, 
                               "handle_txtBtnInterceptsGlobal()");
  localFrame->AddFrame(btnInterceptsGlobal, layout);  

  TGTextButton* btnInterceptsTimestamped = new TGTextButton(localFrame, 
                                                            "Intercepts timestamped", 0);
  btnInterceptsTimestamped->SetToolTipText("Track intercepts (timestamped tracks)", 250);
  btnInterceptsTimestamped->Connect("Clicked()", "TestBeamMoniGUI", this, 
                                    "handle_txtBtnInterceptsTimestampYes()");
  localFrame->AddFrame(btnInterceptsTimestamped, layout);  

  TGTextButton* btnInterceptsNonTimestamped = new TGTextButton(localFrame, 
                                                               "Intercepts non-timestamped", 0);
  btnInterceptsNonTimestamped->SetToolTipText("Track intercepts (non-timestamped tracks)", 250);
  btnInterceptsNonTimestamped->Connect("Clicked()", "TestBeamMoniGUI", this, 
                                       "handle_txtBtnInterceptsTimestampNo()");
  localFrame->AddFrame(btnInterceptsNonTimestamped, layout);  

  frame->AddFrame(localFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

  m_tracks_currentCanvasButton = M_TRACK_PROTO_TRACKS;
 	
  // Canvas on which to draw the plots
  TGCompositeFrame* bottomFrame = new TGCompositeFrame(frame, 60, 20, kVerticalFrame);
  m_ecTracks = new TRootEmbeddedCanvas("ecTracks", bottomFrame, m_w - 100, m_h - 300);
  m_ecTracks->GetCanvas()->SetFillColor(0);
  m_ecTracks->GetCanvas()->SetBorderMode(0);
  TGLayoutHints* canvasLayout = new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10);
  bottomFrame->AddFrame(m_ecTracks, canvasLayout);
  TGLayoutHints* bottomFrameLayout = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0);
  frame->AddFrame(bottomFrame, bottomFrameLayout);
  
}

//=============================================================================
// Describe the contents of the tab "Configuration"
//=============================================================================
void TestBeamMoniGUI::contentsConfig(TGCompositeFrame* frame) {

  TGHorizontalFrame* localFrame = new TGHorizontalFrame(frame, m_w, 100, kFixedWidth);
  TGLayoutHints* layout = new TGLayoutHints(kLHintsLeft, 10, 10, 10, 1);

  TGTextButton* btnTHLDistributions = new TGTextButton(localFrame, "Thresholds", 0);
  btnTHLDistributions->SetToolTipText("THL equalization distributions and thresholds", 250);
  btnTHLDistributions->Connect("Clicked()", "TestBeamMoniGUI", this, 
                               "handle_txtBtnTHLDistributions()");
  localFrame->AddFrame(btnTHLDistributions, layout);  
  TGTextButton* btnAdj = new TGTextButton(localFrame, "Adjustment Bits", 0);
  btnAdj->SetToolTipText("Threshold adjustment bits", 250);
  btnAdj->Connect("Clicked()", "TestBeamMoniGUI", this, 
                  "handle_txtBtnAdjBits()");
  localFrame->AddFrame(btnAdj, layout);  
  TGTextButton* btnMasked = new TGTextButton(localFrame, "Masked Pixels", 0);
  btnMasked->SetToolTipText("Masked pixels", 250);
  btnMasked->Connect("Clicked()", "TestBeamMoniGUI", this, 
                     "handle_txtBtnMasked()");
  localFrame->AddFrame(btnMasked, layout);  

  frame->AddFrame(localFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

  m_config_currentCanvasButton = M_CONFIG_THL_DIST;
 	
  // Canvas on which to draw the plots
  TGCompositeFrame* bottomFrame = new TGCompositeFrame(frame, 60, 20, kVerticalFrame);
  m_ecConfig = new TRootEmbeddedCanvas("ecConfig", bottomFrame, m_w - 100, m_h - 300);
  m_ecConfig->GetCanvas()->SetFillColor(0);
  m_ecConfig->GetCanvas()->SetBorderMode(0);
  TGLayoutHints* canvasLayout = new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10);
  bottomFrame->AddFrame(m_ecConfig, canvasLayout);
  TGLayoutHints* bottomFrameLayout = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0);
  frame->AddFrame(bottomFrame, bottomFrameLayout);
  
}

//=============================================================================
// Describe the contents of the tab "Amalgamation"
//=============================================================================
void TestBeamMoniGUI::contentsAmalgamation(TGCompositeFrame* frame) {

  TGHorizontalFrame* localFrame = new TGHorizontalFrame(frame, m_w, 100, kFixedWidth);
  TGLayoutHints* layout = new TGLayoutHints(kLHintsLeft, 10, 10, 10, 1);

  TGTextButton* btnTDC = new TGTextButton(localFrame, "TDC", 0);
  btnTDC->SetToolTipText("TDC plots", 250);
  btnTDC->Connect("Clicked()", "TestBeamMoniGUI", this, 
                               "handle_txtBtnTDC()");
  localFrame->AddFrame(btnTDC, layout);  

  frame->AddFrame(localFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

  m_amalg_currentCanvasButton = M_AMALG_TDC;
 	
  // Canvas on which to draw the plots
  TGCompositeFrame* bottomFrame = new TGCompositeFrame(frame, 60, 20, kVerticalFrame);
  m_ecAmalgamation = new TRootEmbeddedCanvas("ecAmalgamation", bottomFrame, m_w - 100, m_h - 300);
  m_ecAmalgamation->GetCanvas()->SetFillColor(0);
  m_ecAmalgamation->GetCanvas()->SetBorderMode(0);
  TGLayoutHints* canvasLayout = new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10);
  bottomFrame->AddFrame(m_ecAmalgamation, canvasLayout);
  TGLayoutHints* bottomFrameLayout = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0);
  frame->AddFrame(bottomFrame, bottomFrameLayout);
  
}

//=============================================================================
// Handle the buttons of the tab "Clusters"
//=============================================================================
void TestBeamMoniGUI::handle_txtBtnHitsPerFrame() {

  m_clusters_currentCanvasButton = M_CLUSTER_HITS_PER_FRAME;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ClusterMaker");
  if (!gDirectory) return;

  std::vector<TH1D*> h1;
  h1.resize(nPlanes);
  double ymax = 0.;
  for (int i = 0; i < nPlanes; ++i) {
    h1[i] = 0;
    std::string name = "npixelhitsperevent_" + detNames[i];
    h1[i] = (TH1D*)gDirectory->Get(name.c_str());
    if (!h1[i]) {
      std::cerr << "Could not find histogram " << name << std::endl;
      continue;
    }
    if (h1[i]->GetMaximum() > ymax) ymax = h1[i]->GetMaximum();
  }
  
  TGaxis::SetMaxDigits(3);
  gStyle->SetOptStat(0);

  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    if (h1[i]) {
      std::string name = detNames[i];
      h1[i]->SetTitle(name.c_str());
      h1[i]->GetXaxis()->SetTitle("frame index");
      h1[i]->GetYaxis()->SetTitle("number of hits");
      h1[i]->GetYaxis()->SetRangeUser(0., 1.05 * ymax);
      h1[i]->SetLineColor(kBlue);
      h1[i]->SetFillColor(kBlue);
      h1[i]->DrawCopy();
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnClusterDifferencesX() {

  m_clusters_currentCanvasButton = M_CLUSTER_DIFFERENCES_X;
  handle_txtBtnClusterDifferences('x');
  
}

void TestBeamMoniGUI::handle_txtBtnClusterDifferencesY() {

  m_clusters_currentCanvasButton = M_CLUSTER_DIFFERENCES_Y;
  handle_txtBtnClusterDifferences('y');
  
}

void TestBeamMoniGUI::handle_txtBtnClusterDifferences(const char xy) {

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ResidualPlotter");
  if (!gDirectory) return;

  std::vector<TH1D*> h1;
  h1.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h1[i] = 0;
    if (xy == 'x') {
      std::string name = "DifferenceGlobalx_" + detNames[i];
      h1[i] = (TH1D*)gDirectory->Get(name.c_str());
    } else {
      std::string name = "DifferenceGlobaly_" + detNames[i];
      h1[i] = (TH1D*)gDirectory->Get(name.c_str());    
    }
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  gStyle->SetOptFit(111111);

  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  if (xy == 'x') {
    std::cout << "  Plane    max. x\n";
  } else {
    std::cout << "  Plane    max. y\n";
  }
  std::cout << "================================\n";
  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    if (h1[i]) {
      std::string name = detNames[i];
      h1[i]->GetXaxis()->SetTitle("difference [mm]");
      h1[i]->SetLineColor(kBlue);
      h1[i]->SetTitle(name.c_str());
      h1[i]->Draw();
      std::cout << detNames[i] << "   "
                << h1[i]->GetBinCenter(h1[i]->GetMaximumBin()) << "\n";
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnClusterCorrelationsX() {

  m_clusters_currentCanvasButton = M_CLUSTER_CORRELATIONS_X;
  handle_txtBtnClusterCorrelations('x');
  
}

void TestBeamMoniGUI::handle_txtBtnClusterCorrelationsY() {

  m_clusters_currentCanvasButton = M_CLUSTER_CORRELATIONS_Y;
  handle_txtBtnClusterCorrelations('y');
  
}

void TestBeamMoniGUI::handle_txtBtnClusterCorrelations(const char xy) {

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ResidualPlotter");
  if (!gDirectory) return;

  std::vector<TH2D*> h2;
  h2.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h2[i] = 0;
    if (xy == 'x') {
      std::string name = "CorrelationGlobalx_" + detNames[i];
      h2[i] = (TH2D*)gDirectory->Get(name.c_str());
    } else {
      std::string name = "CorrelationGlobaly_" + detNames[i];
      h2[i] = (TH2D*)gDirectory->Get(name.c_str());    
    }
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  gStyle->SetOptFit(111111);

  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  const bool rebin = false;
  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    if (h2[i]) {
      std::string name = detNames[i];
      h2[i]->SetTitle(name.c_str());
      if (xy == 'x') {
        h2[i]->GetXaxis()->SetTitle("x_{ref} [mm]");
        h2[i]->GetYaxis()->SetTitle("x [mm]");
      } else {
        h2[i]->GetXaxis()->SetTitle("y_{ref} [mm]");
        h2[i]->GetYaxis()->SetTitle("y [mm]");
      }      
      if (rebin) h2[i]->Rebin2D(3, 3);
      h2[i]->DrawCopy("boxcolz");
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnTOTSpectrum() {

  m_clusters_currentCanvasButton = M_CLUSTER_TOT_SPECTRUM;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ClusterMaker");
  if (!gDirectory) return;
  
  const int nClusterSizes = 4;
  std::vector<std::string> clusterSizeNames;
  clusterSizeNames.clear();
  clusterSizeNames.push_back("One");
  clusterSizeNames.push_back("Two");
  clusterSizeNames.push_back("Three");
  clusterSizeNames.push_back("Four");

  // Initialise the histograms.
  std::vector<std::vector<TH1D*> > h1;
  h1.resize(nPlanes);
  for (int i = nPlanes; i--;) {
    h1[i].resize(nClusterSizes);
    for (int j = nClusterSizes; j--;) h1[i][j] = 0;
  }
  
  for (int i = 0; i < nPlanes; ++i) {
    for (int j = 0; j < nClusterSizes; ++j) {
      std::string name = clusterSizeNames[j] + "PixelClusterTOTValues_" + detNames[i]; 
      h1[i][j] = (TH1D*)gDirectory->Get(name.c_str());
      if (!h1[i][j]) {
        std::cout << "Could not find histogram " << name << std::endl;
      }
    }
  }

  gStyle->SetOptStat(0);
  TGaxis::SetMaxDigits(3);
  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    std::string name = detNames[i];
    if (h1[i][0]) {
      h1[i][0]->UseCurrentStyle();
      h1[i][0]->SetTitle(name.c_str());
      if (h1[i][0]->GetXaxis()->GetXmax() > 11000. &&
          h1[i][0]->GetXaxis()->GetXmax() < 13000.) {
        h1[i][0]->GetXaxis()->SetTitle("ToA value");
      } else if (h1[i][0]->GetXaxis()->GetXmax() < 100.) {
        h1[i][0]->GetXaxis()->SetTitle("counts");
      } else {
        h1[i][0]->GetXaxis()->SetTitle("ToT value");
      }
    }

    TLegend* leg1 = new TLegend(0.4, 0.3, 0.9, 0.8);
    leg1->SetFillStyle(0);
    leg1->SetTextFont(42);
    leg1->SetShadowColor(0);
    leg1->SetBorderSize(0);

    for (int j = 0; j < nClusterSizes; ++j) {
      if (!h1[i][j]) continue;
      h1[i][j]->SetMarkerStyle(21);
      h1[i][j]->SetMarkerSize(0.4);
      h1[i][j]->SetLineWidth(2);
			// Normalization:
			double scale = 1. / h1[i][j]->Integral();
			h1[i][j]->Scale(scale);
			//
      switch (j) {
        case 0:
          h1[i][j]->SetMarkerColor(kBlack);
          h1[i][j]->SetLineColor(kBlack);
					break;
        case 1:
          h1[i][j]->SetMarkerColor(kBlue);
          h1[i][j]->SetLineColor(kBlue);
          break;
        case 2:
          h1[i][j]->SetMarkerColor(kRed + 2);
          h1[i][j]->SetLineColor(kRed + 2);
          break;
        case 3:
          h1[i][j]->SetMarkerColor(kGreen + 3);
          h1[i][j]->SetLineColor(kGreen + 3);
          break;
        default:
          break;
      }
		  if (j == 0) {
        h1[i][j]->Draw();
      } else {
        h1[i][j]->Draw("SAME");
        if (h1[i][j]->GetMaximum() > h1[i][j - 1]->GetMaximum()) {
          h1[i][0]->GetYaxis()->SetRangeUser(0, h1[i][j]->GetMaximum());
        }
      }
      std::string title = clusterSizeNames[j] + " pixel clusters";
      leg1->AddEntry(h1[i][j], title.c_str(), "l");
    }
    leg1->Draw();
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();
  
}

void TestBeamMoniGUI::handle_txtBtnClusterSize() {

  m_clusters_currentCanvasButton = M_CLUSTER_SIZE;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ClusterMaker");
  if (!gDirectory) return;
  
  std::vector<TH1D*> h1;
  h1.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h1[i] = 0;
    std::string name = "clusterSize_" + detNames[i];
    h1[i] = (TH1D*)gDirectory->Get(name.c_str());
    if (!h1[i]) {
      std::cerr << "Could not find histogram " << name << std::endl;
    }
  }

  gStyle->SetOptStat(100);
  TGaxis::SetMaxDigits(3);
  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    // gPad->SetGrid();
    if (h1[i]) {
      std::string name = detNames[i];
      h1[i]->SetTitle(name.c_str());
      h1[i]->SetLineColor(kBlue);
      h1[i]->SetFillColor(kBlue);
      // Choose a range for the histogram
      double hmin =  0.;
      double hmax = 12.;
      h1[i]->GetXaxis()->SetRangeUser(hmin, hmax);
      h1[i]->GetXaxis()->SetTitle("cluster size");
      h1[i]->Draw();
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();
  
}

void TestBeamMoniGUI::handle_txtBtnLocalClusterPositions() {

  m_clusters_currentCanvasButton = M_CLUSTER_LOCAL_POS;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ClusterMaker");
  if (!gDirectory) return;

  std::vector<TH2F*> h2;
  h2.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h2[i] = 0;
    std::string name = "clusterPos_" + detNames[i];
    h2[i] = (TH2F*)gDirectory->Get(name.c_str());
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  gStyle->SetOptFit(111111);

  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  std::cout << std::endl;
  std::cout << "  Chip           x            y\n";
  std::cout << "============================================\n";
  const bool rebin = false;
  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    if (h2[i]) {
      std::string name = detNames[i];
      std::cout << name << "    " 
                << h2[i]->GetMean(1) << " +/- " 
                << h2[i]->GetRMS(1)  << "     "
                << h2[i]->GetMean(2) << " +/- "
                << h2[i]->GetRMS(2)  << "\n";
      h2[i]->SetTitle(name.c_str());
      h2[i]->GetXaxis()->SetTitle("x (column)");
      h2[i]->GetYaxis()->SetTitle("y (row)");
      if (rebin) h2[i]->Rebin2D(3, 3);
      h2[i]->DrawCopy("colz");
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();

}


void TestBeamMoniGUI::handle_txtBtnGlobalClusterPositions() {

  m_clusters_currentCanvasButton = M_CLUSTER_GLOBAL_POS;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ClusterMaker");
  if (!gDirectory) return;

  std::vector<TH2F*> h2;
  h2.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h2[i] = 0;
    std::string name = "globalClusterPos_" + detNames[i];
    h2[i] = (TH2F*)gDirectory->Get(name.c_str());
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  gStyle->SetOptFit(111111);

  m_ecClusters->GetCanvas()->Clear();
  m_ecClusters->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  const bool rebin = false;
  std::cout << std::endl;
  std::cout << "  Chip           x [mm]            y [mm]\n";
  std::cout << "============================================\n";
  for (int i = 0; i < nPlanes; ++i) {
    m_ecClusters->GetCanvas()->cd(i + 1);
    if (h2[i]) {
      std::string name = detNames[i];
      std::cout << name << "    " 
                << h2[i]->GetMean(1) << " +/- " 
                << h2[i]->GetRMS(1)  << "     "
                << h2[i]->GetMean(2) << " +/- "
                << h2[i]->GetRMS(2)  << "\n";
      h2[i]->SetTitle(name.c_str());
      h2[i]->GetXaxis()->SetTitle("x [mm]");
      h2[i]->GetYaxis()->SetTitle("y [mm]");
      if (rebin) h2[i]->Rebin2D(3, 3);
      h2[i]->DrawCopy("colz");
    }
  }
  
  m_ecClusters->GetCanvas()->cd();
  m_ecClusters->GetCanvas()->Update();

}

//=============================================================================
// Handle the buttons of the tab "Tracks"
//=============================================================================
void TestBeamMoniGUI::handle_txtBtnProtoTracks() {

  m_tracks_currentCanvasButton = M_TRACK_PROTO_TRACKS;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  m_currentHistogramFile->cd("tpanal/PatternRecognition");
  if (!gDirectory) return;
  
  TH1F* hProtoTracks = (TH1F*)gDirectory->Get("prototracks");
  TH1F* hProtoTracksPerFrame = (TH1F*)gDirectory->Get("prototracksperframe");
  TH1F* hClustersPerTrack = (TH1F*)gDirectory->Get("clusterspertrack");
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  
  m_ecTracks->GetCanvas()->Clear();
  m_ecTracks->GetCanvas()->Divide(2, 2);

  m_ecTracks->GetCanvas()->cd(1);
  if (hProtoTracks) {
    hProtoTracks->SetTitle("Proto-tracks / frame");
    hProtoTracks->GetXaxis()->SetTitle("number of proto-tracks");
    hProtoTracks->SetLineColor(kBlue);
    hProtoTracks->DrawCopy();
  }
  m_ecTracks->GetCanvas()->cd(2);
  if (hProtoTracksPerFrame) {
    hProtoTracksPerFrame->SetTitle("Proto-tracks / frame");
    hProtoTracksPerFrame->GetXaxis()->SetTitle("frame index");
    hProtoTracksPerFrame->SetLineColor(kBlue);
    hProtoTracksPerFrame->SetFillColor(kBlue);
    hProtoTracksPerFrame->DrawCopy();
  }
  m_ecTracks->GetCanvas()->cd(3);
  if (hClustersPerTrack) {
    hClustersPerTrack->SetTitle("Clusters / proto-track");
    hClustersPerTrack->GetXaxis()->SetTitle("number of clusters");
    hClustersPerTrack->SetLineColor(kBlue);
    hClustersPerTrack->SetFillColor(kBlue);
    hClustersPerTrack->DrawCopy();
  }

  m_ecTracks->GetCanvas()->cd();
  m_ecTracks->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnTrackFit() {

  m_tracks_currentCanvasButton = M_TRACK_FIT;

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  m_currentHistogramFile->cd("tpanal/ResidualPlotter");
  if (!gDirectory) return;
  
  TH1F* hChi2 = (TH1F*)gDirectory->Get("trackchisquared");
  TH1F* hSlopeX = (TH1F*)gDirectory->Get("tracksxslope");
  TH1F* hSlopeY = (TH1F*)gDirectory->Get("tracksyslope");
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(1);
  
  m_ecTracks->GetCanvas()->Clear();
  m_ecTracks->GetCanvas()->Divide(3, 1);

  m_ecTracks->GetCanvas()->cd(1);
  if (hChi2) {
    hChi2->SetTitle("#chi^{2}");
    hChi2->GetXaxis()->SetTitle("#chi^{2}");
    hChi2->SetLineColor(kBlue);
    hChi2->DrawCopy();
  }
  m_ecTracks->GetCanvas()->cd(2);
  if (hSlopeX) {
//    hSlopeX->SetTitle("slope");
//    hSlopeX->GetXaxis()->SetTitle("#Deltax / #Deltaz");
    hSlopeX->SetTitle("Track Angle X");
    hSlopeX->GetXaxis()->SetTitle("radians");
    hSlopeX->SetLineColor(kBlue);
    hSlopeX->DrawCopy();
  }
  m_ecTracks->GetCanvas()->cd(3);
  if (hSlopeY) {
//    hSlopeY->SetTitle("slope");
//    hSlopeY->GetXaxis()->SetTitle("#Deltay / #Deltaz");
    hSlopeY->SetTitle("Track Angle Y");
    hSlopeY->GetXaxis()->SetTitle("radians");
    hSlopeY->SetLineColor(kBlue);
    hSlopeY->DrawCopy();
  }

  m_ecTracks->GetCanvas()->cd();
  m_ecTracks->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnResidualsX() {

  m_tracks_currentCanvasButton = M_TRACK_RESIDUALS_X;
  handle_txtBtnResiduals('x', false);
  
}

void TestBeamMoniGUI::handle_txtBtnResidualsY() {

  m_tracks_currentCanvasButton = M_TRACK_RESIDUALS_Y;
  handle_txtBtnResiduals('y', false);
  
}

void TestBeamMoniGUI::handle_txtBtnResidualsAssociatedX() {

  m_tracks_currentCanvasButton = M_TRACK_RESIDUALS_ASSOCIATED_X;
  handle_txtBtnResiduals('x', true);
  
}

void TestBeamMoniGUI::handle_txtBtnResidualsAssociatedY() {

  m_tracks_currentCanvasButton = M_TRACK_RESIDUALS_ASSOCIATED_Y;
  handle_txtBtnResiduals('y', true);
  
}

void TestBeamMoniGUI::handle_txtBtnResiduals(const char xy, const bool assoc) {

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ResidualPlotter");
  if (!gDirectory) return;
  
  std::vector<TH1D*> h1;
  h1.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h1[i] = 0;
    if (assoc) {
      if (xy == 'x') {
        std::string name = "xresidualassociated_" + detNames[i];
        h1[i] = (TH1D*)gDirectory->Get(name.c_str());
      } else {
        std::string name = "yresidualassociated_" + detNames[i];
        h1[i] = (TH1D*)gDirectory->Get(name.c_str());    
      }
    } else {
      if (xy == 'x') {
        std::string name = "xresidual_" + detNames[i];
        h1[i] = (TH1D*)gDirectory->Get(name.c_str());
      } else {
        std::string name = "yresidual_" + detNames[i];
        h1[i] = (TH1D*)gDirectory->Get(name.c_str());    
      }
    }
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);

  m_ecTracks->GetCanvas()->Clear();
  m_ecTracks->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  std::cout << std::endl;
  if (assoc) {
    std::cout << "Associated residuals in " << xy << "\n";
  } else {
    std::cout << "Residuals in " << xy << "\n";
  }
  std::cout << std::endl;
  std::cout << "Plane      mean [um]     sigma [um]         chi2\n";
  std::cout << "=================================================\n";
  for (int i = 0; i < nPlanes; ++i) {
    m_ecTracks->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    if (h1[i]) {
      std::string name = detNames[i];
      h1[i]->SetTitle(name.c_str());
      if (xy == 'x') {
        h1[i]->GetXaxis()->SetTitle("x residual [mm]");
      } else {
        h1[i]->GetXaxis()->SetTitle("y residual [mm]");
      }
      h1[i]->Fit("gaus", "Q0");
      h1[i]->SetLineColor(kBlack);
      h1[i]->DrawCopy();
      TF1* fFit = h1[i]->GetFunction("gaus");
      if (fFit) {
        fFit->SetLineColor(kRed + 2);
        fFit->Draw("same");
        double mean     = 1.e3 * fFit->GetParameter(1);
        double sigma    = 1.e3 * fFit->GetParameter(2);
        double sigmaErr = 1.e3 * fFit->GetParError(2);
        double chi2 = fFit->GetChisquare();
        std::cout << detNames[i] << "  "
                  << mean << "  " 
                  << sigma << " +/- " << sigmaErr 
                  << "  " << chi2 << "\n";
        TPaveText* pt = new TPaveText(0.6, 0.7, 0.9, 0.9, "NDC");
        pt->SetFillStyle(0);
        pt->SetTextFont(42);
        pt->SetShadowColor(0);
        pt->SetBorderSize(0);
        std::string title = "#sigma = " + toString(sigma, 1) + " #mum";
        pt->AddText(title.c_str());
        pt->Draw("same");
      }
    }
  }
  
  m_ecTracks->GetCanvas()->cd();
  m_ecTracks->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnInterceptsLocal() {

  m_tracks_currentCanvasButton = M_TRACK_INTERCEPTS_LOCAL;
  handle_txtBtnIntercepts(false);
  
}

void TestBeamMoniGUI::handle_txtBtnInterceptsGlobal() {

  m_tracks_currentCanvasButton = M_TRACK_INTERCEPTS_GLOBAL;
  handle_txtBtnIntercepts(true);
  
}

void TestBeamMoniGUI::handle_txtBtnIntercepts(const bool global) {

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/ResidualPlotter");
  if (!gDirectory) return;
  
  std::vector<TH2F*> h2;
  h2.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h2[i] = 0;
    if (global) {
      std::string name = "globaltrackintercepts_" + detNames[i];
      h2[i] = (TH2F*)gDirectory->Get(name.c_str());
    } else {
      std::string name = "localtrackintercepts_" + detNames[i];
      h2[i] = (TH2F*)gDirectory->Get(name.c_str());    
    }
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);

  m_ecTracks->GetCanvas()->Clear();
  m_ecTracks->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecTracks->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    if (h2[i]) {
      std::string name = detNames[i];
      h2[i]->SetTitle(name.c_str());
      h2[i]->GetXaxis()->SetTitle("x [mm]");
      h2[i]->GetYaxis()->SetTitle("y [mm]");
      h2[i]->DrawCopy("colz");
    }
  }
  
  m_ecTracks->GetCanvas()->cd();
  m_ecTracks->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnInterceptsTimestampYes() {

  m_tracks_currentCanvasButton = M_TRACK_INTERCEPTS_TIMESTAMPED;
  handle_txtBtnInterceptsTimestamp(true);
  
}

void TestBeamMoniGUI::handle_txtBtnInterceptsTimestampNo() {

  m_tracks_currentCanvasButton = M_TRACK_INTERCEPTS_NONTIMESTAMPED;
  handle_txtBtnInterceptsTimestamp(false);
  
}

void TestBeamMoniGUI::handle_txtBtnInterceptsTimestamp(const bool yes) {

  // Check if a ROOT file has been opened.
  if (!m_currentHistogramFile) return;

  // Retrieve the detector names.
  std::vector<std::string> detNames;
  if (!getDetectorNames(detNames)) return;
  const int nPlanes = detNames.size();

  m_currentHistogramFile->cd("tpanal/TrackTimestamper");
  if (!gDirectory) return;
  
  std::vector<TH2F*> h2;
  h2.resize(nPlanes);
  for (int i = 0; i < nPlanes; ++i) {
    h2[i] = 0;
    if (yes) {
      std::string name = "hassoctracks " + detNames[i];
      h2[i] = (TH2F*)gDirectory->Get(name.c_str());
    } else {
      std::string name = "hnonassoctracks " + detNames[i];
      h2[i] = (TH2F*)gDirectory->Get(name.c_str());    
    }
  }
  
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);

  m_ecTracks->GetCanvas()->Clear();
  m_ecTracks->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecTracks->GetCanvas()->cd(i + 1);
    gPad->SetGrid();
    if (h2[i]) {
      std::string name = detNames[i];
      h2[i]->SetTitle(name.c_str());
      h2[i]->GetXaxis()->SetTitle("x [mm]");
      h2[i]->GetYaxis()->SetTitle("y [mm]");
      h2[i]->DrawCopy("colz");
    }
  }
  
  m_ecTracks->GetCanvas()->cd();
  m_ecTracks->GetCanvas()->Update();

}

//=============================================================================
// Handle the buttons of the tab "Configuration"
//=============================================================================
void TestBeamMoniGUI::handle_txtBtnTHLDistributions() {

  m_config_currentCanvasButton = M_CONFIG_THL_DIST;
  
  const int nPlanes = configs.size();
  if (nPlanes <= 0) {
    std::cerr << "No configurations loaded.\n";
    return;
  }
  
  std::vector<int> thresholds(nPlanes, -1);
  if (m_currentEventFile) {
    TTree* tbtree = (TTree*)m_currentEventFile->Get("tbtree");
    if (tbtree) {
      TestBeamDataSummary* summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
      for (int i = 0; i < nPlanes; ++i) {
        std::string chipId = configs[i].chipId;
        bool foundThreshold = false;
        for (int j = 0; j < summary->nDetectors(); ++j) {
          if (summary->detectorId(j) == chipId) {
            thresholds[i] = summary->thresholdForDetector(chipId);
            foundThreshold = true;
            break;
          }
        }
        if (!foundThreshold) {
          std::cerr << "Could not find threshold for detector " << chipId << "\n";
        }
      }
    } else {
      std::cerr << "Cannot access event summary.\n";
    }
  } else {
    std::cerr << "No event file open. Cannot load thresholds.\n";
  }
    
  gStyle->SetOptStat(0);
  m_ecConfig->GetCanvas()->Clear();
  m_ecConfig->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecConfig->GetCanvas()->cd(i + 1);
    const std::string chipId = configs[i].chipId;
    const int relaxdId = configs[i].relaxdId;
    const int chipPos = configs[i].chipPos;
    std::string name = "RelaxD" + toString(relaxdId) + 
                       ": Chip" + toString(chipPos) + 
                       " [" + chipId + "]";
    const int thlMin = configs[i].startValue;
    const int thlMax = configs[i].endValue;
    const int nSteps = configs[i].nSteps;
    if (nSteps == 0) continue;
    const int step = (thlMax - thlMin) / nSteps;
    TH1F* h_max = new TH1F("h_max", "h_max", nSteps + 1, 
                           thlMin - 0.5, thlMax + 0.5);
    TH1F* h_min = new TH1F("h_min", "h_min", nSteps + 1, 
                           thlMin - 0.5, thlMax + 0.5);
    TH1F* h_eql = new TH1F("h_eql", name.c_str(), nSteps + 1, 
                           thlMin - 0.5, thlMax + 0.5);
      
    for (int j = 0; j < nSteps; ++j) {
      const double dac = thlMin + j * step;
      h_max->Fill(dac, configs[i].distributions[j][0]);
      h_min->Fill(dac, configs[i].distributions[j][1]);
      h_eql->Fill(dac, configs[i].distributions[j][2]);
    }
    h_min->SetLineColor(kBlue);
    h_min->SetFillColor(kBlue);
    h_max->SetLineColor(kRed + 2);
    h_max->SetFillColor(kRed + 2);
    h_eql->SetLineColor(kBlack);
    h_eql->SetFillColor(kBlack);
    
    h_eql->GetXaxis()->SetTitle("DAC counts");
    h_eql->DrawCopy();
    h_min->DrawCopy("same");
    h_max->DrawCopy("same");
    h_eql->DrawCopy("same");
    
    if (thresholds[i] > 0) {
      TLine* line = new TLine(thresholds[i], 0, 
                              thresholds[i], h_eql->GetMaximum());
      line->SetLineWidth(2);
      line->SetLineColor(kGreen + 2);
      line->Draw();
    }
    
    TPaveText* pt = new TPaveText(0.6, 0.7, 0.88, 0.88, "NDC");
    pt->SetFillStyle(0);
    pt->SetTextFont(42);
    pt->SetShadowColor(0);
    pt->SetBorderSize(0);
    int maxbin = h_eql->GetMaximumBin();
    int eqmean = thlMin + maxbin * step;
    std::string title = "mean eq.: " + toString(eqmean);
    pt->AddText(title.c_str());
    if (thresholds[i] > 0) {
      title = "THL: " + toString(thresholds[i]);
      pt->AddText(title.c_str());
      const double dac2Charge = 25.4;
      const double diffDac = (eqmean - thresholds[i]);
      const double diffCharge = diffDac * dac2Charge;
      title = "diff.: " + toString(int(diffDac)) + 
              "(" + toString(int(diffCharge)) + " e^{-})";
      pt->AddText(title.c_str());
    } else {
      title = "THL: unknown";
      pt->AddText(title.c_str());
    }
    pt->SetTextSize(0.045);
    pt->Draw("same");
    
    h_eql->Delete();
    h_min->Delete();
    h_max->Delete();
      
  }

  m_ecConfig->GetCanvas()->Update();

}
void TestBeamMoniGUI::handle_txtBtnAdjBits() {

  m_config_currentCanvasButton = M_CONFIG_ADJUSTMENT;
  
  const int nPlanes = configs.size();
  if (nPlanes <= 0) {
    std::cerr << "No configurations loaded.\n";
    return;
  }
      
  gStyle->SetOptStat(0);
  m_ecConfig->GetCanvas()->Clear();
  m_ecConfig->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecConfig->GetCanvas()->cd(i + 1);
    const std::string chipId = configs[i].chipId;
    const int relaxdId = configs[i].relaxdId;
    const int chipPos = configs[i].chipPos;
    std::string name = "RelaxD" + toString(relaxdId) + 
                       ": Chip" + toString(chipPos) + 
                       " [" + chipId + "]";
    const int nBits = 16;
    TH1F* hAdj = new TH1F("hAdj", name.c_str(), nBits,
                          - 0.5, 15.5);
    for (int j = 0; j < nBits; ++j) {
      hAdj->Fill(j, configs[i].bits[j]);
    }
    hAdj->SetLineColor(kBlue);
    hAdj->SetFillColor(kBlue);
    hAdj->GetXaxis()->SetTitle("pixel adjustment");
    hAdj->DrawCopy();
    hAdj->Delete();      
  }

  m_ecConfig->GetCanvas()->Update();

}

void TestBeamMoniGUI::handle_txtBtnMasked() {

  m_config_currentCanvasButton = M_CONFIG_MASKED;
  
  const int nPlanes = configs.size();
  if (nPlanes <= 0) {
    std::cerr << "No configurations loaded.\n";
    return;
  }
      
  gStyle->SetOptStat(0);
  m_ecConfig->GetCanvas()->Clear();
  m_ecConfig->GetCanvas()->Divide(nPlanes / 2 + nPlanes % 2, 2);

  for (int i = 0; i < nPlanes; ++i) {
    m_ecConfig->GetCanvas()->cd(i + 1);
    const std::string chipId = configs[i].chipId;
    const int relaxdId = configs[i].relaxdId;
    const int chipPos = configs[i].chipPos;
    std::string name = "RelaxD" + toString(relaxdId) + 
                       ": Chip" + toString(chipPos) + 
                       " [" + chipId + "]";
    const int nRows = 256;
    const int nCols = 256;
    TH2C* hMasked = new TH2C("hMasked", name.c_str(), 
                             nRows, -0.5, nRows - 0.5,
                             nCols, -0.5, nCols - 0.5); 
    for (int j = 0; j < nRows; ++j) {
      for (int k = 0; k < nCols; ++k) {
        if (configs[i].masked[j][k]) hMasked->Fill(j, k);
      }
    }
    hMasked->SetLineColor(kRed + 2);
    hMasked->SetFillColor(kRed + 2);
    hMasked->GetXaxis()->SetTitle("row");
    hMasked->GetYaxis()->SetTitle("column");
    hMasked->GetYaxis()->SetTitleOffset(1.5);
    hMasked->DrawCopy("BOX");
    hMasked->Delete();      
  }
  m_ecConfig->GetCanvas()->Update();

}

//=============================================================================
// Handle the buttons of the tab "Amalgamation"
//=============================================================================
void TestBeamMoniGUI::handle_txtBtnTDC() {

  m_amalg_currentCanvasButton = M_AMALG_TDC;
  
  if (!m_currentEventFile) {
    std::cerr << "No event file open. Cannot access TDC histograms.\n";
    return;
  }
  
  TTree* tbtree = (TTree*)m_currentEventFile->Get("tbtree");
  if (!tbtree) {
    std::cerr << "Cannot access event summary.\n";
    return;
  }
  
  TestBeamDataSummary* summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
  
  m_ecAmalgamation->GetCanvas()->Clear();
  m_ecAmalgamation->GetCanvas()->Divide(3, 2);
  TGaxis::SetMaxDigits(5);

  m_ecAmalgamation->GetCanvas()->cd(1);
  summary->hNumberPairs->UseCurrentStyle();
  summary->hNumberSync->UseCurrentStyle();
  summary->hNumberUnsync->UseCurrentStyle();
  summary->hNumberPairs->SetLineColor(kBlack);
  summary->hNumberSync->SetLineColor(kRed + 2);
  summary->hNumberUnsync->SetLineColor(kBlue);
  TH1F* h0 = 0;
  TH1F* h1 = summary->hNumberPairs;
  TH1F* h2 = summary->hNumberSync;
  TH1F* h3 = summary->hNumberUnsync;
  if (h2->GetMaximum() > h1->GetMaximum()) {h0 = h1; h1 = h2; h2 = h0;}
  if (h3->GetMaximum() > h2->GetMaximum()) {h0 = h2; h2 = h3; h3 = h0;}
  if (h2->GetMaximum() > h1->GetMaximum()) {h0 = h1; h1 = h2; h2 = h0;}
  h1->SetTitle("Triggers / Frame");
  h1->SetStats(0);
  h1->DrawCopy(); 
  h2->DrawCopy("same"); 
  h3->DrawCopy("same");
  TLegend* lN = new TLegend(0.7, 0.6, 0.9, 0.9);
  lN->SetFillStyle(0);
  lN->SetTextFont(42);
  lN->SetShadowColor(0);
  lN->SetBorderSize(0);
  lN->AddEntry(summary->hNumberSync, "Sync", "l");
  lN->AddEntry(summary->hNumberUnsync, "UnSync" ,"l");
  lN->AddEntry(summary->hNumberPairs, "Paired", "l");
  lN->Draw();    
  
  m_ecAmalgamation->GetCanvas()->cd(2);
  gPad->SetLogy();
  summary->hExtraSyncTriggers->Scale(1. / summary->hExtraSyncTriggers->GetEntries());
  summary->hExtraUnsyncTriggers->Scale(1. / summary->hExtraUnsyncTriggers->GetEntries());
  summary->hExtraSyncTriggers->UseCurrentStyle();
  summary->hExtraSyncTriggers->SetTitle("Unmatched Triggers / Frame");
  summary->hExtraSyncTriggers->SetStats(0);
  summary->hExtraSyncTriggers->SetLineColor(kRed + 2);
  summary->hExtraSyncTriggers->DrawCopy();
  summary->hExtraSyncTriggers->UseCurrentStyle();
  summary->hExtraUnsyncTriggers->SetLineColor(kBlue);
  summary->hExtraUnsyncTriggers->DrawCopy("same");
  TLegend* lE = new TLegend(0.7, 0.7, 0.9, 0.9);
  lE->SetFillStyle(0);
  lE->SetTextFont(42);
  lE->SetShadowColor(0);
  lE->SetBorderSize(0);
  lE->AddEntry(summary->hExtraUnsyncTriggers, "UnSync", "l");
  lE->AddEntry(summary->hExtraSyncTriggers, "Sync", "l");
  lE->Draw();

  gStyle->SetOptStat(111110);
  m_ecAmalgamation->GetCanvas()->cd(3);
  gPad->SetLogy();
  TAxis* axis = summary->hTDCShutterTime->GetXaxis();
  int tot = 0;
  int lowerbin = 0;
  for (int i = 1; i < axis->GetNbins(); ++i) {
    tot += (int)summary->hTDCShutterTime->GetBinContent(i);
    if (tot > 1) {
      lowerbin = i;
      break;
    }
  }
  tot = 0;
  int higherbin =axis->GetNbins();
  for (int i = axis->GetNbins(); i != 0; i--) {
    tot += (int)summary->hTDCShutterTime->GetBinContent(i);
    if (tot > 0.005 * summary->hTDCShutterTime->GetEntries()) {
      higherbin = i;
      break;
    }
  }
  TH1F* hTDCShutterTime = new TH1F("hTDCShutterTime2", summary->hTDCShutterTime->GetTitle(), 
                                   100, 0, axis->GetBinCenter(higherbin));
  for (int i = 0;i <= axis->GetNbins(); ++i) {
    for (int j = 0; j < summary->hTDCShutterTime->GetBinContent(i); ++j) {
      hTDCShutterTime->Fill(summary->hTDCShutterTime->GetBinCenter(i));
    }
  }
  hTDCShutterTime->SetTitle("Shutter Time");
  hTDCShutterTime->GetXaxis()->SetNdivisions(5);
  hTDCShutterTime->GetXaxis()->SetTitle("duration [#mus]");
  hTDCShutterTime->GetXaxis()->SetTitleOffset(0.5);
  hTDCShutterTime->DrawCopy();

  m_ecAmalgamation->GetCanvas()->cd(4);
  gPad->SetLogy();
  TH1F* hTDCUnsyncTriggerTime = new TH1F("hTDCUnsyncTriggerTime2", summary->hTDCUnsyncTriggerTime->GetTitle(),
                                         100, 0, axis->GetBinCenter(higherbin));
  for (int i = 0; i <= axis->GetNbins(); ++i) {
    for (int j = 0; j < summary->hTDCUnsyncTriggerTime->GetBinContent(i); ++j) {
      hTDCUnsyncTriggerTime->Fill(summary->hTDCUnsyncTriggerTime->GetBinCenter(i));
    }
  }
  hTDCUnsyncTriggerTime->SetTitle("T_{unsync} - T_{open shutter}");
  hTDCUnsyncTriggerTime->GetXaxis()->SetNdivisions(5);
  hTDCUnsyncTriggerTime->GetXaxis()->SetTitle("delay [#mus]");
  hTDCUnsyncTriggerTime->GetXaxis()->SetTitleOffset(0.5);
  hTDCUnsyncTriggerTime->UseCurrentStyle();
  hTDCUnsyncTriggerTime->SetLineColor(kBlue);
  hTDCUnsyncTriggerTime->DrawCopy();
    
  m_ecAmalgamation->GetCanvas()->cd(5);
  summary->hSynchronisationDelay->SetTitle("T_{sync} - T_{unsync}");
  summary->hSynchronisationDelay->GetXaxis()->SetTitle("delay [ns]");
  summary->hSynchronisationDelay->Draw();
 
  m_ecAmalgamation->GetCanvas()->cd(6);
  if (summary->hTDCTime) {
    summary->hVetraTime->UseCurrentStyle();
    summary->hVetraTime->SetStats(0);
    summary->hVetraTime->SetMaximum(6);
    summary->hVetraTime->SetLineColor(1);
    summary->hVetraTime->SetFillColor(1);
    summary->hVetraTime->SetTitle("Spills");
    summary->hVetraTime->DrawCopy("hist");
    summary->hVetraTime->GetXaxis()->SetTitle("seconds");
    summary->hMedipixTime->UseCurrentStyle();
    summary->hMedipixTime->SetLineColor(4);
    summary->hMedipixTime->SetFillColor(4);
    summary->hMedipixTime->DrawCopy("hist same");
    summary->hTDCTime->UseCurrentStyle();
    summary->hTDCTime->SetLineColor(2);
    summary->hTDCTime->SetFillColor(2);
    summary->hTDCTime->DrawCopy("hist same");
    TLegend* l2 = new TLegend(0.7, 0.6, 0.9, 0.9);
    l2->SetTextFont(42);
    l2->SetFillStyle(0);
    l2->SetShadowColor(0);
    l2->SetBorderSize(0);
    l2->AddEntry(summary->hVetraTime, "DUT", "f");
    l2->AddEntry(summary->hTDCTime, "TDC", "f");
    l2->AddEntry(summary->hMedipixTime, "Timepix", "f");
    l2->Draw();
  }

  m_ecAmalgamation->GetCanvas()->Update();

}

void TestBeamMoniGUI::updateTabClustersCanvas() {

  switch (m_clusters_currentCanvasButton) {
    case M_CLUSTER_HITS_PER_FRAME:
      handle_txtBtnHitsPerFrame();
      break;
    case M_CLUSTER_DIFFERENCES_X:
      handle_txtBtnClusterDifferencesX();
      break;
    case M_CLUSTER_DIFFERENCES_Y:
      handle_txtBtnClusterDifferencesY();
      break;
    case M_CLUSTER_CORRELATIONS_X:
      handle_txtBtnClusterCorrelationsX();
      break;
    case M_CLUSTER_CORRELATIONS_Y:
      handle_txtBtnClusterCorrelationsY();
      break;
    case M_CLUSTER_SIZE:
      handle_txtBtnClusterSize();
      break;
    case M_CLUSTER_TOT_SPECTRUM:
      handle_txtBtnTOTSpectrum();
      break;
    case M_CLUSTER_LOCAL_POS:
      handle_txtBtnLocalClusterPositions();
      break;
    case M_CLUSTER_GLOBAL_POS:
      handle_txtBtnGlobalClusterPositions();
      break;
    default:
      std::cerr << "updateTabClustersCanvas(): option " 
                << m_clusters_currentCanvasButton << " unknown\n";
      break;
  }

}

void TestBeamMoniGUI::updateTabTracksCanvas() {

  switch (m_tracks_currentCanvasButton) {
    case M_TRACK_PROTO_TRACKS:
      handle_txtBtnProtoTracks();
      break;
    case M_TRACK_FIT:
      handle_txtBtnTrackFit();
      break;
    case M_TRACK_RESIDUALS_X:
      handle_txtBtnResidualsX();
      break;
    case M_TRACK_RESIDUALS_Y:
      handle_txtBtnResidualsY();
      break;
    case M_TRACK_RESIDUALS_ASSOCIATED_X:
      handle_txtBtnResidualsAssociatedX();
      break;
    case M_TRACK_RESIDUALS_ASSOCIATED_Y:
      handle_txtBtnResidualsAssociatedY();
      break;
    case M_TRACK_INTERCEPTS_LOCAL:
      handle_txtBtnInterceptsLocal();
      break;
    case M_TRACK_INTERCEPTS_GLOBAL:
      handle_txtBtnInterceptsGlobal();
      break;
    case M_TRACK_INTERCEPTS_TIMESTAMPED:
      handle_txtBtnInterceptsTimestampYes();
      break;
    case M_TRACK_INTERCEPTS_NONTIMESTAMPED:
      handle_txtBtnInterceptsTimestampNo();
      break;
    default:
      std::cerr << "updateTabTracksCanvas(): option " 
                << m_tracks_currentCanvasButton << " unknown\n";
      break;
  }

}

void TestBeamMoniGUI::updateTabConfigCanvas() {

  switch (m_config_currentCanvasButton) {
    case M_CONFIG_THL_DIST:
      handle_txtBtnTHLDistributions();
      break;
    case M_CONFIG_ADJUSTMENT:
      handle_txtBtnAdjBits();
      break;
    case M_CONFIG_MASKED:
      handle_txtBtnMasked();
      break;
    default:
      std::cerr << "updateTabConfigCanvas(): option " 
                << m_config_currentCanvasButton << " unknown\n";
      break;
  }

}

void TestBeamMoniGUI::updateTabAmalgamationCanvas() {

  switch (m_amalg_currentCanvasButton) {
    case M_AMALG_TDC:
      handle_txtBtnTDC();
      break;
    default:
      std::cerr << "updateTabAmalgamationCanvas(): option"
                << m_amalg_currentCanvasButton << " unknown\n";
      break;
  }

}

bool TestBeamMoniGUI::getDetectorNames(std::vector<std::string>& names) {

  names.clear();
  if (!m_currentEventFile) return false;
  TTree* tbtree = (TTree*)m_currentEventFile->Get("tbtree");
  if (!tbtree) return false;
  std::vector<std::pair<std::string, double> > positions;
  TestBeamDataSummary* summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string id = summary->detectorId(i);
    double z = -99999.;
    for (unsigned int j = alignments.size(); j--;) {
      if (alignments[j].chipId == id) {
        z = alignments[j].z;
        break;
      }
    }
    positions.push_back(std::make_pair(id, z));
  }
  std::sort(positions.begin(), positions.end(), TestBeamMoniGUI::increasingZ());
  for (unsigned int i = 0; i < positions.size(); ++i) {
    names.push_back(positions[i].first);
  }
  return true;
  
}

void TestBeamMoniGUI::setROOTColourPalette() {

  const int nRGBs = 5;
  const int nCont = 255;
    
  double stops[nRGBs] = {0.00, 0.34, 0.61, 0.84, 1.00};
  double red[nRGBs]   = {0.00, 0.00, 0.87, 1.00, 0.51};
  double green[nRGBs] = {0.00, 0.81, 1.00, 0.20, 0.00};
  double blue[nRGBs]  = {0.51, 1.00, 0.12, 0.00, 0.00};
  TColor::CreateGradientColorTable(nRGBs, stops, red, green, blue, nCont);
  gStyle->SetNumberContours(nCont);

}

void TestBeamMoniGUI::setROOTStyle() {

  gROOT->SetStyle("Plain");
  gStyle->SetCanvasBorderMode(0); 
  gStyle->SetCanvasColor(0);
  gStyle->SetPadBorderMode(0);
  gStyle->SetPadColor(0);
  gStyle->SetFrameBorderMode(0);
  gStyle->SetFrameFillColor(0);
  gStyle->SetDrawBorder(0);
  gStyle->SetStatStyle(0);
  gStyle->SetStatBorderSize(0);
  gStyle->SetStatColor(0);
  gStyle->SetStatFont(42);
  gStyle->SetStatFontSize(0.05);
  gStyle->SetStatX(0.9);
  gStyle->SetStatY(0.9);
  gStyle->SetStatW(0.25);
  gStyle->SetStatH(0.15);
  gStyle->SetPalette(1); 
  gStyle->SetTextSize(0.05);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleFont(42, "xyz");
  gStyle->SetTitleFont(42, "t");
  gStyle->SetTitleSize(0.05, "xyz");
  gStyle->SetTitleSize(0.05, "t");
  gStyle->SetTitleFillColor(0);
  gStyle->SetTitleX(0.3);
  gStyle->SetTitleW(0.4);
  gStyle->SetLabelFont(42, "xyz");
  gStyle->SetLabelSize(0.05, "xyz");
  gStyle->SetLabelOffset(0.015);
  gStyle->SetTitleOffset(1, "xy");
  gStyle->SetLegendFont(42);
  gStyle->SetTickLength(0.05,"x");
  gStyle->SetNdivisions(505, "x");
  gStyle->SetNdivisions(510, "y");
  gROOT->ForceStyle();

}

std::string TestBeamMoniGUI::toString(int num) {

	std::ostringstream start;
	start << num;
	return start.str();

}

std::string TestBeamMoniGUI::toString(double num, int precision) {

  std::stringstream start;
  start.precision(precision);
  start << std::fixed << num;
  return start.str();
  
}
