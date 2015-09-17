///////////////////////////////////////////////////////////////////////////////
#include "LanGau.h"
#include "TFile.h"
#include "TMath.h"
///////////////////////////////////////////////////////////////////////////////
TF1* LanGau::FitLanGau(TH1F* h1) {

  TF1* pLanGau = new TF1("pLanGau",LanGauFun,-50.,250.,4);
  //   TF1* pLanGau = new TF1("pLanGau",LandauFun,-50.,200.,4);
   pLanGau->SetParNames("Width","MPV","Area","GSigma");


  float mean,meanerr;
  float width,widtherr;
  float gsigma,gsigmaerr;
  Int_t nBinsX=h1->GetXaxis()->GetNbins();
  Double_t startFit = h1->GetMean() - 3.*h1->GetRMS();
  Double_t endFit = h1->GetMean() + 3.*h1->GetRMS();

  pLanGau->SetRange(startFit,endFit);
  pLanGau->SetParameters(0.1,
 			 h1->GetMean(),
 			 h1->Integral(1,nBinsX),
 			 h1->GetRMS());
// 			 //			 h1->GetRMS() );

  pLanGau->SetParLimits(0,  0.1, 25.);
  pLanGau->SetParLimits(1, -50., 200.);
  pLanGau->SetParLimits(2,   0., 10000000.);
  pLanGau->SetParLimits(3,  0., 5.);

  //===fit Landau + Gaus
  h1->Fit("pLanGau","Rq0");
 
  h1->DrawCopy();

  mean = pLanGau->GetMaximumX(startFit,endFit);
  Double_t MPV=pLanGau->GetParameter(1);
  meanerr=pLanGau->GetParError(1);
  width=pLanGau->GetParameter(0);
  widtherr=pLanGau->GetParError(0); 
  gsigma=pLanGau->GetParameter(3);
  gsigmaerr=pLanGau->GetParError(3);

  Double_t chi2 = pLanGau->GetChisquare();
  Double_t nDOF = pLanGau->GetNDF();
  Double_t maxVal = pLanGau->GetMaximum();

  std::cout << "FitLanGau: Chi2 is " << chi2 << "/" << nDOF << std::endl;
  std::cout << "   Mean="<< mean << "+/-" << meanerr
	    << " Mean-MPV = " << mean-MPV
	    << " width=" << width << "+/-" << widtherr
	    <<  " gsigma=" << gsigma << "+/-" << gsigmaerr
	    <<  " and max=" << maxVal
	    << std::endl;

  return pLanGau;

}
///////////////////////////////////////////////////////////////////////////////
TF1* LanGau::FitLanGauPau(TH1F* h1) {

  //  TF1* pLanGau = new TF1("pLanGauPau",LanGauPauFun,-50.,200.,5);
  TF1* pLanGau = new TF1("pLanGauPau",LanPauFun,-50.,200.,5);
  pLanGau->SetParNames("MPV","Width1","Width2", "Area", "GaussWidth");

  float mean=0.;
  float meanerr=0.;;
  float width1=0.;
  float width1err=0.;
  float width2=0.;
  float width2err=0.;
  Int_t nBinsX=h1->GetXaxis()->GetNbins();
  Double_t startFit = h1->GetMean() - 2.*h1->GetRMS();
  Double_t endFit = h1->GetMean() + 3.*h1->GetRMS();

  pLanGau->SetRange(startFit,endFit);

  pLanGau->SetParameters(h1->GetMean(),
			 0.5*h1->GetRMS(),
			 0.5*h1->GetRMS(),
			 h1->Integral(1,nBinsX),
			 h1->GetRMS());

  pLanGau->SetParLimits(0, -50., 200.);
  pLanGau->SetParLimits(1,  0., 50.);
  pLanGau->SetParLimits(2,  0., 10.);
  pLanGau->SetParLimits(3,   0., 10000000.);
  pLanGau->SetParLimits(4,  0., 2.);


  //===fit Landau + Gaus
  h1->Fit("pLanGauPau","Rq");
 
  h1->DrawCopy();

  mean = pLanGau->GetMaximumX(startFit,endFit);
  Double_t MPV=pLanGau->GetParameter(0);
  meanerr=pLanGau->GetParError(0);
  width1=pLanGau->GetParameter(1);
  width2=pLanGau->GetParameter(2);
  width1err=pLanGau->GetParError(1); 
  width2err=pLanGau->GetParError(2); 

  Double_t chi2 = pLanGau->GetChisquare();
  Double_t nDOF = pLanGau->GetNDF();
  Double_t maxVal = pLanGau->GetMaximum();

  std::cout << "FitLanGau: Chi2 is " << chi2 << "/" << nDOF << std::endl;
  std::cout << "   Max ="<< mean << "+/-" << meanerr
	    << " MPV = " << MPV
	    << " width1 =" << width1 << "+/-" << width1err
	    << " width1 =" << width2 << "+/-" << width2err
	    << " Gaussian sigma = " << pLanGau->GetParameter(4)
	    << "+/-" << pLanGau->GetParError(4)
	    <<  " and max=" << maxVal
	    << std::endl;

  return pLanGau;

}
///////////////////////////////////////////////////////////////////////////////
void LanGau::FitLanGau(const Text_t *filename, const Text_t *histname) {


  TFile* ff = TFile::Open(filename);
  TH1F *h1 = (TH1F *) ff->Get(histname);

  FitLanGau(h1);

  return;

}
///////////////////////////////////////////////////////////////////////////////
void LanGau::FitSliceLanGau(const TString & filename, 
			    const TString & histname, 
			    const TString & outFileName) {

  TFile *ff=TFile::Open(filename);
  TH2F *h2 = (TH2F *) ff->Get(histname);
  std::cout <<"calling FitSliceLanGau to open file " 
	    << outFileName << " histogram " << histname << std::endl;
  FitSliceLanGau(outFileName, h2);

  return;

}
///////////////////////////////////////////////////////////////////////////////
void LanGau::FitSliceLanGau(const TString & outFileName, TH2F* histtofit) {

  TH1F* hnew=0;
  TF1* pLanGau=0;

  float mean=0.;
  float meanerr=0.;
  float width=0.;
  float widtherr=0.;
  float gsigma=0.;
  float gsigmaerr=0.;
  float chi2=0.;
  float nDOF=0.;

  TH2F *h2 = histtofit;

  Int_t nbinsx=h2->GetXaxis()->GetNbins();
  Int_t nbinsy=h2->GetYaxis()->GetNbins();
  Float_t lowx=h2->GetXaxis()->GetXmin();
  Float_t hix=h2->GetXaxis()->GetXmax();
  Float_t lowy=h2->GetYaxis()->GetXmin();
  Float_t hiy=h2->GetYaxis()->GetXmax();

  std::cout << "LanGau::FitSliceLanGau: "
	    << "nbinsx=" << nbinsx << " nbinsy=" << nbinsy 
	    << " lowx=" << lowx  << " hix=" << hix 
	    << " lowy=" << lowy << " hiy=" << hiy 
	    << std::endl;


  gROOT->cd();
  //  std::cout << "hello 1 " << std::endl;
  TH1F *hmean   = new TH1F("hmean","hmean",nbinsx,lowx,hix);
  TH1F *MPV     = new TH1F("MPV","MPV",nbinsx,lowx,hix);
  TH1F *hwidth  = new TH1F("hwidth","hwidth",nbinsx,lowx,hix);
  TH1F *hGSigma = new TH1F("hGSigma","hGSigma",nbinsx,lowx,hix);
  TH1F* hChi2   = new TH1F("hChi2","hChi2",nbinsx,lowx,hix);
  TH1F *hproj   = new TH1F("hproj","hproj",nbinsy,lowy,hiy);
  Int_t iy=0;
  //
  for (iy=1;iy<nbinsy+1;iy++){
    hproj->SetBinContent(iy,0.);    
  }
  // Creating an array for the slice histograms
  TObjArray Hlist(0);
  // Looping over slices
  for (Int_t ix=1;ix<nbinsx+1;ix++){
    hproj->Reset();
    for (iy=1;iy<nbinsy+1;iy++){
      Float_t yval = h2->GetYaxis()->GetBinCenter(iy);
      Int_t ientMax=(int)h2->GetCellContent(ix,iy);
      for (Int_t ient=0; ient<ientMax; ient++){
	hproj->Fill(yval,1.);
      }
      
    }
    if (hproj->GetEntries()>20.){

      pLanGau = FitLanGau(hproj);
      //      pLanGau = FitLanGauPau(hproj);

      hnew = (TH1F*)hproj->Clone();
      char names[100], titles[200];
      sprintf(names,"ADCSlice%d",ix);
      sprintf(titles,"ADC slice from bin no. %d",ix);
      hnew->SetName(names);
      hnew->SetTitle(titles);
      Hlist.Add(hnew);
      //==== calculate the MPValue for the covoluted function
      Double_t startFit = hproj->GetMean() - 3.0*hproj->GetRMS();
      Double_t endFit = hproj->GetMean() + 3.0*hproj->GetRMS();
      mean = pLanGau->GetMaximumX(0.75*startFit,0.75*endFit);
      double mostProb=pLanGau->GetParameter(1);
      double mostProbError=pLanGau->GetParError(1);
      width=pLanGau->GetParameter(2);
      widtherr=pLanGau->GetParError(2); 
      //      meanerr=pLanGau->GetParError(1);
      meanerr=pLanGau->GetParError(1);
      gsigma=pLanGau->GetParameter(3);
      gsigmaerr=pLanGau->GetParError(3); 
      chi2 = pLanGau->GetChisquare();
      nDOF = pLanGau->GetNDF();

      hmean->SetBinContent(ix,mean);
      hmean->SetBinError(ix,meanerr);
      MPV->SetBinContent(ix, mostProb);
      MPV->SetBinError(ix,mostProbError);
      hwidth->SetBinContent(ix,width);
      hwidth->SetBinError(ix,widtherr);
      hGSigma->SetBinContent(ix,gsigma);
      hGSigma->SetBinError(ix,gsigmaerr);
      hChi2->SetBinContent(ix, chi2/nDOF);
      
    }
  }

  std::cout << "Trying to write file " << outFileName << std::endl;
  TFile f(outFileName,"recreate");
  Hlist.Write();
  hmean->SetLineColor(2);
  hmean->SetLineWidth(2);
  hmean->Write();
  MPV->SetLineColor(4);
  MPV->SetLineWidth(4);
  MPV->Write();
  hwidth->Write();
  hGSigma->Write();
  hChi2->Write();

  f.Close();
  delete pLanGau;
  delete hmean;
  delete hwidth;
  delete hGSigma;
  delete hproj;

}
///////////////////////////////////////////////////////////////////////////////
Int_t LanGau::LanGauPro(Double_t *params, Double_t& maxx, Double_t& FWHM) {

  // Seaches for the location (x value) at the maximum of the
  // Landau-Gaussian convolute and its full width at half-maximum.
  //
  // The search is probably not very efficient, but it's a first try.

  Double_t p,x,fy,fxr,fxl;
  Double_t step;
  Double_t l,lold;
  Int_t i = 0;
  Int_t MAXCALLS = 100;

  // Search for maximum

  p = params[1] - 0.1 * params[0];
  step = 0.05 * params[0];
  lold = -2.0;
  l    = -1.0;

  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;

    lold = l;
    x = p + step;
    l = LanGauFun(&x,params);

    if (l < lold)
      step = -step/10;

    p += step;
  }

  if (i == MAXCALLS)
    return (-1);

  maxx = x;

  fy = l/2;

  // Search for right x location of fy

  p = maxx + params[0];
  step = params[0];
  lold = -2.0;
  l    = -1e300;
  i    = 0;

  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;

    lold = l;
    x = p + step;
    l = TMath::Abs(LanGauFun(&x,params) - fy);

    if (l > lold)
      step = -step/10;

    p += step;
  }

  if (i == MAXCALLS)
    return (-2);

  fxr = x;

  // Search for left x location of fy

  p = maxx - 0.5 * params[0];
  step = -params[0];
  lold = -2.0;
  l    = -1e300;
  i    = 0;

  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;

    lold = l;
    x = p + step;
    l = TMath::Abs(LanGauFun(&x,params) - fy);

    if (l > lold)
      step = -step/10;

    p += step;
  }

  if (i == MAXCALLS)
    return (-3);

  fxl = x;

  FWHM = fxr - fxl;
  return (0);

}
///////////////////////////////////////////////////////////////////////////////
Double_t LanGau::LanGauFun(Double_t *x, Double_t *par) {

  //Fit parameters:
  //par[0]=Width (scale) parameter of Landau density
  //par[1]=Most Probable (MP, location) parameter of Landau density
  //par[2]=Total area (integral -inf to inf, normalization constant)
  //par[3]=Width (sigma) of convoluted Gaussian function
  //
  //In the Landau distribution (represented by the CERNLIB approximation),
  //the maximum is located at x=-0.22278298 with the location parameter=0.
  //This shift is corrected within this function, so that the actual
  //maximum is identical to the MP parameter.

  // Numeric constants
  Double_t invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)
  Double_t mpshift  = -0.22278298;       // Landau maximum location

  // Control constants
  Double_t np = 300.0;      // number of convolution steps
  Double_t sc =   3.0;      // convolution extends to +-sc Gaussian sigmas

  // Variables
  Double_t xx;
  Double_t mpc;
  Double_t fland;

  Double_t sum = 0.0;
  Double_t xlow,xupp;
  Double_t step;
  Double_t i;


  //  std::cout<<"Welcome to LangauFun"<< std::endl;

  // MP shift correction
  mpc = par[1] - mpshift * par[0];

  // Range of convolution integral
  xlow = x[0] - sc * par[3];
  xupp = x[0] + sc * par[3];

  step = (xupp-xlow) / np;

  // Convolution integral of Landau and Gaussian by sum
  for(i=1.0; i<=np/2; i++) {
    xx = xlow + (i-.5) * step;
    fland = TMath::Landau(xx,mpc,par[0]) / par[0];
    sum += fland * TMath::Gaus(x[0],xx,par[3]);

    xx = xupp - (i-.5) * step;
    fland = TMath::Landau(xx,mpc,par[0]) / par[0];
    sum += fland * TMath::Gaus(x[0],xx,par[3]);
  }

  return (par[2] * step * sum * invsq2pi / par[3]);

}
///////////////////////////////////////////////////////////////////////////////
Double_t LanGau::LandauFun(Double_t *x, Double_t *par) {

  //Fit parameters:
  //par[0]=Width (scale) parameter of Landau density
  //par[1]=Most Probable (MP, location) parameter of Landau density
  //par[2]=Total area (integral -inf to inf, normalization constant)
  //
  //In the Landau distribution (represented by the CERNLIB approximation),
  //the maximum is located at x=-0.22278298 with the location parameter=0.
  //This shift is corrected within this function, so that the actual
  //maximum is identical to the MP parameter.

  // Numeric constants
  Double_t mpshift  = -0.22278298;       // Landau maximum location

  // MP shift correction
  double mpc = par[1] - mpshift * par[0];

  return par[2] * TMath::Landau(x[0],mpc,par[0]);

}
///////////////////////////////////////////////////////////////////////////////
Double_t LanGau::LanGauPauFun(Double_t *x, Double_t *par) {
  // Use Paula's LanPau function and convolute with a Gaussian

  // par[0]=most probable value
  // par[1]=width for x>par[1]
  // par[2]=width for x<par[1]
  // par[4]=Gaussian width
  // par[3]=normalization

  // Numeric constants
  //double invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)

  // Control constants
  Double_t nSteps = 300.0;      // number of convolution steps
  Double_t sc =   3.0;      // convolution extends to +-sc Gaussian sigmas

  // Range of convolution integral
  double xLow  = x[0] - sc * par[4];
  double xHigh = x[0] + sc * par[4];

  double xx=0.;
  double sum=0.;
  double stepSize = (xHigh-xLow) / nSteps;

  // Convolution integral of Landau and Gaussian by sum
  for(double i=1.0; i<=nSteps/2; i++) {

    xx = xLow + (i-.5) * stepSize;
    sum += LanPau(xx, par[0], par[1], par[2]) * TMath::Gaus(x[0], xx, par[4]);

    xx = xHigh - (i-.5) * stepSize;
    sum += LanPau(xx, par[0], par[1], par[2]) * TMath::Gaus(x[0],xx,par[4]);
  }

  return (par[3] * stepSize * sum / par[4]);
  //  return (stepSize * sum);

}
///////////////////////////////////////////////////////////////////////////////
Double_t LanGau::LanPauFun(Double_t *x, Double_t *par) {
  // par[0]=most probable value
  // par[1]=width for x>par[1]
  // par[2]=width for x<par[1]
  // par[3]=normalization

  return par[3]*LanPau(x[0],par[0], par[1], par[2]);

}
///////////////////////////////////////////////////////////////////////////////
Double_t LanGau::LanPau(double x, double MPV, double sigma1, double sigma2) {
  // fudged Landau, has different with on left and right side of the peak
  // From Paula Collins
  double result;
//   std::cout << " LanGau::LanPau par[0] " << par[0] << " par [1] " << par[1]
// 	    << " par [2] " << par[2] << std::endl;
  if(sigma1>0.&&sigma2>0.) {
    float sign = (x-MPV);
    float val= (sign > 0) ? sign/sigma2 : sign/sigma1;
    result=TMath::Exp(-(val+TMath::Exp(-val))/2.);
  } else {
    result=0.;
  }
  return result;

}
///////////////////////////////////////////////////////////////////////////////
