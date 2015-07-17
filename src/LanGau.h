#ifndef __LANGAU__
#define __LANGAU__

#include <string>
#include <iostream>
#include "TROOT.h"
#include "TString.h"
#include "TH1.h"
#include "TF1.h"
#include "TH2.h"
#include "TH3.h"

namespace LanGau {

  void FitLanGau(const Text_t *filename, const Text_t *histname);

  TF1* FitLanGau(TH1F* histo);

  TF1* FitLanGauPau(TH1F* histo);

  void FitSliceLanGau(const TString & filename, 
		      const TString & histname, 
		      const TString & outFileName);

  void FitSliceLanGau(const TString & outFileName, TH2F* histtofit);

  Int_t LanGauPro(Double_t *params, Double_t& maxx, Double_t& FWHM);

  Double_t LanGauFun(Double_t *x, Double_t *par);

  Double_t LandauFun(Double_t *x, Double_t *par);

  Double_t LanGauPauFun(Double_t *x, Double_t *par);

  Double_t LanPauFun(Double_t *x, Double_t *par);

  Double_t LanPau(double x, double MPV, double sigma1, double sigma2);

};
#endif
