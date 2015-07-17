// $Id: NClustersMonitor.C,v 1.5 2009-08-13 16:52:10 mjohn Exp $
// Include files 
#include <iostream>
#include <stdlib.h>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"

#include "Clipboard.h"
#include "NClustersMonitor.h"
#include "TestBeamCluster.h"

//-----------------------------------------------------------------------------
// Implementation file for class : NClustersMonitor
//-----------------------------------------------------------------------------

NClustersMonitor::~NClustersMonitor(){} 

NClustersMonitor::NClustersMonitor(Parameters* p,bool d)
  : Algorithm("NClustersMonitor")
{
  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
}

void NClustersMonitor::initial()
{
  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    if (parameters->masked[chip]) continue;
    
    // declare clusterTime histograms
    std::string title = std::string("N(clusters) vs. time for ") + chip;
    std::string name = std::string("clustertimes_") + chip;
    TH1F* h = new TH1F(name.c_str(),title.c_str(),
                       100 * (int(summary->lastTime() - summary->firstTime()) + 10),
                       -5, summary->lastTime() - summary->firstTime() + 5);
    clusterTimes.insert(make_pair(chip, h));
    
    // declare nClusters histograms
    title = std::string("N(clusters) for ") + chip + std::string(" [dotted: N(hits)]");
    name = std::string("ncluster_") + chip;
    h = new TH1F(name.c_str(), title.c_str(), 100, 0.5, 500.5);
    nClusters.insert(make_pair(chip, h));
    
    // declare nHits histograms
    title = std::string("N(clusters) for ") + chip + std::string(" [dotted: N(hits)]");
    name = std::string("nhits_") + chip;
    h = new TH1F(name.c_str(), title.c_str(), 100, 0.5, 500.5);
    nHits.insert(make_pair(chip, h));

    numClusters.insert(make_pair(chip, 0));
    numHits.insert(make_pair(chip, 0));
  }
}

void NClustersMonitor::run(TestBeamEvent *event, Clipboard *clipboard )
{
  event->doesNothing();
  for(int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    if (parameters->masked[chip]) continue;
    numClusters[chip] = 0;
    numHits[chip] = 0;
  }

  TestBeamClusters* clusters = (TestBeamClusters*)clipboard->get("Clusters");
  if (!clusters) {
    std::cerr << m_name << std::endl;
    std::cerr << "    ERROR: no clusters on clipboard" << std::endl;
    return;
  }
  TestBeamClusters::iterator iter = clusters->begin();
  for(; iter != clusters->end(); ++iter) {
    TestBeamCluster* cluster = *iter;
    const std::string chip = cluster->element()->detectorId();
    clusterTimes[chip]->Fill(cluster->element()->timeStamp() - summary->firstTime());
    numHits[chip] += cluster->size();
    ++numClusters[chip];
  }

  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    nClusters[chip]->Fill(numClusters[chip]);
    nHits[chip]->Fill(numHits[chip]);
  }
}


void NClustersMonitor::end()
{
  if (!display || !parameters->verbose) return;
  TCanvas* cCM = new TCanvas("cCM", "VELO Timepix testbeam", 1000, 500);
  cCM->cd();
  std::vector<TPad*> pads;
  const int n = summary->nDetectors();
  std::cout << m_name << std::endl;
  for (int i = 0; i < n; ++i) {
    const std::string chip = summary->detectorId(i);
    if (parameters->masked[chip]) continue;
    float xfrac = (i % 4) / 4.;
    float yfrac = 1 - ((i / 4) + 1) / 2.;
    std::string name = std::string("pad_") + chip;
    pads.push_back(new TPad(name.c_str(), "Pad", 0.01 + xfrac, 0.01 + yfrac, 0.24 + xfrac, 1 / 2. + yfrac));
    cCM->cd();
    pads[i]->Draw();
    pads[i]->cd();
    nClusters[chip]->SetLineColor(i + 2);
    clusterTimes[chip]->SetNdivisions(505);
    nClusters[chip]->DrawCopy();
    nHits[chip]->SetLineStyle(3);
    nHits[chip]->DrawCopy("same");
    std::cout << "    " << chip << ": ave N(clusters)=" << nClusters[chip]->GetMean()
                                << ", ave N(hits)=" << nHits[chip]->GetMean() << std::endl;
  }
  TCanvas* cCM2 = new TCanvas("cCM2", "VELO Timepix testbeam", 1200, 768);
  cCM2->cd();
  for (int i = 0; i < n; ++i) {
    const std::string chip = summary->detectorId(i);
    if (parameters->masked[chip]) continue;
    const float yfrac = 1 - ((i % n) / float(n));
    const std::string name = std::string("padT_") + chip;
    pads.push_back(new TPad(name.c_str(), "Pad", 0.01, yfrac - 1 / float(n) + 0.01, 0.99, yfrac - 0.02));
    cCM2->cd();
    pads[i + summary->nDetectors()]->Draw();
    pads[i + summary->nDetectors()]->cd();
    clusterTimes[chip]->SetLineColor(i + 2);
    clusterTimes[chip]->SetTitle("");
    clusterTimes[chip]->SetStats(0);
    clusterTimes[chip]->GetYaxis()->SetNdivisions(1);
    clusterTimes[chip]->GetXaxis()->SetLabelSize(0.15);
    clusterTimes[chip]->SetMinimum(1);
    clusterTimes[chip]->GetYaxis()->SetLabelSize(0.15);
    clusterTimes[chip]->GetXaxis()->SetTitle("seconds since beginnning of the run");
    clusterTimes[chip]->GetXaxis()->SetTitleSize(0.05);
    clusterTimes[chip]->DrawCopy();
  }
}

