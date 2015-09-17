// $Id: RawHitMapMaker.C,v 1.10 2010-06-07 11:08:34 mjohn Exp $
// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TH2.h"
#include "TF1.h"

#include "RawHitMapMaker.h"
#include "TestBeamCluster.h"
#include "CommonTools.h"
#include "TDCFrame.h"

using namespace std;

//-----------------------------------------------------------------------------
// Implementation file for class : RawHitMapMaker
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------

double gausaddpol0(double *x, double *par) {
  double x2 = x[0] - par[1];
  double z1 = (0 == par[2]) ? 0 : x2 / par[2];
  return (par[0] * exp(-0.5 * (z1 * z1)) + par[3]);
}

RawHitMapMaker::~RawHitMapMaker(){} 

RawHitMapMaker::RawHitMapMaker(Parameters* p,bool d)
: Algorithm("RawHitMapMaker")
{
  parameters = p;
  display = d;
  CommonTools::defineColorMap();
  
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
}

void RawHitMapMaker::initial()
{
  std::string title = std::string("Chip:  ");
  std::string title2 = std::string("Cor Chip: ");
  std::string title3 = std::string("OL Chip: ");
  std::string name = std::string("hitmap_");
  std::string name2 = std::string("Corr_hitmap_");
  std::string name3 = std::string("OL_hitmap_");
  TH2F* h = 0;
  TH1F* h1 = 0;
  TH2F* h2 = 0;
  TH2F* h3 = 0;
  
  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    h = new TH2F((name + chip).c_str(), (title + chip).c_str(),266,-5.5,260.5,266,-5.5,260.5 );
    hitmap.insert(make_pair(chip, h));
    h2 = new TH2F((name2 + chip).c_str(), (title2 + chip).c_str(),266,-5.5,260.5,266,-5.5,260.5 );
    corr_hitmap.insert(make_pair(chip, h2));
    h3 = new TH2F((name3 + chip).c_str(), (title3 + chip).c_str(),266,-5.5,260.5,266,-5.5,260.5 );
    overlap_hitmap.insert(make_pair(chip, h3));
  }
  for (int i = 0; i < summary->nDetectors(); ++i) {
    std::map<std::string, TH2F*> temp;
    std::map<std::string, TH2F*> temp2;
    std::map<std::string, TH1F*> temp3;
    std::map<std::string, TH1F*> temp4;
    const std::string firstChip = summary->detectorId(i);
    for (int j = 0; j < summary->nDetectors(); ++j) {
      const std::string secondChip = summary->detectorId(j);
      if (!correlationRequested(firstChip, secondChip)) continue;
      name = firstChip + std::string("_vs_") + secondChip;
      h = new TH2F((std::string("row_correlation_") + name).c_str(), (std::string("Row ") + name).c_str(), 33,-5.5,260.5,33,-5.5,260.5);
      temp.insert(make_pair(secondChip, h));
      h = new TH2F((std::string("col_correlation_") + name).c_str(), (std::string("Col ") + name).c_str(), 33,-5.5,260.5,33,-5.5,260.5);
      temp2.insert(make_pair(secondChip, h));
      float min = -180.;
      float max = 180.;
      
      h1 = new TH1F((std::string("row_difference_") + name).c_str(), (std::string("Row ") + name).c_str(), 300, min, max);
      temp3.insert(make_pair(secondChip, h1));
      h1 = new TH1F((std::string("col_difference_") + name).c_str(), (std::string("Col ") + name).c_str(), 300, min, max);  
      temp4.insert(make_pair(secondChip, h1));
    }
    row_difference.insert(make_pair(firstChip, temp3));
    col_difference.insert(make_pair(firstChip, temp4));
    row_correlation.insert(make_pair(firstChip, temp));
    col_correlation.insert(make_pair(firstChip, temp2));
  }
  
  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    if (parameters->alignment.count(chip) <= 0) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: no alignment for " << chip << std::endl;
      return;
    }
    if (summary->detectorId(i) == parameters->dut && 0){
      aligncol.insert(make_pair(std::string(chip), 128-(0.707/0.055)*(parameters->alignment[chip]->displacementX()+parameters->alignment[chip]->displacementY())));
      alignrow.insert(make_pair(std::string(chip), 128 * (sqrt(2.) - 1) + (0.707 / 0.055)*(parameters->alignment[chip]->displacementX()-parameters->alignment[chip]->displacementY())));
    } else {
      aligncol.insert(make_pair(std::string(chip),parameters->alignment[chip]->displacementX() / 0.055));
      alignrow.insert(make_pair(std::string(chip),parameters->alignment[chip]->displacementY() / (-0.055)));
    }
  }
}

void RawHitMapMaker::run(TestBeamEvent* event, Clipboard* clipboard)
{
  TestBeamEventElement *element,*element2;
  for (unsigned int j = 0; j < event->nElements(); ++j) {
    element = event->getElement(j);
    std::vector<RowColumnEntry* >* data = element->data();
    const std::string chip = element->detectorId();
    // Skip masked chips.
    if (parameters->masked[chip]) continue;
    for (std::vector< RowColumnEntry*>::iterator k = data->begin(); k < data->end(); ++k) {
      hitmap[chip]->Fill((*k)->column(), (*k)->row());
    }
  }
  for (unsigned int j = 0; j < event->nElements(); ++j) {
    element = event->getElement(j);
    for (unsigned int jj = 0; jj < event->nElements(); ++jj) {
      element2 = event->getElement(jj);
      const std::string firstChip = element->detectorId();
      const std::string secondChip = element2->detectorId();
      // Skip masked chips.
      if (parameters->masked[firstChip] || parameters->masked[secondChip]) continue;
      if (row_correlation[firstChip][secondChip] == NULL) continue;
      std::vector<RowColumnEntry*>* data = element->data();
      if (data->size() > 256) continue; // huge number of hits, ignore correlations.
      std::vector<RowColumnEntry*>* data2 = element2->data();
      if (data2->size() > 256) continue; // huge number of hits, ignore correlations.
      for (std::vector<RowColumnEntry*>::iterator k = data->begin(); k < data->end(); ++k) {    
        for (std::vector<RowColumnEntry*>::iterator kk = data2->begin(); kk < data2->end(); ++kk) {
          if (firstChip != parameters->dut && secondChip == parameters->dut) {
            row_correlation[firstChip][secondChip]->Fill(rowEff((*k)->row(), (*k)->column()),  (*kk)->row());
            col_correlation[firstChip][secondChip]->Fill(colEff((*k)->row(), (*k)->column()), (*kk)->column());
            row_difference[firstChip][secondChip]->Fill(rowEff((*k)->row(), (*k)->column()) - (*kk)->row());
            col_difference[firstChip][secondChip]->Fill(colEff((*k)->row(), (*k)->column()) - (*kk)->column());
             if(fabs(rowEff((*k)->row(),(*k)->column())-(*kk)->row()-(alignrow[secondChip]-rowEff(alignrow[firstChip],aligncol[firstChip])))<3 && 
               fabs(colEff((*k)->row(),(*k)->column())-(*kk)->column()-(aligncol[secondChip]+0.707*(aligncol[firstChip]-alignrow[firstChip])))<3){
               corr_hitmap[secondChip]->Fill((*kk)->column(), (*kk)->row());
             }
          } else if (firstChip == parameters->dut && secondChip != parameters->dut) {
            row_correlation[firstChip][secondChip]->Fill( (*k)->row(), rowEff((*kk)->row(),(*kk)->column()));
            col_correlation[firstChip][secondChip]->Fill( (*k)->column(), colEff((*kk)->row(),(*kk)->column()));
            row_difference[firstChip][secondChip]->Fill( (*k)->row() - rowEff((*kk)->row(),(*kk)->column()));
            col_difference[firstChip][secondChip]->Fill( (*k)->column() - colEff((*kk)->row(),(*kk)->column() ));
            if(fabs((*k)->row()-rowEff((*kk)->row(),(*kk)->column())+alignrow[firstChip]-rowEff(alignrow[secondChip],aligncol[secondChip]))<4 && 
               fabs((*k)->column()-colEff((*kk)->row(),(*kk)->column())+(aligncol[firstChip]+(0.707*aligncol[secondChip]-0.707*alignrow[secondChip])))<4){
              corr_hitmap[secondChip]->Fill((*kk)->column(), (*kk)->row());
            }
          } else {   
            row_correlation[firstChip][secondChip]->Fill((*k)->row(), (*kk)->row());
            col_correlation[firstChip][secondChip]->Fill((*k)->column(), (*kk)->column());
            row_difference[firstChip][secondChip]->Fill((*k)->row() - (*kk)->row());
            col_difference[firstChip][secondChip]->Fill((*k)->column() - (*kk)->column());
            if(fabs((*k)->row()-(*kk)->row()+(alignrow[secondChip]-alignrow[firstChip]))<6 && 
               fabs((*k)->column()-(*kk)->column()-(aligncol[secondChip]-aligncol[firstChip]))<6){             
              corr_hitmap[secondChip]->Fill((*kk)->column(), (*kk)->row());
            }
          }
        }
      }
    }    
  }
  
  for (unsigned int j = 0; j < event->nElements(); ++j) {
    element = event->getElement(j);
    const std::string firstChip = element->detectorId();
    // Skip masked chips.
    if (parameters->masked[firstChip]) continue;
    std::vector< RowColumnEntry* >* data = element->data();
    std::string chip = element->detectorId();
    for (std::vector< RowColumnEntry*>::iterator k = data->begin(); k < data->end(); ++k) {
      std::vector< RowColumnEntry*>* data = element->data();
      int counter = 0;
      if (data->size() > 256) continue; // huge number of hits, ignore correlations.
      for (unsigned int jj = j; jj < event->nElements(); ++jj) {
        element2 = event->getElement(jj);
        std::vector<RowColumnEntry*>* data2 = element2->data();
        if (data2->size()>256) continue;
        std::string secondChip = element2->detectorId();
        for(std::vector<RowColumnEntry*>::iterator kk = data2->begin();kk < data2->end(); ++kk) {
          if (secondChip == parameters->dut && firstChip != parameters->dut && 0){
            if(fabs(rowEff((*k)->row(),(*k)->column())-(*kk)->row()-(alignrow[secondChip]-rowEff(alignrow[firstChip],aligncol[firstChip])))<3 && 
               fabs(colEff((*k)->row(),(*k)->column())-(*kk)->column()-(aligncol[secondChip]+0.707*(aligncol[firstChip]-alignrow[firstChip])))<3) {
              counter++; 
              break;
            }
          } else if (firstChip == parameters->dut && secondChip != parameters->dut && 0) {
            if (fabs((*k)->row()-rowEff((*kk)->row(),(*kk)->column())+alignrow[firstChip]-rowEff(alignrow[secondChip],aligncol[secondChip]))<4 && 
                fabs((*k)->column()-colEff((*kk)->row(),(*kk)->column())+(aligncol[firstChip]+(0.707*aligncol[secondChip]-0.707*alignrow[secondChip])))<4) {
              counter++;
              break;
            }
          } else if (fabs((*k)->row()-(*kk)->row()+(alignrow[secondChip]-alignrow[firstChip]))<4 && 
                     fabs((*k)->column()-(*kk)->column()-(aligncol[secondChip]-aligncol[firstChip]))<4) { 
            counter++;
            break;
          }
        }     
      }
      if(counter>=7){overlap_hitmap[firstChip]->Fill( (*k)->column() , (*k)->row() );}
      //  if(counter<(4)){(*k)->set_value(-9999);}
    }
  }
  return;
}


void RawHitMapMaker::end()
{
  if (!display || !parameters->verbose) return;
  // HITMAP CANVAS 
  TCanvas* cHM = new TCanvas("cHM", "Raw hitmaps", 720, 720); 
  cHM->cd();
  cHM->Divide(3, 3);
  for (int i = 0; i < summary->nDetectors(); ++i) {
    if (parameters->masked[summary->detectorId(i)]) continue;
    cHM->cd(i + 1);
    TH2F* h = hitmap[summary->detectorId(i)];
    if (h) {
      CommonTools::suppressNoisyPixels(h, 10);
      gStyle->SetOptStat("erm");
      h->DrawCopy("colz");
    } 
  }
  
  // CORR HITMAP CANVAS
  TCanvas *cHMC = new TCanvas("cHMC","Correlated rawhitmaps",720,720);
  cHMC->cd();
  cHMC->Divide(3,3);
  for(int i = 0; i < summary->nDetectors(); ++i) {
    if (parameters->masked[summary->detectorId(i)]) continue;
    cHMC->cd(i + 1);
    TH2F* h2 = corr_hitmap[summary->detectorId(i)];
    if (h2) {
      CommonTools::suppressNoisyPixels(h2, 10);
      gStyle->SetOptStat("erm");
      h2->DrawCopy("colz");
    }
  }
  
  // OVERLAP HITMAP CANVAS
  TCanvas *cHMO = new TCanvas("cHMO", "Hits that overlap in many detectors",720,720);
  cHMO->cd();
  cHMO->Divide(3, 3);
  for (int i = 0; i < summary->nDetectors(); ++i) {
    if (parameters->masked[summary->detectorId(i)]) continue;
    cHMO->cd(i + 1);
    TH2F* h3 = overlap_hitmap[summary->detectorId(i)];
    if (h3) {
      CommonTools::suppressNoisyPixels(h3,10);
      gStyle->SetOptStat("erm");
      h3->DrawCopy("colz");
    }
  }
  
  //CORRELATION CANVAS
  TCanvas *cCM = new TCanvas("cCol", "Detector 1 Correlation", 1200,400);
  cCM->cd();
  cCM->Divide(6, 2);
  int ipad = 1;
  for (int j = 0; j < summary->nDetectors(); ++j) {
    if (parameters->masked[summary->detectorId(j)]) continue;
    for (int k = 0; k < summary->nDetectors(); ++k) {
      if (parameters->masked[summary->detectorId(k)]) continue;
      if(ipad > 6) break;
      if(row_correlation[summary->detectorId(j)][summary->detectorId(k)]){
        cCM->cd(ipad);
        row_correlation[summary->detectorId(j)][summary->detectorId(k)]->SetStats(0);
        row_correlation[summary->detectorId(j)][summary->detectorId(k)]->DrawCopy("box");
        cCM->cd(ipad+6);
        col_correlation[summary->detectorId(j)][summary->detectorId(k)]->SetStats(0);
        col_correlation[summary->detectorId(j)][summary->detectorId(k)]->DrawCopy("box");
        ipad++;
      }
    }
  }
  
  TCanvas* cdiff = new TCanvas("cdiff", "Detector 1 diff", 1200,400);
  cdiff->cd();
  cdiff->Divide(6, 2);
  ipad = 1;
  for (int j = 0; j < summary->nDetectors(); ++j) {
    if (parameters->masked[summary->detectorId(j)]) continue;
    for (int k = 0; k < summary->nDetectors(); ++k) {
      if (parameters->masked[summary->detectorId(k)]) continue;
      if(ipad > 6) break;
      if(row_difference[summary->detectorId(j)][summary->detectorId(k)]){
        cdiff->cd(ipad);
        row_difference[summary->detectorId(j)][summary->detectorId(k)]->SetStats(0);
        double mu = row_difference[summary->detectorId(j)][summary->detectorId(k)]->GetMean();
        double sigma = row_difference[summary->detectorId(j)][summary->detectorId(k)]->GetRMS();
        double iniParams[4] = {6000, mu, 0.1*sigma, 1000};
        double hiParams[4] = {40000, mu+20*sigma, 1*sigma,3000};
        double lowParams[4] = {0,mu-20*sigma, 0.001*sigma,0};
        
        TF1* func = new TF1("func", gausaddpol0, mu-5*sigma, mu+5*sigma,4);
        for(int i=0; i<4; i++){
          func->SetParameter(i, iniParams[i]);
          func->SetParLimits(i,lowParams[i],hiParams[i]);
        }
        
        //row_difference[summary->detectorId(j)][summary->detectorId(k)]->Fit(func,"R");
        row_difference[summary->detectorId(j)][summary->detectorId(k)]->DrawCopy();
        
        cdiff->cd(ipad+6);
        col_difference[summary->detectorId(j)][summary->detectorId(k)]->SetStats(0);
        
        double mu2 = col_difference[summary->detectorId(j)][summary->detectorId(k)]->GetMean();
        double sigma2 = col_difference[summary->detectorId(j)][summary->detectorId(k)]->GetRMS();
        double iniParams2[4] = {6000, mu2, 0.1*sigma2, 1000};
        double hiParams2[4] = {40000, mu2+5*sigma2, sigma2,3000};
        double lowParams2[4] = {0,mu2-5*sigma2, 0.005*sigma2,0};
        TF1 *func2 = new TF1("func2", gausaddpol0, mu2-0.2*sigma2, mu2+0.2*sigma2,4);
        for(int h=0; h<4; h++){
          func2->SetParameter(h, iniParams2[h]);
          func2->SetParLimits(h,lowParams2[h],hiParams2[h]);
        }
        //col_difference[summary->detectorId(j)][summary->detectorId(k)]->Fit(func2);
        col_difference[summary->detectorId(j)][summary->detectorId(k)]->DrawCopy();
        ++ipad;
      }
    }
  }
  
}

float RawHitMapMaker::rowEff(float row, float col)
{
  return row;
  //return 0.707*row+0.707*col;
}

float RawHitMapMaker::colEff(float row, float col)
{
  return col;
  //return 256-(0.707*col-0.707*row);
}


bool RawHitMapMaker::correlationRequested(std::string firstChip, std::string secondChip)
{
  if (firstChip == std::string("C03-W0015")) {
    if (secondChip != std::string("C03-W0015")) {
      return true;
    }
  }
  return true;
}

