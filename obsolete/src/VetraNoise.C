// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>

#include <unistd.h>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TRandom.h"
#include "TH2.h"
#include "TStyle.h"

// local
#include "VetraNoise.h"

// static variables
float VetraNoise::getNoiseFromStrip[2048];
float VetraNoise::getNoiseFromTell1Ch[2048];

VetraNoise::~VetraNoise(){}

VetraNoise::VetraNoise(){}

void VetraNoise::readNoise(std::string dut){
  VetraMapping mapping;
  TH1F * noiseHisto;
  std::string noiseFile;
  if(dut=="PR01")
    noiseFile="aux/PR01_RMSNoise_vs_ChipChannel.root";
  else if(dut=="BCB")
    noiseFile="aux/BCB_RMSNoise_vs_ChipChannel.root";
  else if(dut=="D0")
    noiseFile="aux/D0_RMSNoise_vs_ChipChannel.root";
  else if(dut=="3D")
    noiseFile="RootDataLocal/3D_Vetra_Noise_histos.root";

  TFile *f = TFile::Open(noiseFile.c_str());
  noiseHisto = (TH1F*)(f->Get("RMSNoise_vs_ChipChannel"));

  for(int iTell1Ch=0;iTell1Ch<2048; iTell1Ch++) {
    getNoiseFromTell1Ch[iTell1Ch]=noiseHisto->GetBinContent(iTell1Ch);
    int strip;
    strip = mapping.getStripFromTell1ch(iTell1Ch);
    if(strip < 0) continue;
    if(noiseHisto->GetBinContent(iTell1Ch)>1)
      getNoiseFromStrip[strip] = noiseHisto->GetBinContent(iTell1Ch);
    else 
      getNoiseFromStrip[strip] = 1;
  }
  f->Close();
}
