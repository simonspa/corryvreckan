#ifndef GUIDISPLAY_H
#define GUIDISPLAY_H 1

// Local includes
#include "TestBeamObject.h"

// Global includes
#include "signal.h"
#include <iostream>

// ROOT includes
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TGCanvas.h"
#include "TGFrame.h"
#include "TGTextEntry.h"
#include "TGDockableFrame.h"
#include "TGMenu.h"
#include "TApplication.h"
#include <RQ_OBJECT.h>
#include "TROOT.h"
#include "TSystem.h"
#include "TRootEmbeddedCanvas.h"

class GuiDisplay : public TestBeamObject {
  
  RQ_OBJECT("GuiDisplay")

public:
  // Constructors and destructors
  GuiDisplay(){};
  ~GuiDisplay(){}

  // Graphics associated with GUI
  TGMainFrame*														m_mainFrame;
  TRootEmbeddedCanvas*										canvas;
  map<string, vector<TH1*>> 							histograms;
  map<TH1*,string> 												styles;
  map<string, TGTextButton*> 							buttons;
  map<TRootEmbeddedCanvas*, bool>					stackedCanvas;
  TGHorizontalFrame* 											buttonMenu;

  // Button functions
  inline void Display(char* canvasNameC){
    string canvasName(canvasNameC);
    if(histograms[canvasName].size() == 0){
      cout<<"Canvas does not have any histograms, exiting"<<endl;
      return;
    }
    int nHistograms = histograms[canvasName].size();
    canvas->GetCanvas()->Clear();
    canvas->GetCanvas()->cd();
    if(!stackedCanvas[canvas]){
      if(nHistograms < 4) canvas->GetCanvas()->Divide(nHistograms);
      else canvas->GetCanvas()->Divide(ceil(nHistograms/2.),2);
    }
    for(int i=0;i<nHistograms;i++){
      if(!stackedCanvas[canvas]) canvas->GetCanvas()->cd(i+1);
      string style = styles[histograms[canvasName][i]];
      if(stackedCanvas[canvas]){
        style = "same";
        histograms[canvasName][i]->SetLineColor(i+1);
      }
      histograms[canvasName][i]->Draw(style.c_str());
    }
    canvas->GetCanvas()->Paint();
    canvas->GetCanvas()->Update();
  };
  
  // Exit the monitoring
  inline void Exit(){
    raise(SIGQUIT);
  }

  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(GuiDisplay,0)
  
};

#endif // GUIDISPLAY_H
