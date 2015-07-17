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
#include "VetraMapping.h"

// Static variables
int VetraMapping::Tell1chToStrip[2048];
int VetraMapping::StripToTell1ch[2048];
int VetraMapping::StripToRegion[2048];
double VetraMapping::StripToPosition[2048];
int VetraMapping::RoutingLineToStrip[2048];

VetraMapping::~VetraMapping(){}

VetraMapping::VetraMapping(){
}

void VetraMapping::createMapping(Parameters* p){
  parameters=p;
  std::string dut=parameters->dut;

  int strip, region, routingLine;
  int t1Ch;
  float position;
  if (dut=="PR01"){
    for(int iStr=0; iStr<2048;iStr++){
      StripToTell1ch[iStr]=-1;
      StripToRegion[iStr]=-1;
      StripToPosition[iStr]=-1;
      RoutingLineToStrip[iStr]=-1;
    }

    std::vector <int> maskedStrips;
    maskedStrips.push_back(9);
    maskedStrips.push_back(84);
    maskedStrips.push_back(414);
    maskedStrips.push_back(491);

    for(int iCh=0; iCh<2048; iCh++){
      ChannelToStrip(iCh, true, &routingLine, &strip, &region, &position);
      //  std::cout<<"Mapping:  Beetle="<<(int)iCh/128<<" link="<<(int)iCh/32<<" channel="<<iCh%32<<" tell1Ch="<<iCh<<"  routingLine="<<routingLine<<" strip="<<strip<<" region="<<region<<" position="<<position<<std::endl;
      if(position == 0)
	strip = -1;
      else{
	RoutingLineToStrip[routingLine]=strip;
	StripToTell1ch[strip]=iCh;
	
	StripToRegion[strip]=region;
	StripToPosition[strip]=position;
	for(int iMask = 0; iMask<(int)maskedStrips.size(); iMask++){
	  if(maskedStrips.at(iMask) == strip)
	    strip = -1;
	}
      }
      Tell1chToStrip[iCh]=strip;
    }
  }
  //-------- Mapping for BCB -------------
  else if(dut=="BCB"){
    for(int iCh=0; iCh<2048; iCh++){
      if(iCh<1536) strip=-1;
      else if((iCh-1792)%4 == 0) strip=iCh+11-1792;
      else if((iCh-1792)%4 == 1) strip=iCh+8-1792;
      else if((iCh-1792)%4 == 2) strip=iCh+8-1792;
      else if((iCh-1792)%4 == 3) strip=iCh+5-1792;
      else strip=-1;
      //exceptions
      if(strip>130) strip=-1;
      else if(strip==13) strip=-1;
      else if(strip==29) strip=-1;
      else if(strip==47) strip=-1;
      else if(strip==49) strip=-1;
      Tell1chToStrip[iCh]=strip;
    }

    for(int iStr=0; iStr<2048; iStr++){
      if(iStr<8) t1Ch=-1;
      else if((iStr)%4 == 0) t1Ch=iStr-5+1792;
      else if((iStr)%4 == 1) t1Ch=iStr-8+1792;
      else if((iStr)%4 == 2) t1Ch=iStr-8+1792;
      else if((iStr)%4 == 3) t1Ch=iStr-11+1792;
      else t1Ch=-1;
      //exceptions
      if(iStr>131) t1Ch=-1;
      else if(iStr==13) t1Ch=-1;
      else if(iStr==29) t1Ch=-1;
      else if(iStr==47) t1Ch=-1;
      else if(iStr==49) t1Ch=-1;
      StripToTell1ch[iStr]=t1Ch;
    }
    for(int iStr=0; iStr<2048; iStr++){
      position=iStr*0.08;//BCB's readout pitch=80 microns
      StripToPosition[iStr]=position;
    }
  }

  //----------- Mapping for D0 ---------------
  else if(dut=="D0"){
//     for(int iCh=0; iCh<2048; iCh++){
//       if(iCh<1536) strip=-1;
//       else strip = 1536-iCh+575;
//       if(strip < 64) strip=-1;
//       if((strip>0)&&(strip<320)) strip+=256;
//       else strip-=256;
//       Tell1chToStrip[iCh]=strip;
//     }
//     int tmpStr;
//     for(int iStr=0; iStr<2048; iStr++){
//       t1Ch=0;
//       if(iStr < 64) t1Ch=-1;
//       else if (iStr > 575) t1Ch=-1;
//       else{
// 	if(iStr<320) tmpStr=iStr+256;
// 	else tmpStr=iStr-256;
// 	t1Ch=575-tmpStr+1536;
//       }
//       StripToTell1ch[iStr]=t1Ch;
//     }
    for(int iT1Ch=0; iT1Ch<2048; iT1Ch++){
      if(iT1Ch<1536) strip = -1;
      else if(iT1Ch<1792) strip = 256+iT1Ch-1536;
      else strip=iT1Ch-1792;
      Tell1chToStrip[iT1Ch]=strip;
    }
    for(int iStr=0;iStr<2048;iStr++){ 
      if(iStr<256) t1Ch=iStr+1792;
      else if(iStr<512) t1Ch=1536+iStr-256;
      else t1Ch=-1; //not bonded strips

      StripToTell1ch[iStr]=t1Ch;
    }

    //  (readout) strip to position
    for(int iStr=0; iStr<2048; iStr++){
      position=iStr*0.06;//D0's readout pitch=60 microns
      StripToPosition[iStr]=position;
    }
  }
}

void VetraMapping::ChannelToStrip(int channel, bool Breorder, int *BroutingLine, int* Bstrip, int* Bregion, float* Sradius){
  int reorder, strip, routingLine, region;
  float radius;

  const float INNER_OFFSET = 10.02; // mm

  const float PITCH_REG1 = 0.04;
  const float PITCH_REG2 = 0.04;
//   const float PITCH_REG3 = 0.04; //unused
  const float PITCH_REG4 = 0.06;

  const float B12_REG4_1ST_ROUTINGLINE_RADIUS = 36.99;
  const float B12_REG1_1ST_ROUTINGLINE_RADIUS = 12.58;
  const float B14_REG2_1ST_ROUTINGLINE_RADIUS = 16.90;
  const float B15_REG4_1ST_ROUTINGLINE_RADIUS = 46.71;

//   const int B12_REG4_1ST_CHANNEL = 1536;  //unused
  const int B12_REG4_1ST_ROUTINGLINE = 773;
  const int B12_REG1_1ST_ROUTINGLINE = 879;

//   const int B14_REG2_1ST_CHANNEL = 1792;  //unused  
  const int B14_REG2_1ST_ROUTINGLINE = 109;
  const int B14_REG3_1ST_ROUTINGLINE = 129;

//   const int B15_REG4_1ST_CHANNEL = 1920;  //unused
  const int B15_REG4_1ST_ROUTINGLINE = 441;
  const int B15_REG2_1ST_ROUTINGLINE = 495;
  const int B15_REG1_1ST_ROUTINGLINE = 559;

  //Following the tell1 channels, we got 3 Beetles bonded of a total of 16. The bonded Beetles are in radius:
  // Beetle 12 -> from Tell1 channel 1536 to 1663 (Yellow)
  // Beetle 14 -> from Tell1 channel 1792 to 1919 (Blue)
  // Beetle 15 -> from Tell1 channel 1920 to 2047 (Black)
  // The "color" of the Beetles is acording to https://webpp.phy.bris.ac.uk/elog/zap-test_summer09/100530_125353/sensor_Instrumented_routingLines.pdf
  int BeetleNumber = (int)channel/128; 
  int BeetlePad = channel-BeetleNumber*128;
  if(BeetleNumber == 12){ // Yellow Beetle
    routingLine = BeetlePad + B12_REG4_1ST_ROUTINGLINE;
    // Beetle's pad 0 is bonded to routingLine N+3, pad 1 is bonded to routingLine N+1, 
    // pad 2 to N+2 and pad 3 to N,  so the schema is 3-1-2-0
    // For check purposes is usefull to see the routingLines before (Breorder == FALSE) and after the mapping (Breorder = TRUE)
    if(Breorder){ 
      reorder = BeetlePad % 4;
      if(reorder == 0)
	routingLine = routingLine + 3;
      else if(reorder == 3)
	routingLine = routingLine - 3;
      }
    if(routingLine<B12_REG4_1ST_ROUTINGLINE){
      std::cout<<"¡¡¡ERROR: Mapping algorithm FAILED for Beetle12, Region4, T1ch:"<<channel;
      std::cout<<" BeetlePad:"<<BeetlePad<<" routingLine:"<<routingLine<<" !!!"<<std::endl;
      return;
    }
    if(routingLine < B12_REG1_1ST_ROUTINGLINE) {
      region = 4;
      radius = (routingLine - B12_REG4_1ST_ROUTINGLINE)*PITCH_REG4 + B12_REG4_1ST_ROUTINGLINE_RADIUS;
    }
    else { 
      region = 1;
      radius = (routingLine - B12_REG1_1ST_ROUTINGLINE)*PITCH_REG1 + B12_REG1_1ST_ROUTINGLINE_RADIUS;
    }
  }
  else if(BeetleNumber == 14){ // Blue Beetle
    routingLine = BeetlePad + B14_REG2_1ST_ROUTINGLINE;
    if(Breorder){
      reorder = BeetlePad % 4;
      if(reorder == 0)
	routingLine = routingLine + 3;
      else if(reorder == 3)
	routingLine = routingLine - 3;
    }
    if(routingLine<B14_REG2_1ST_ROUTINGLINE){
      std::cout<<"¡¡¡ERROR: Mapping algorithm FAILED for Beetle14, Region2, T1ch:"<<channel;
      std::cout<<" BeetlePad:"<<BeetlePad<<" routingLine:"<<routingLine<<" !!!"<<std::endl;
      return;
    }
    if(routingLine < B14_REG3_1ST_ROUTINGLINE)
      region = 2;
    else
      region = 3;
    radius = (routingLine - B14_REG2_1ST_ROUTINGLINE)*PITCH_REG2 + B14_REG2_1ST_ROUTINGLINE_RADIUS;
  }
  else if(BeetleNumber == 15){  //Black Beetle
    routingLine = BeetlePad + B15_REG4_1ST_ROUTINGLINE;
    if(Breorder){
      reorder = BeetlePad % 4;
      if(reorder == 0)
	routingLine = routingLine + 3;
      else if(reorder == 3)
	routingLine = routingLine - 3;
    }
    if(routingLine<B15_REG4_1ST_ROUTINGLINE){
      std::cout<<"¡¡¡ERROR: Mapping algorithm FAILED for Beetle14, Region2, T1ch:"<<channel;
      std::cout<<" BeetlePad:"<<BeetlePad<<" routingLine:"<<routingLine<<" !!!"<<std::endl;
      return;
    }
    if(routingLine < B15_REG2_1ST_ROUTINGLINE){
      region = 4;
      radius = (routingLine - B15_REG4_1ST_ROUTINGLINE)*PITCH_REG4 + B15_REG4_1ST_ROUTINGLINE_RADIUS;
    }
    else if ((routingLine >= B15_REG2_1ST_ROUTINGLINE)&&(routingLine < B15_REG1_1ST_ROUTINGLINE)) {
      region = 2;
      radius = (routingLine - B15_REG2_1ST_ROUTINGLINE)*PITCH_REG2 + INNER_OFFSET;
    }
    else {
      region = 1;
      radius = (routingLine - B15_REG1_1ST_ROUTINGLINE)*PITCH_REG1 + INNER_OFFSET;
    }
  }
  else {
    routingLine = 0;
    reorder = 0;
    region = 0;
    radius = 0;
  }

  //Here we translate the routing line into the strips order. The strip nº0 is the 1st strip (the inner most) in region 1, and the next are consecutives. That way strips from 0 to 191 belongs to region 1, strips from 192 to 383 are in region 2, strips from 384 to 639 are in region 3 and finally strips from 640 to 1005 are in region 4.
  int REG1_GROUP1_FIRST_RL = 559; //First routing line in region 1 -> corresponds to strip 0
  int REG1_GROUP1_LAST_RL = 622; 
  int REG1_GROUP2_FIRST_RL = 879;
  int REG1_GROUP2_LAST_RL = 2046; //Last routing line in region 1 -> correspond to strip 191

  int REG2_GROUP1_FIRST_RL = 495; //First routing line in region 2 -> corresponds to strip 192
  int REG2_GROUP1_LAST_RL = 558; 
  int REG2_GROUP2_FIRST_RL = 1;
  int REG2_GROUP2_LAST_RL = 128; //Last routing line in region 2 -> correspond to strip 383

  int REG3_GROUP1_FIRST_RL = 129; //First routing line in region 3 -> corresponds to strip 384
  int REG3_GROUP1_LAST_RL = 256; 
  int REG3_GROUP2_FIRST_RL = 623;
  int REG3_GROUP2_LAST_RL = 750; //Last routing line in region 3 -> correspond to strip 639

  int REG4_GROUP1_FIRST_RL = 257; //First routing line in region 4 -> corresponds to strip 640
  int REG4_GROUP1_LAST_RL = 384; 
  int REG4_GROUP2_FIRST_RL = 751;
  int REG4_GROUP2_LAST_RL = 878; 
  int REG4_GROUP3_FIRST_RL = 385;
  int REG4_GROUP3_LAST_RL = 494; //Last routing line in region 4 -> correspond to strip 2046

  if((routingLine >= REG1_GROUP1_FIRST_RL) && (routingLine <= REG1_GROUP1_LAST_RL))
    strip = routingLine - REG1_GROUP1_FIRST_RL;
  else if((routingLine >= REG1_GROUP2_FIRST_RL) && (routingLine <= REG1_GROUP2_LAST_RL))
    strip = (routingLine - REG1_GROUP2_FIRST_RL) + 64;
  else if((routingLine >= REG2_GROUP1_FIRST_RL) && (routingLine <= REG2_GROUP1_LAST_RL))
    strip = routingLine - REG2_GROUP1_FIRST_RL + 192;
  else if((routingLine >= REG2_GROUP2_FIRST_RL) && (routingLine <= REG2_GROUP2_LAST_RL))
    strip = (routingLine - REG2_GROUP2_FIRST_RL) + 256;
  else if((routingLine >= REG3_GROUP1_FIRST_RL) && (routingLine <= REG3_GROUP1_LAST_RL))
    strip = routingLine - REG3_GROUP1_FIRST_RL + 384;
  else if((routingLine >= REG3_GROUP2_FIRST_RL) && (routingLine <= REG3_GROUP2_LAST_RL))
    strip = (routingLine - REG3_GROUP2_FIRST_RL)+ 512;
  else if((routingLine >= REG4_GROUP1_FIRST_RL) && (routingLine <= REG4_GROUP1_LAST_RL))
    strip = routingLine - REG4_GROUP1_FIRST_RL + 640;
  else if((routingLine >= REG4_GROUP2_FIRST_RL) && (routingLine <= REG4_GROUP2_LAST_RL))
    strip = (routingLine - REG4_GROUP2_FIRST_RL) + 768;
  else if((routingLine >= REG4_GROUP3_FIRST_RL) && (routingLine <= REG4_GROUP3_LAST_RL))
    strip = (routingLine - REG4_GROUP3_FIRST_RL) + 896;
  else
    strip = 0;

  *BroutingLine = routingLine;
  *Bstrip = strip;
  *Bregion = region;
  *Sradius = radius;
}
