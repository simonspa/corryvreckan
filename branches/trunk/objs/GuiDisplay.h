#ifndef GUIDISPLAY_H
#define GUIDISPLAY_H 1

#include "TestBeamObject.h"

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
  vector<TRootEmbeddedCanvas*> 						canvasVector;
  map<string, TRootEmbeddedCanvas*> 			canvases;
  map<TRootEmbeddedCanvas*, vector<TH1*>> histograms;
  map<TH1*,string> 												styles;
  map<string, TGTextButton*> 							buttons;
  map<TRootEmbeddedCanvas*, bool>					stackedCanvas;

  
  // Button functions
  inline void Display(char* canvasName){
    if(canvases.count(canvasName) == 0){
      cout<<"Canvas does not exist, exiting"<<endl;
      return;
    }
    TRootEmbeddedCanvas* canvas = canvases[canvasName];
    int nHistograms = histograms[canvas].size();
    canvas->GetCanvas()->Clear();
    canvas->GetCanvas()->cd();
    if(!stackedCanvas[canvas]){
      if(nHistograms < 4) canvas->GetCanvas()->Divide(nHistograms);
      else canvas->GetCanvas()->Divide(ceil(nHistograms/2.),2);
    }
    for(int i=0;i<nHistograms;i++){
      if(!stackedCanvas[canvas]) canvas->GetCanvas()->cd(i+1);
      string style = styles[histograms[canvas][i]];
      if(stackedCanvas[canvas]){
        style = "same";
        histograms[canvas][i]->SetLineColor(i+1);
      }
      histograms[canvas][i]->Draw(style.c_str());
    }
    canvases[canvasName]->GetCanvas()->Paint();
    canvases[canvasName]->GetCanvas()->Update();
  };

  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(GuiDisplay,0)
  
};

#endif // GUIDISPLAY_H
