// $Id: EventDisplay.C,v 1.9 2010-06-07 11:08:34 mjohn Exp $
// Include files 
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "TStyle.h"
#include "TCanvas.h"
#include "TPolyLine3D.h"

// local
#include "Clipboard.h"
#include "EventDisplay.h"
#include "TestBeamTrack.h"
#include "TestBeamCluster.h"
#include "CommonTools.h"

//-----------------------------------------------------------------------------
// Implementation file for class : EventDisplay
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

EventDisplay::~EventDisplay(){} 

EventDisplay::EventDisplay(Parameters* p,TApplication *a)
  : Algorithm("EventDisplay")
{
  app=a;
  parameters=p;
}
 
void EventDisplay::initial()
{
  cCC1 = new TCanvas("cClusCol","VELO Timepix testbeam event display",768,768);
  graph=new TGraph2D();
  graph->SetNameTitle("Master","Event Display");
  graph->SetMarkerSize(0.8);
  graph->SetMarkerStyle(20);
  
  hhh=new TH2F("hhh","     (Select 'Quit ROOT' from File menu for next event)",100,-10,10,100,-10,10);
  hhh->GetXaxis()->SetTitle("X (mm)");
  hhh->GetYaxis()->SetTitle("Y (mm)");
  hhh->GetXaxis()->SetTitleOffset(1.5);
  hhh->GetYaxis()->SetTitleOffset(1.5);
  hhh->SetMinimum(-5);
  hhh->SetMaximum(915);
  hhh->SetStats(0);
}

void EventDisplay::run(TestBeamEvent *event, Clipboard *clipboard)
{
  TestBeamClusters* clusters=(TestBeamClusters*)clipboard->get("Clusters");
  if(clusters==NULL) return;
  if(clusters->size()<2) return;  
// plot clusters
  int graphCounter=-1;
  for(TestBeamClusters::iterator iter=clusters->begin();iter!=clusters->end();iter++){
    TestBeamCluster *cluster = *iter;
    std::string chip=cluster->element()->detectorId();
    graph->SetPoint( graphCounter++ , cluster->globalX(),cluster->globalY(),cluster->globalZ() );
  }
  graph->Set(graphCounter);
  TH2F* h=(TH2F*)hhh->Clone();
//  time_t time(event->getElement(0)->timeStamp());
//  std::string timeString(asctime(localtime(&time)));
//  CommonTools::stringtrim(timeString);
  h->SetTitle((std::string(h->GetTitle())).c_str());
//  h->SetTitle((timeString+std::string(h->GetTitle())).c_str());
  graph->SetHistogram(h);
  graph->GetHistogram()->Draw();
  gStyle->SetPalette(1);
  graph->Draw("pcol");


  // plot tracks
  TestBeamTracks* tracks=(TestBeamTracks*)clipboard->get("Tracks");
  if(tracks){
    for(TestBeamTracks::iterator iter=tracks->begin();iter!=tracks->end();iter++){
      TestBeamTrack *track=(*iter);
      TPolyLine3D *aline = new TPolyLine3D(2);
      aline->SetPoint( 0, track->firstState()->X(),track->firstState()->Y(),track->firstState()->Z() );
      aline->SetPoint( 1, track->firstState()->X()+track->slopeXZ()*(h->GetMaximum()-5),
                          track->firstState()->Y()+track->slopeYZ()*(h->GetMaximum()-5),h->GetMaximum()-5);
      aline->SetLineColor(2);
      aline->Draw();
    }
  }
  
  cCC1->Update();
  app->Run(true);
  graph->Clear();
}


void EventDisplay::end()
{
  delete cCC1;
}
