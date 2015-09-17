// $Id: PulseShapeAnalyser.C,v 1.4 2009-07-13 15:11:11 mjohn Exp $
// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>


#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TF1.h"

// local
#include "Clipboard.h"
#include "PulseShapeAnalyser.h"
#include "TestBeamEventElement.h"
#include "TestBeamCluster.h"
#include "LanGau.h"

//-----------------------------------------------------------------------------
// Implementation file for class : PulseShapeAnalyser
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

PulseShapeAnalyser::~PulseShapeAnalyser(){} 

PulseShapeAnalyser::PulseShapeAnalyser(Parameters* p,bool d)
  : Algorithm("PulseShapeAnalyser")
{
  parameters=p;
  display=d;
  TFile *inputFile=TFile::Open(parameters->eventFile.c_str(),"READ");
  TTree *tbtree=(TTree*)inputFile->Get("tbtree");
  summary=(TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
}
 
void PulseShapeAnalyser::initial()
{
  TH1F *h=0;TH1F *g=0;
  for(int i=0;i<summary->nDetectors();i++){
    std::string source=summary->source(i); 
    std::string chip=summary->detectorId(i);
    std::string title=source+std::string(" ADC value, ")+chip;
    std::string name=std::string("pulseshape_")+chip;
    if(source==std::string("timepix"))
      h = new TH1F(name.c_str(),title.c_str(),200,-0.5, 599.5 );
    if(source==std::string("medipix"))
      h = new TH1F(name.c_str(),title.c_str(), 11,-0.5,  10.5 );
    pulseShape.insert(make_pair(chip,h));
    name=std::string("pulseshape_raw_")+chip;
    g = (TH1F*)h->Clone(name.c_str());
    pulseShapeRaw.insert(make_pair(chip,g));
  }
}

void PulseShapeAnalyser::run(TestBeamEvent *event, Clipboard *clipboard)
{
  TestBeamClusters* clusters=(TestBeamClusters*)clipboard->get("Clusters");
  TestBeamClusters::iterator iter=clusters->begin();
  for(;iter!=clusters->end();iter++){
    TestBeamCluster *cluster = *iter;
    std::string chip=cluster->element()->detectorId();
      std::string source=cluster->element()->sourceName();
    if(source==std::string("timepix")){
      pulseShape[chip]->Fill(cluster->totalADC());
    }else{
      std::vector<RowColumnEntry*>* veCl = cluster->hits();
      std::vector<RowColumnEntry*>::iterator itCl=veCl->begin();
      double highest=0;
      for(;itCl!=veCl->end();itCl++){
        if((*itCl)->value()>highest) highest=(*itCl)->value();
      }
      pulseShape[chip]->Fill(highest);
    }
  }

  for(unsigned int j=0;j<event->nElements();j++){
    TestBeamEventElement *element=event->getElement(j);
    std::vector< RowColumnEntry* >* data = element->data();
    std::string chip = element->detectorId();
    for(std::vector< RowColumnEntry* >::iterator k=data->begin();k<data->end();k++){
      pulseShapeRaw[chip]->Fill((*k)->value());
    }
  }
}


void PulseShapeAnalyser::end()
{
  if(!display || !parameters->verbose) return;
  TCanvas *cPS = new TCanvas("cPS","VELO Timepix testbeam",1200,500);
  cPS->Divide(4,2);
  for(int i=0;i<summary->nDetectors();i++){
    std::string chip = summary->detectorId(i);
    cPS->cd(i+1);
    pulseShape[chip]->DrawCopy();
    //pulseShapeRaw[chip]->SetLineColor(2);
    pulseShapeRaw[chip]->SetLineStyle(3);
    pulseShapeRaw[chip]->DrawCopy("same");
    if(summary->source(i)==std::string("timepix")){
      TF1 *fit = LanGau::FitLanGau(pulseShape[chip]);
      fit->SetLineColor(4);
      fit->SetLineWidth(1);
      fit->DrawCopy("same");
    }else{
      gPad->SetLogy();
    }
  }
}

