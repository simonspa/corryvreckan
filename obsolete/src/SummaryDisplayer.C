// $Id: SummaryDisplayer.C,v 1.10 2010/06/16 23:33:02 mjohn Exp $
// Include files 
#include <iomanip>

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TExec.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "THStack.h"
#include "THashList.h"
#include "TObjString.h"

// local
#include "SummaryDisplayer.h"

//-----------------------------------------------------------------------------
// Implementation file for class : SummaryDisplayer
//
// 2009-06-28 : Malcolm John
//-----------------------------------------------------------------------------

//=============================================================================
// Standard constructor, initializes variables
//=============================================================================
SummaryDisplayer::SummaryDisplayer(Parameters *p) 
{
  parameters=p;
  verbose=p->verbose;
  display();
}

void SummaryDisplayer::display(){
  TFile *inputFile=TFile::Open(parameters->eventFile.c_str(),"READ");
  if(inputFile==NULL) return;
  TTree *tbtree=(TTree*)inputFile->Get("tbtree");
  if(tbtree==NULL) return;

  summary=(TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);

  if (verbose) {
    std::cout << "\n ZS file summary" << std::endl;
    std::cout << "----------------" << std::endl << std::endl;
    std::cout << "     Detector     Clock    THL   Hits/Frame\n";
    for (int i = 0; i < summary->nDetectors(); i++) {
      const std::string id = summary->detectorId(i);
      const int clock = summary->clockForDetector(id);
      const int thl = summary->thresholdForDetector(id);
      std::cout << " (" << i + 1 << ")  " << id << "   "
        << std::setw(4) << clock << "   "
        << std::setw(5) << thl << "    ";
      TH1F* hHits = summary->nHitsPerEvent(id);
      if (hHits == NULL) {
        std::cout << "WARNING: zero hits above threshold";
      } else {
        std::cout << std::setprecision(4) << hHits->GetMean();
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  float yleg=0.1+0.08*(summary->nDetectors()<10?10-summary->nDetectors():0);
  TLegend *l=new TLegend(0.4,yleg,0.99,0.92);
  l->SetFillColor(10);
  TH1F *h = new TH1F("h","Frame count vs. detector ID",10,0,10);
  h->SetBit(TH1::kCanRebin);
  h->SetStats(0);
  TH1F* g=(TH1F*)h->Clone("g");

  THStack *stack_L = new THStack("stack_L","DAQ Time (wrt first file)");
  THStack *stack_S = new THStack("stack_S","DAQ Time (wrt first file)");
  THStack *stack_R = new THStack("stack_R","DAQ Time (wrt DUT)");
  THStack *stack_N = new THStack("stack_N","Number of hits/events");
  THStack *stack_C = new THStack("stack_C","Number of files/events");
  THStack *stack_F = new THStack("stack_F","Number of frames/spill");
  THStack *stack_T = new THStack("stack_T","Time since last frame");
  THStack *stack_U = new THStack("stack_U","N_fr(timepix)-N_fr(tdc)/spill");

  std::map<std::string,int> chipColor;

  TH1D *h_L=0; TH1D *h_S=0; TH1F *h_N=0; TH1F *h_C=0;
  TH1D *h_R=0; TH1F *h_F=0; TH1F *h_T=0; TH1F *h_U=0;
  for(int i=0;i<summary->nDetectors();i++) {
    chipColor.insert(make_pair(summary->detectorId(i),i+2));
    h_L = (TH1D*)summary->jitterPlot_L(summary->detectorId(i));
    h_S = (TH1D*)summary->jitterPlot_S(summary->detectorId(i));
    h_R = (TH1D*)summary->jitterPlot_Rel(summary->detectorId(i));
    h_N = (TH1F*)summary->nHitsPerEvent(summary->detectorId(i));
    h_C = (TH1F*)summary->nFilesPerEvent(summary->detectorId(i));
    h_F = (TH1F*)summary->framesPerSpill(summary->detectorId(i));
    h_T = (TH1F*)summary->timeSinceLast(summary->detectorId(i));
    h_U = (TH1F*)summary->tdcTimepixDiff(summary->detectorId(i));

    if(h_L!=NULL) {
      h_L->SetFillColor(chipColor[summary->detectorId(i)]);
      h_S->SetFillColor(chipColor[summary->detectorId(i)]);
      h_R->SetFillColor(chipColor[summary->detectorId(i)]);
      h_N->SetFillColor(chipColor[summary->detectorId(i)]);
      h_C->SetFillColor(chipColor[summary->detectorId(i)]);
      h_F->SetFillColor(chipColor[summary->detectorId(i)]);
      h_T->SetFillColor(chipColor[summary->detectorId(i)]);
      h_U->SetFillColor(chipColor[summary->detectorId(i)]);
      stack_L->Add(h_L);
      stack_S->Add(h_S);
      stack_R->Add(h_R);
      stack_N->Add(h_N);
      stack_C->Add(h_C);
      stack_F->Add(h_F);
      stack_T->Add(h_T);
      stack_U->Add(h_U);
      l->AddEntry(h_L,summary->detectorId(i).c_str(),"f");
      std::string label=summary->detectorId(i)+std::string("  (")+summary->source(i)+std::string(")  ");
      h->Fill(label.c_str(),summary->nFilesForDetector(summary->detectorId(i)) );
      int nbins=summary->nFilesPerEvent(summary->detectorId(i))->GetXaxis()->GetNbins();
      g->Fill(label.c_str(),summary->nFilesPerEvent(summary->detectorId(i))->Integral(2,nbins));//bin 1 = 0 files, which we exclude
    }
  }

  if(summary->hNumberPairs->GetEntries()){
    gStyle->SetOptStat(111110);
    TCanvas *canvTdc = new TCanvas("canvTdc","TDC information",1200,600);
    canvTdc->Divide(2);
    TPad* lp=(TPad*)canvTdc->cd(1);
    lp->Divide(1,2);
    TPad* lup=(TPad*)lp->cd(1);
    lup->Divide(2);
    lup->cd(1);
    summary->hNumberSync->SetLineColor(2);
    summary->hNumberUnsync->SetLineColor(4);
    TH1F* h0 = 0;
    TH1F* h1 = summary->hNumberPairs;
    TH1F* h2 = summary->hNumberSync;
    TH1F* h3 = summary->hNumberUnsync;
    if(h2->GetMaximum()>h1->GetMaximum()){h0=h1;h1=h2;h2=h0;}
    if(h3->GetMaximum()>h2->GetMaximum()){h0=h2;h2=h3;h3=h0;}
    if(h2->GetMaximum()>h1->GetMaximum()){h0=h1;h1=h2;h2=h0;}
    h1->Draw(); 
    h2->Draw("same"); 
    h3->Draw("same");
    TLegend *lN=new TLegend(0.69,0.69,0.99,0.99);
    lN->SetFillColor(0);
    lN->AddEntry(summary->hNumberSync,"Sync","l");
    lN->AddEntry(summary->hNumberUnsync,"UnSync","l");
    lN->AddEntry(summary->hNumberPairs,"Paired","l");
    lN->Draw();    
    lup->cd(2)->SetLogy();
    summary->hExtraSyncTriggers->Scale(1./summary->hExtraSyncTriggers->GetEntries());
    summary->hExtraUnsyncTriggers->Scale(1./summary->hExtraUnsyncTriggers->GetEntries());
    summary->hExtraSyncTriggers->Draw();
    summary->hExtraSyncTriggers->SetStats(0);
    summary->hExtraSyncTriggers->SetLineColor(2);
    summary->hExtraUnsyncTriggers->Draw("same");
    summary->hExtraUnsyncTriggers->SetLineColor(4);
    TLegend *lE=new TLegend(0.69,0.69,0.99,0.99);
    lE->SetFillColor(0);
    lE->AddEntry(summary->hExtraUnsyncTriggers,"UnSync","l");
    lE->AddEntry(summary->hExtraSyncTriggers,"Sync","l");
    lE->Draw();
    TPad* lbp=(TPad*)lp->cd(2);
    lbp->Divide(3);
    lbp->cd(1);
    if(summary->hFEI4FractionMatched){
      summary->hFEI4FractionMatched->Draw();
    }
    lbp->cd(2);
    if(summary->hFEI4TimeDiff){
      summary->hFEI4TimeDiff->Draw();
      summary->hFEI4TimeDiff->GetXaxis()->SetTitle("ns");
    }else{
      summary->hUnmatched->Draw();
    }
    lbp->cd(3);
    stack_U->Draw();
    TPad* rp=(TPad*)canvTdc->cd(2);
    rp->Divide(1,3);
    TPad* rup = (TPad*)rp->cd(1);
    rup->Divide(2);
    rup->cd(1)->SetLogy();
    TAxis* axis = summary->hTDCShutterTime->GetXaxis();
    int tot = 0;
    int lowerbin = 0;
    for(int i=1;i<axis->GetNbins();i++){
      tot += (int)summary->hTDCShutterTime->GetBinContent(i);
      if (tot > 1) {
        lowerbin = i;
        break;
      }
    }
    tot = 0;
    int higherbin=axis->GetNbins();
    for(int i=axis->GetNbins();i!=0;i--){
      tot += (int)summary->hTDCShutterTime->GetBinContent(i);
      if(tot>0.005*summary->hTDCShutterTime->GetEntries()){
        higherbin=i;
        break;
      }
    }
    TH1F* hTDCShutterTime = new TH1F("hTDCShutterTime2",summary->hTDCShutterTime->GetTitle()
        ,100,0,axis->GetBinCenter(higherbin));
    for(int i=0;i<=axis->GetNbins();i++){
      for(int j=0;j<summary->hTDCShutterTime->GetBinContent(i);j++){
        hTDCShutterTime->Fill(summary->hTDCShutterTime->GetBinCenter(i));
      }
    }
    //TH1F* hTDCShutterTime=summary->hTDCShutterTime;
    hTDCShutterTime->GetXaxis()->SetNdivisions(5);
    hTDCShutterTime->GetXaxis()->SetTitle("#mus");
    hTDCShutterTime->GetXaxis()->SetTitleOffset(0.5);
    hTDCShutterTime->GetXaxis()->SetTitleSize(0.07);
    hTDCShutterTime->GetXaxis()->SetLabelSize(0.07);
    hTDCShutterTime->GetYaxis()->SetTitleSize(0.07);
    hTDCShutterTime->GetYaxis()->SetLabelSize(0.07);
    hTDCShutterTime->Draw();


    rup->cd(2)->SetLogy();
    TH1F* hTDCUnsyncTriggerTime = new TH1F("hTDCUnsyncTriggerTime2",summary->hTDCUnsyncTriggerTime->GetTitle()
        ,100,0,axis->GetBinCenter(higherbin));
    for(int i=0;i<=axis->GetNbins();i++){
      for(int j=0;j<summary->hTDCUnsyncTriggerTime->GetBinContent(i);j++){
        hTDCUnsyncTriggerTime->Fill(summary->hTDCUnsyncTriggerTime->GetBinCenter(i));
      }
    }
    //TH1F* hTDCUnsyncTriggerTime=summary->hTDCUnsyncTriggerTime;
    hTDCUnsyncTriggerTime->GetXaxis()->SetNdivisions(5);
    hTDCUnsyncTriggerTime->GetXaxis()->SetTitle("#mus");
    hTDCUnsyncTriggerTime->GetXaxis()->SetTitleOffset(0.5);
    hTDCUnsyncTriggerTime->GetXaxis()->SetTitleSize(0.07);
    hTDCUnsyncTriggerTime->GetXaxis()->SetLabelSize(0.07);
    hTDCUnsyncTriggerTime->GetYaxis()->SetTitleSize(0.07);
    hTDCUnsyncTriggerTime->GetYaxis()->SetLabelSize(0.07);
    hTDCUnsyncTriggerTime->Draw();


    rp->cd(2);
    summary->hSynchronisationDelay->Draw();
    summary->hSynchronisationDelay->GetXaxis()->SetTitle("ns");

    TPad* rqp = (TPad*)rp->cd(3);
    rqp->Divide(2);
    rqp->cd(1);
    if(summary->hTDCTime){
      summary->hVetraTime->SetStats(0);
      summary->hVetraTime->SetMaximum(6);
      summary->hVetraTime->SetLineColor(1);
      summary->hVetraTime->SetFillColor(1);
      summary->hVetraTime->Draw("hist");
      summary->hVetraTime->GetXaxis()->SetTitle("seconds");
      summary->hMedipixTime->SetLineColor(4);
      summary->hMedipixTime->SetFillColor(4);
      summary->hMedipixTime->Draw("hist same");
      summary->hTDCTime->SetLineColor(2);
      summary->hTDCTime->SetFillColor(2);
      summary->hTDCTime->Draw("hist same");
      TLegend *l2=new TLegend(0.4,0.59,0.6,0.89);
      l2->SetFillColor(0);
      l2->SetLineColor(0);
      l2->AddEntry(summary->hVetraTime,"DUT","f");
      l2->AddEntry(summary->hTDCTime,"TDC","f");
      l2->AddEntry(summary->hMedipixTime,"Timepix","f");
      l2->Draw();
    }
    rqp->cd(2);
    if(summary->hFEI4MatchTime){
      summary->hFEI4MatchTime->Draw();
    }

    canvTdc->Update();
    if(parameters->printSummaryDisplay){
      canvTdc->Print("TDCPlots.png");
      canvTdc->Print("TDCPlots.pdf");
      canvTdc->Print("TDCPlots.eps");
    }
  }
  if(h_L==0){
    std::cout << "Too few data here. Abandoning timepix plots." << std::endl;
    return;
  }

  TCanvas *c1 = new TCanvas("c1","VELO Timepix testbeam",300,300,1200,600);
  c1->cd();
  TPad *padLUL = new TPad("padLUL","Pad",0.01,0.51,0.24,0.99);
  TPad *padLUR = new TPad("padLUR","Pad",0.26,0.51,0.49,0.99);
  TPad *padLDL = new TPad("padLDL","Pad",0.01,0.01,0.24,0.49);
  TPad *padLDR = new TPad("padLDR","Pad",0.26,0.01,0.49,0.49);
  TPad *padRUL = new TPad("padRUL","Pad",0.51,0.51,0.74,0.99);
  TPad *padRUR = new TPad("padRUR","Pad",0.76,0.51,0.99,0.99);
  TPad *padRDL = new TPad("padRDL","Pad",0.51,0.01,0.74,0.49);
  TPad *padRDR = new TPad("padRDR","Pad",0.76,0.01,0.99,0.49);
  padLUL->Draw();
  padLUR->Draw();
  padLDL->Draw();
  padLDR->Draw();
  padRUL->Draw();
  padRUR->Draw();
  padRDL->Draw();
  padRDR->Draw();
  padLUL->cd();
  stack_L->Draw();
  stack_L->GetXaxis()->SetTitle(h_L->GetXaxis()->GetTitle());
  l->Draw();
  padLUL->Update();
  padLUR->cd();
  //padLUR->SetLogy();
  stack_T->Draw();
  stack_T->GetXaxis()->SetTitle("milliseconds");
  stack_T->Draw();
  padLDL->cd();
  //padLDL->SetLogy();
  /* superfluous can be replaced...
     stack_R->Draw();
     stack_R->GetXaxis()->SetTitle(h_R->GetXaxis()->GetTitle());
     stack_R->Draw();
   */
  padLDR->cd();
  summary->detectorElementsWithHit()->SetTitle("N(frames with >=1 hit pixels)");
  summary->detectorElementsWithHit()->GetXaxis()->SetTitle("N(detectors)");
  summary->detectorElementsWithHit()->Draw();
  padRDR->cd();
  padRDR->SetBottomMargin(0.5);
  h->LabelsDeflate("X");
  h->LabelsOption("v");
  h->GetXaxis()->SetLabelSize(h->GetXaxis()->GetLabelSize()*1.8);
  h->SetLineWidth(3);
  h->SetLineStyle(3);
  h->Draw("hist");
  h->SetMinimum(0);
  THashList *labels = g->GetXaxis()->GetLabels();
  for (int i=0;i<g->GetXaxis()->GetNbins();i++){
    TObjString* pLabel = (TObjString*)labels->At(i);
    if(pLabel==NULL) continue;
    TString label(pLabel->GetString()(0,pLabel->GetString().First("(")-2));
    g->SetFillColor(chipColor[std::string(label)]);
    g->GetXaxis()->SetRange(i+1,g->GetXaxis()->GetNbins());
    g->DrawCopy("same");
  }
  h->Draw("same hist");
  padRUL->cd();
  padRUL->SetLogy();
  stack_N->Draw();
  stack_N->SetTitle(h_N->GetTitle());
  stack_N->GetXaxis()->SetTitle(h_N->GetXaxis()->GetTitle());
  stack_N->Draw();
  padRUR->cd();
  stack_C->Draw();
  stack_C->SetTitle(h_C->GetTitle());
  stack_C->GetXaxis()->SetTitle(h_C->GetXaxis()->GetTitle());
  stack_C->Draw();
  padRDL->cd();
  stack_F->Draw();
  c1->Update();
  if(parameters->printSummaryDisplay){
    c1->Print("SummaryPlots.png");
    c1->Print("SummaryPlots.pdf");
    c1->Print("SummaryPlots.eps");
  }
}
//=============================================================================
// Destructor
//=============================================================================
SummaryDisplayer::~SummaryDisplayer() {} 

//=============================================================================
