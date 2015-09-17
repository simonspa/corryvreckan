// $Id: Amalgamation.C,v 1.10 2010/06/16 23:33:02 mjohn Exp $
// Include files 
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <limits>
#include <deque>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TSystem.h"

// local
#include "CommonTools.h"
#include "Amalgamation.h"
#include "TestBeamEvent.h"
#include "VetraNtupleClass.h"
#include "VetraMapping.h"

// dhynds test root 6
#include "TestBeamEvent.h"
#include "TestBeamEventElement.h"
#include "RowColumnEntry.h"
#include "TestBeamDataSummary.h"
#include "TDCFrame.h"
#include "TDCTrigger.h"
#include "PhysicalConstants.h"





//-----------------------------------------------------------------------------
// Implementation file for class : Amalgamation
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------


std::vector<std::string> &split(const std::string &s, const std::string &delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, *delim.c_str())) {
    elems.push_back(item);
  }
  return elems;
}

bool sortFEI4(const std::string& file1, const std::string& file2) {

  int i1 = 0, i2 = 0;
  if (file1.find(".raw") == std::string::npos) {
    i1 = -1;
  } else {
    std::size_t pStart = file1.find_last_of("_");
    std::size_t pStop = file1.find_last_of(".");
    if (pStart == std::string::npos ||
        pStop - 1 < pStart + 1) {
      i1 = 0;
    } else {
      pStart += 1;
      std::string indx = file1.substr(pStart, pStop - pStart);
      i1 = atoi(indx.c_str());
    }
  }
  if (file2.find(".raw") == std::string::npos) {
    i2 = -1;
  } else {
    std::size_t pStart = file2.find_last_of("_");
    std::size_t pStop = file2.find_last_of(".");
    if (pStart == std::string::npos ||
        pStop - 1 < pStart + 1) {
      i2 = 0;
    } else {
      pStart += 1;
      std::string indx = file2.substr(pStart, pStop - pStart);
      i2 = atoi(indx.c_str());
    }
  }
  return i1 < i2;

}

bool sortOnTime(const IndexEntry& i1, const IndexEntry& i2)
{
  return i1.time()<i2.time();
}

bool sortTDCTime(const FileTime& t1, const FileTime& t2)
{
  return t1.second < t2.second;
}

bool sortTriggers( TDCTrigger* t1, TDCTrigger* t2)
{
  return t1->timeAfterShutterOpen() < t2->timeAfterShutterOpen();
}

Amalgamation::Amalgamation(Parameters* p)
 : compressedRawFormat(false)
 , slash("/")
 , hash("#")
 , tdc(p)
 , firstDate(0)
 , firstTime(0)
 , lastDate(2147483647)//largest possible
 , lastTime(2147483647)//signed 32-bit number
 , firstTimepixTime(0)
 , lastTimepixTime(2147483647)
{
  parameters=p;
  summary=new TestBeamDataSummary();
  int sc=0;
  
  if(parameters->useRelaxd)   sc=indexPixFiles(parameters->relaxdDataDirectory);
  if(sc) return;
  
  if(parameters->useMedipix)  sc=indexPixFiles(parameters->medipixDataDirectory);
  if(sc) return;
  
  if(parameters->useTDC){
    if(!parameters->useVetra&&!parameters->useFEI4&&!parameters->useSciFi){
      sc=indexTDCFiles();
      if(sc) return;
      sc=checkTimeRange(firstTime,lastTime);
      if(sc) return;
      matchTDCandTelescope();
    }else{
      tdc.includingaDUT=true;
      if(parameters->useVetra){
        sc=indexVetraFile();
        if(sc) return;
      }
      if(parameters->useFEI4){
        sc=indexFEI4Data();
        if(sc) return;
      }
      if(parameters->useSciFi){
        sc=indexSciFiData();
        if(sc) return;
      }
    }
  }
  //Clone summary histos in the TestBeamDataSummary for the record.
    summary->CopyTDCHistos(tdc.hNumberSync,
                           tdc.hNumberUnsync,
                           tdc.hNumberPairs,
                           tdc.hTDCShutterTime,
                           tdc.hTDCUnsyncTriggerTime,
                           tdc.hSynchronisationDelay,
                           tdc.hExtraUnsyncTriggers,
                           tdc.hExtraSyncTriggers,
                           tdc.hUnmatched,
                           tdc.hTDCSyncTriggerTime,
                           tdc.hMedipixTime,
                           tdc.hDUTTime,
                           tdc.hTDCTime,
                           tdc.hFEI4TimeDiff,
                           tdc.hFEI4MatchTime,
                           tdc.hFEI4FractionMatched);

  std::sort(index.begin(),index.end(),sortOnTime);
  if(index.size()) buildEventFile();
}



int Amalgamation::indexPixFiles(std::string dataDirectory)
{
  char str[2048];
  bool firstEvt=true;
  std::vector<std::string> files = std::vector<std::string>();
  
  std::string directory=dataDirectory;
  // unzip the tar file if given that.                                                                                                                                            
  if(std::string::npos!=directory.find(".tar.gz")){
    std::string zip=directory.substr(directory.rfind("/")+1,directory.size());
    std::string bas=directory.substr(0,directory.rfind("/"));
    directory=directory.substr(0,directory.rfind(".tar.gz"));
    std::ostringstream out;
    out<<"cd "<<bas<<"; tar -xzf "<<zip;
    gSystem->Exec(out.str().c_str());
    dataDirectory=directory;
  }
  
  CommonTools::getdir(directory,files);
  
  int dscFiles=0;
  int txtFiles=0;
  for (unsigned int i = 0;i < files.size();i++) {
    if(files[i].find(".dsc")!=std::string::npos) dscFiles++;
    if(files[i].find(".txt")!=std::string::npos) txtFiles++;
  }
  
  if(files.size()==0){
    if(directory.find(".txt")!=std::string::npos){
      // realise the directory is, in fact, one big data file
      files.push_back(directory.substr(directory.rfind("/")+1,directory.size()-1));
      directory=directory.substr(0,directory.rfind("/"));
      txtFiles++;
    }else{
      std::cout <<"The directory is empty!"<< std::endl;
      return 1;
    }
  }
  bool relaxd=false;
  if(dscFiles==0){
    if(txtFiles==0){
      std::cout << "No .dsc files (PixelMan) nor .txt files (RELAXD) found in " << directory << " Abandoning..."<<std::endl;
      return 1;
    }
    if(parameters->verbose){std::cout <<"Autodetecting this is RELAXD readout"<< std::endl;}
    //dhynds parameters->eventWindow=0.000000001;// a nanosecond - with RELAXd the readout is sychronised by design.
    relaxd=true;
  }
  if(parameters->verbose) std::cout << "Indexing timepix frames assuming a "<< parameters->eventWindow <<"s time-window\n" << std::endl;
  
  unsigned int ifile=0;
  for (unsigned int i = 0;i < files.size();i++) {
    if(not relaxd && files[i].find(".dsc")==std::string::npos) continue;
    if(    relaxd && files[i].find(".txt")==std::string::npos) continue;
    ifile++;
    if(parameters->verbose){
      std::string word("detectors");
      if(summary->nDetectors()==1) word="detector";
      if(!relaxd){
        if(((ifile/2) - int((ifile/2))==0)||ifile>=files.size()){
          std::cout <<"\rIndexing "<<ifile/2+1<<"/"<<files.size()/2<<" .dsc files. "<<summary->nDetectors()<<" "<<word<<" identified" << std::flush;
        }
      }else{
/*        if(((ifile) - int((ifile))==0)||ifile>=files.size()){
          std::cout <<"\rIndexing "<<ifile+1<<"/"<<files.size()<<" .txt files. "<<summary->nDetectors()<<" "<<word<<" identified" << std::flush;
        } //dhynds
 */    if(((ifile/2) - int((ifile/2))==0)||ifile>=files.size()){
            std::cout <<"\rIndexing "<<ifile/2+1<<"/"<<files.size()/2<<" .txt files. "<<summary->nDetectors()<<" "<<word<<" identified" << std::flush;
        }  
      }
    }
    
    std::string fullpath(directory+slash+files[i]);
    std::fstream file_op(fullpath.c_str(),std::ios::in);
    std::vector<std::string> content;
    while(!file_op.eof()){
      file_op.getline(str,2000);
      content.push_back(std::string(str));
    }
    file_op.close();
    
    int mode=0;
    int clock=-1;
    int roughtime=0;
    double time = -1;
    std::string chip;
    float acqTime=0.0;
    int threshold=0;
    
    
    if(relaxd){
      std::vector<std::string>::const_iterator ite=content.begin();
      
      int frameNumInTxtFile=1;
      for(;ite<content.end();ite++){
        if(std::string::npos==(*ite).find("#")) continue; // skip over data - just read meta here
        std::vector<std::string> lineContent;
        split((*ite),"#",lineContent);
        std::vector<std::string>::iterator it=lineContent.begin();
        for(;it!=lineContent.end();it++){
          
          int colon = (*it).find(":");
          if(colon<0) continue;
          
          std::pair<std::string,std::string> field((*it).substr(0,colon),(*it).substr(colon+1,(*it).size()-1));
          
          if(std::string::npos!=field.first.find("Acq time")){
            acqTime=atof(field.second.c_str());
          }
          if(std::string::npos!=field.first.find("ChipboardID")){
            chip=field.second;
            CommonTools::stringtrim(chip);
          }
          if(std::string::npos!=field.first.find("Triger time")){
            time=strtod(field.second.c_str(),NULL);
          }
          if(std::string::npos!=field.first.find("DACs")){
            int column=0;
            std::string token;
            std::string line=field.second;
            std::stringstream in ( CommonTools::stringtrim(line).c_str() );
            while ( std::getline ( in, token, ' ' ) ){
              column++;
              if(column==7){
                threshold=atoi(token.c_str());
                break;
              }
            }
          }
          if(std::string::npos!=field.first.find("Mpx type")){
            mode=atoi(field.second.c_str());
          }
          if(std::string::npos!=field.first.find("Timepix clock")){
            clock=atoi(field.second.c_str());
          }
          if(std::string::npos!=field.first.find("Start time (string)")){
            if(firstEvt){
              summary->dataSources(parameters->useVetra,parameters->useRelaxd);
              summary->origins(parameters->vetraFile,dataDirectory,parameters->eventFile);
              summary->startTime(field.second,parameters->eventWindow);
              firstEvt=false;
            }
            std::string mo=field.second.substr(field.second.find(":")-9,3);
            int month=0;
            if(std::string("Jan")==mo) month=1;
            if(std::string("Feb")==mo) month=2;
            if(std::string("Mar")==mo) month=3;
            if(std::string("Apr")==mo) month=4;
            if(std::string("May")==mo) month=5;
            if(std::string("Jun")==mo) month=6;
            if(std::string("Jul")==mo) month=7;
            if(std::string("Aug")==mo) month=8;
            if(std::string("Sep")==mo) month=9;
            if(std::string("Oct")==mo) month=10;
            if(std::string("Nov")==mo) month=11;
            if(std::string("Dec")==mo) month=12;
            if(0==month) std::cout << "WARNING: can't identify month from: "<<summary->startTime()<<std::endl;
            int day  =atoi( field.second.substr(field.second.find(":")-5,2).c_str());
            int hour =atoi( field.second.substr(field.second.find(":")-2,2).c_str());
            int min  =atoi( field.second.substr(field.second.find(":")+1,2).c_str());
            int sec  =int(atof( field.second.substr(field.second.rfind(":")+1,7).c_str())+0.5);
            roughtime=(month*100+day)*86400+3600*hour+60*min+sec;
         //     std::cout<<"Relaxd written at "<<hour<<":"<<min<<":"<<sec<<std::endl; //dhynds
          }else if(std::string::npos!=field.first.find("Start time")){
            time=strtod(field.second.c_str(),NULL);
            //  std::cout<<"relaxd time "<<std::setprecision(16)<<time<<std::endl; //dhynds
          }
        }
        summary->registerDetector(chip,mode,clock,acqTime,threshold);
        index.push_back(IndexEntry(roughtime,time,std::string("timepix"),chip,directory+slash+files[i],frameNumInTxtFile));
        //std::cout<<"    Time: "<<std::setprecision(16)<<time<<" ... Chip: "<<chip<<" "<<summary->nFilesForDetector(chip)<<" ... File: "<<directory+slash+files[i]<<" ... Frame: "<<frameNumInTxtFile<<"  "<<de(roughtime)<<" ("<<roughtime<<")"<<" "<<index.size()<<std::endl;
        frameNumInTxtFile++;
        
      }//loop over file content
      compressedRawFormat=true;
    }else{//PixelMan format
      
      std::vector<std::string>::const_iterator iter=content.begin();
      int frameNumInDscFile=-1;
      if(std::string::npos!=(*iter).find("A00")){
        compressedRawFormat=true;
        frameNumInDscFile=0;
        iter++;//skip 'A0000' line
        iter++;//skip '[F00]' line
      }
      
      for(;iter<content.end();iter++){
        
        if(std::string::npos!=(*iter).find("Acq time")){
          acqTime=atof((*(iter+2)).c_str());
        }
        if(std::string::npos!=(*iter).find("ChipboardID")){
       //     chip=((*(iter+2)).c_str());
           std::string chipreal=(*(iter+2)); //dhynds for medipix3
            chip = chipreal.substr(0, 9);
            CommonTools::stringtrim(chip);
            
     //dhynds       std::cout<<std::endl<<"************"<<chip<<"************"<<std::endl;
          CommonTools::stringtrim(chip);
        }
        if(std::string::npos!=(*iter).find("Triger time")){
          time=strtod((*(iter+2)).c_str(),NULL);
        }
        if(std::string::npos!=(*iter).find("DACs")){
          int column=0;
          std::string token;
          std::string line=(*(iter+2));
          std::stringstream in ( CommonTools::stringtrim(line).c_str() );
          while ( std::getline ( in, token, ' ' ) ){
            column++;
            if(column==1){//dhynds for medipix3 threshold!
              threshold=atoi(token.c_str());
              break;
            }
          }
        }
        if(std::string::npos!=(*iter).find("Mpx type")){
          mode=atoi((*(iter+2)).c_str());
        }
        if(std::string::npos!=(*iter).find("Timepix clock")){
          clock=atoi((*(iter+2)).c_str());
        }
        if(std::string::npos!=(*iter).find("Start time (string)")){
          if(firstEvt){
            summary->dataSources(parameters->useVetra,parameters->useMedipix);
            summary->origins(parameters->vetraFile,dataDirectory,parameters->eventFile);
            summary->startTime(*(iter+2),parameters->eventWindow);
            firstEvt=false;
          }
          std::string mo=(iter+2)->substr((iter+2)->find(":")-9,3);
          int month=0;
          if(std::string("Jan")==mo) month=1;
          if(std::string("Feb")==mo) month=2;
          if(std::string("Mar")==mo) month=3;
          if(std::string("Apr")==mo) month=4;
          if(std::string("May")==mo) month=5;
          if(std::string("Jun")==mo) month=6;
          if(std::string("Jul")==mo) month=7;
          if(std::string("Aug")==mo) month=8;
          if(std::string("Sep")==mo) month=9;
          if(std::string("Oct")==mo) month=10;
          if(std::string("Nov")==mo) month=11;
          if(std::string("Dec")==mo) month=12;
          if(0==month) std::cout << "WARNING: can't identify month from: "<<summary->startTime()<<std::endl;
          int day  =atoi( (iter+2)->substr((iter+2)->find(":")-5,2).c_str());
          int hour =atoi( (iter+2)->substr((iter+2)->find(":")-2,2).c_str());
          int min  =atoi( (iter+2)->substr((iter+2)->find(":")+1,2).c_str());
          int sec  =int(atof( (iter+2)->substr((iter+2)->rfind(":")+1,7).c_str()));
          roughtime=(month*100+day)*86400+3600*hour+60*min+sec;
        //    std::cout<<"Pixelman written at "<<hour<<":"<<min<<":"<<sec<<std::endl; //dhynds
        }else if(std::string::npos!=(*iter).find("Start time")){
          time=(strtod((*(iter+2)).c_str(),NULL));
      //      std::cout<<"pixelman time "<<std::setprecision(16)<<time<<std::endl; //dhynds

        }
        if(std::string::npos!=(*iter).find("[F") || iter+1==content.end()){
          //Next Frame in the .dsc file so register this entry
          // (and find data in data file if 2010 [zs] data)
          
          summary->registerDetector(chip,mode,clock,acqTime,threshold);
          
          //June 20101: Richard's 'fix': add AcqTime to start time
//          roughtime+=int(acqTime+0.5);
//          time+=acqTime;
          
          //std::cout<<"    Time: "<<std::setprecision(16)<<time<<" ... Chip: '"<<chip<<"' (#"<<summary->nDetectors()<<") ... File: "<<files[i]<<" ... Acq.time: "<<std::setprecision(8)<<acqTime<<" ... Threshold: "<<threshold<<" ... Frame: "<<frameNumInDscFile<<"  "<<de(roughtime)<<" ("<<roughtime<<")"<<std::endl;
          
          std::string datafile=directory+slash+files[i].substr(0,files[i].find(".dsc"));
          index.push_back(IndexEntry(roughtime,time,std::string(mode==3?"timepix":"medipix"),chip,datafile,frameNumInDscFile));
          
          if(compressedRawFormat){ //Early 2010 running 
            if(std::string::npos!=(*iter).find("[F")){ // the end marker of a frame is the being marker of the next
              frameNumInDscFile=atoi( ((*iter).substr(2,(*iter).find("]"))).c_str() );
            }
          }
        }
      }//loop over file content
    }//end of is not relaxd condition
  }//loop over files
  if(parameters->verbose) std::cout<<std::endl;
  std::sort(index.begin(),index.end(),sortOnTime);
  if(index.size()) summary->lastTime(index.back().roughtime());
  if(index.size()) summary->firstTime(index.front().roughtime());
  for(Index::iterator iit=index.begin();iit!=index.end();iit++){
    summary->timing((*iit).detector(),(*iit).time());
  }
  summary->timingEnd();
  return 0;
}




int Amalgamation::indexTDCFiles()
{
  // Retrieve all TDC files from specified location
  FilesTimes files_times;
  std::vector<std::string> files = std::vector<std::string>();
  CommonTools::getdir(parameters->tdcDirectory,files);
  std::cout<<"Number of tdc files is "<<files.size()<<std::endl;
  if(files.size()==0){
    std::cout <<"The TDC directory is empty!"<< std::endl;
    return 1;
  }
  // Identify which TDC files correspond to DUT data time range
  for (unsigned int i = 0;i < files.size();i++) {
    if(files[i].find("TDC")!=0||files[i].find(".txt")==std::string::npos) continue;
    int year =atoi( files[i].substr(files[i].rfind("_")-4,4).c_str());
    int hour =atoi( files[i].substr(files[i].rfind("_")+1,2).c_str());
    int min  =atoi( files[i].substr(files[i].rfind("_")+3,2).c_str());
    int sec  =atoi( files[i].substr(files[i].rfind("_")+5,2).c_str());
    std::string ampm = files[i].substr(files[i].rfind("_")+7,2);
    int month=atoi( files[i].substr(4,2).c_str() );
    int day  =atoi( files[i].substr(6,2).c_str() );
    if((files[i].rfind("_")-files[i].find("_"))<9){
      month=atoi( files[i].substr(files[i].find("_")+1,1).c_str() );
      if((files[i].rfind("_")-files[i].find("_"))==7){
        day=atoi( files[i].substr(files[i].find("_")+2,1).c_str() );
      }else{
        day=atoi( files[i].substr(files[i].find("_")+2,2).c_str() );
      }
    }
    if(ampm==std::string("AM") and hour==12) hour-=12;
    if(ampm==std::string("PM") and hour!=12) hour+=12;
    int date=year*10000+month*100+day;
    int time=hour*3600+min*60+sec;//dhynds shifted by +40
    int fulltime=(month*100+day)*86400+time;
    if(tdc.includingaDUT){
			if(0==firstDate){ //dhynds moved loop here
				firstDate=date; 
				lastDate=date;
				firstTime=fulltime;
				lastTime=fulltime;
			}
      if(date<firstDate) continue;//dhynds
      if(date>firstDate) continue;//dhynds
      // if(date==firstDate and fulltime<firstTime-10) continue;
      // if(date==lastDate  and fulltime>lastTime +10) continue;

    }else{
      if(0==firstTime){
        firstTime=fulltime;
        lastTime=fulltime;
        firstDate=date;
        lastDate=date;
      }else{
        if(fulltime<firstTime){ firstDate=date; firstTime=fulltime;}
        if(fulltime>lastTime){  lastDate =date; lastTime=fulltime;}
      }
    }
    std::string fullpath=parameters->tdcDirectory+slash+files[i];
    files_times.push_back(std::make_pair(fullpath,fulltime));
  }
  
  std::sort(files_times.begin(),files_times.end(),sortTDCTime);
  if(!tdc.hTDCTime) tdc.createTimeTrendHistos(firstTime,lastTime);
  // Feed good files into the TDCManager which makes TDCFile,TDCFrame
  // and TDCTrigger objects according to the TDC signals [0,8,12,13]
  for(unsigned int i = 0;i < files_times.size();i++){
    if(parameters->verbose&&tdc.files()->size()){
      std::cout<<"\rReading TDCFile "<<i+1<<"/"<<files_times.size()<<" from "<<de(files_times[i].second)<<" "<<files_times[i].first<< std::endl;
    }
    int sc = tdc.readFile(files_times[i].second,files_times[i].first);
    if(0==sc)tdc.hTDCTime->Fill(files_times[i].second-firstTime,1);
  }
  if(parameters->verbose) std::cout<<"\n"<<std::endl;
  if(tdc.totalNFrames()==0){
    if(parameters->verbose){std::cout <<"No TDC files found to time-match."<< std::endl;}
  }  
  return 0;
}



int Amalgamation::indexFEI4Data()
{  
  // FEI4: create vector of FEI4 file names & store directory name locally
  std::vector<std::string> files = std::vector<std::string>();  
  std::string directory=parameters->fei4Directory;

  // FEI4: unzip the tar file if given that.                                                                                                                                            
  if(std::string::npos!=directory.find(".tar.gz")){
    std::string zip=directory.substr(directory.rfind("/")+1,directory.size());
    std::string bas=directory.substr(0,directory.rfind("/"));
    directory=directory.substr(0,directory.rfind(".tar.gz"));
    std::ostringstream out;
    out<<"cd "<<bas<<"; tar -xzf "<<zip;
    gSystem->Exec(out.str().c_str());
    parameters->fei4Directory=directory;
  }
  // FEI4: find files in directory structure
  int rawFiles=0;
  CommonTools::getdir(directory,files);
  for (unsigned int i = 0;i < files.size();i++) {
    if(files[i].find(".raw")!=std::string::npos) rawFiles++;
  }
  if(files.size()==0){
    if(directory.find(".raw")!=std::string::npos){
      // realise the directory is, in fact, one big data file
      files.push_back(directory.substr(directory.rfind("/")+1,directory.size()-1));
      directory=directory.substr(0,directory.rfind("/"));
      rawFiles++;
    }else{
      std::cout <<"The directory is empty!"<< std::endl;
      return 1;
    }
  }
  // FEI4: sort files by spill index
  std::sort(files.begin(), files.end(), sortFEI4);
  // FEI4: loop though files and write time-stamp, trigger, bcid and hit info to memory
  char str[2048];
  unsigned int ifile=0;
	std::map<unsigned int,int> fei4TimeStamp;

	std::vector<std::string> fei4Data;
	std::vector<unsigned int> fei4Index;
  for (unsigned int i = 0;i < files.size();i++) {
    if( files[i].find(".raw")==std::string::npos){ continue; }
    ifile++;    
    std::string fullpath(directory+slash+files[i]);
    std::fstream file_op(fullpath.c_str(),std::ios::in);
    std::vector<std::string> content;
    while(!file_op.eof()){
      file_op.getline(str,2000);
      content.push_back(std::string(str));
    }
    file_op.close();
    
    std::string dateline=content[1];
    std::vector<std::string> lineContent;
    split(dateline," ",lineContent);
    std::vector<std::string>::iterator it=lineContent.begin();
    
    std::string mo=lineContent[2];
    int month=0;
    if(std::string("Jan")==mo) month=1;
    if(std::string("Feb")==mo) month=2;
    if(std::string("Mar")==mo) month=3;
    if(std::string("Apr")==mo) month=4;
    if(std::string("May")==mo) month=5;
    if(std::string("Jun")==mo) month=6;
    if(std::string("Jul")==mo) month=7;
    if(std::string("Aug")==mo) month=8;
    if(std::string("Sep")==mo) month=9;
    if(std::string("Oct")==mo) month=10;
    if(std::string("Nov")==mo) month=11;
    if(std::string("Dec")==mo) month=12;
    if(0==month) std::cout << "WARNING: can't identify month from: "<<summary->startTime()<<std::endl;
    int day  =atoi( lineContent[3].c_str() );
    int year =atoi( lineContent[5].c_str() );
    std::vector<std::string> timeContent;
    split(lineContent[4],":",timeContent);
    int hour =atoi( timeContent[0].c_str() );
    int min  =atoi( timeContent[1].c_str() );
    int sec  =atoi( timeContent[2].c_str() );
    int date=year*10000+month*100+day;
    int time=hour*3600+min*60+sec+15; //dhynds added +185
    int fulltime=(month*100+day)*86400+time;
    if(0==firstDate){
      firstDate=date; 
      lastDate=date;
      firstTime=fulltime;
      lastTime=fulltime;
    }else{
      if(fulltime<firstTime){
        firstDate=date;
        firstTime=fulltime;
      }
      if(fulltime>lastTime){
        lastDate=date;
        lastTime=fulltime;
      }
    }
    
    if(parameters->verbose){std::cout<<"\r ***Reading FEI4File "<<i+1<<"/"<<files.size()<<" from "<<de(fulltime)<<" "<<files[i]<< std::endl;}

    for(std::vector<std::string>::iterator ite=content.begin();ite<content.end();ite++){
      std::string line=CommonTools::stringtrim(*ite);
      if(0==line.find("TD")||0==line.find("DH")||0==line.find("DR")){
        fei4Data.push_back(line);
        if(0==line.find("TD")){
          fei4Index.push_back(fei4Data.size()-1);
          fei4TimeStamp[fei4Index.size()-1]=fulltime;
        }
      }
    }
  }
  fei4Data.push_back(std::string(""));
  fei4Index.push_back(fei4Data.size()-1);
  //FEI4: 'beginning' and 'end' are pointers to the begining and end of each trigger in the huge stored vector of stored
  std::vector<unsigned int>::iterator beginning=fei4Index.begin();
  std::vector<unsigned int>::iterator end=beginning+1;    
  
  if(checkTimeRange(firstTime,lastTime)){return 1;}
  tdc.createTimeTrendHistos(firstTime,lastTime);
  for(unsigned int ivs=0;ivs<fei4TimeStamp.size();ivs++){
    tdc.hDUTTime->Fill(fei4TimeStamp[ivs]-firstTime,3);
  }  
  // Now we have identified the time range over which we want to have 
  // TDC information, read and index the relevant TDC information
	indexTDCFiles();

  

  int prevBcid=0;  
  float prevTime=0;
  bool firstEntry=true;  
  std::cout << " now will loop over stored TDCFiles " << std::endl;

  // FEI4: Now loop over stored TDCFiles noting the rough (precise to seconds) time of the file[=spill]
	
	//dhynds changed the order to that of the scifi amalgamation. The idea is to loop over the FEI4 frames, and for 
	//each frame, loop over all of the tdc information and find the frame that matches. Their readout system is 
	//losing too many triggers and it seems inefficient to do things the other way round. I have not seen them lose
	//whole frames, but would not be surprised if they did - so this method should be more robust for the future.
	std::vector<std::vector < TDCTrigger > > *fei4frames = new std::vector<std::vector < TDCTrigger > >;
	std::vector<TDCTrigger> fei4triggers;
	TDCTrigger* fei4trigger=new TDCTrigger();
		std::vector<std::pair<int, int > > timeStampFei4;
		bool newTriggerFei4=false;
		bool newFrameFei4=false;
	int tdid=0; int lv1id=-1; int bcid=0; int prevBcid1=0; int hit_no=-1; double RCE_time_first=777465877; double RCE_time=0;

		int i_x=0;
		int i_used=0;
	for(std::vector<std::string>::iterator i=fei4Data.begin(); i!=fei4Data.end(); i++){
		
		std::string line=*i;
		std::vector<std::string> lineContent;
		split(line," ",lineContent);
//		std::cout<<"Line "<<i_x<<": "<<line<<std::endl;
		
		if(0==line.find("TD")){ //new trigger read out
//			std::cout<<"New trigger"<<std::endl;
				newTriggerFei4=true;
			i_used=i_x;
//			std::cout<<"Saving TD line "<<i_used<<std::endl;
		}else if(0==line.find("DH")){ //new set of data. For each trigger the FEI4 can read up to 16 bunch crossings of data
			bcid=atoi(lineContent[3].c_str());
			if(timeStampFei4.size() == 0){
				prevBcid1=bcid;
			}
			if(newTriggerFei4){
	//			std::cout<<"Time between new and last trigger is "<<0.0256*(bcid-prevBcid1)<<std::endl;
				if(0.0256*(bcid-prevBcid1) > 5000){ //window between frames
					prevBcid1=0;
					newFrameFei4=true;
					newTriggerFei4=true;
				}
				else{
					timeStampFei4.push_back(std::make_pair(i_used,bcid));
//					std::cout<<"New trigger with bcid "<<bcid<<", index "<<i_used<<" and line "<<line<<std::endl;
					prevBcid1=bcid;
			//		i--;
					newTriggerFei4=false;
				}
			}
		}
		
		if(newFrameFei4){
//			std::cout<<"Made frame with "<<timeStampFei4.size()<<" entries"<<std::endl;
			bool frameFound=false;
			
			
			//loop over all files
			for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
//				std::cout<<"New tdc file"<<std::endl;
				if(frameFound)continue;
				TDCFile* file=(*ifi);
				//loop over all frames
				for(TDCFrames::iterator ifr=file->frames()->begin();ifr!=file->frames()->end();ifr++){
					float fei4TimeSinceLastTrigger=0;
					float tdcTimeSinceLastTrigger=0;
					float prevTDCTime=0;
					bool matched=false;
					
					TDCFrame* frame=(*ifr);
					TDCTriggers* triggers = frame->triggers();

					std::vector<std::pair<int, int > >::iterator fei4hit = timeStampFei4.begin();
					
					
					double nMatched=0.;
					//check if times between triggers in tdc match time between triggers in scifi files
					for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
						float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
						matched=false;
						if(triggers->begin()==itr){
							matched=true;
						}else{
							float gate=0.1;//us 
							tdcTimeSinceLastTrigger=0.001*(thisTDCTime-prevTDCTime);
							fei4TimeSinceLastTrigger=0.0256*(fei4hit->second - (fei4hit-1)->second);
	//										std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger<<std::endl; //dhynds
							if(fabs(tdcTimeSinceLastTrigger-fei4TimeSinceLastTrigger)<gate){
								matched=true;
	//											std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger<<std::endl; //dhynds
	//										std::cout<<"trigger matched"<<std::endl;
							}
							else{
								//		std::cout<<"not matched"<<std::endl;
							}
						}
						if(matched){fei4hit++;prevTDCTime=thisTDCTime;nMatched++;tdc.hFEI4MatchTime->Fill(fei4TimeSinceLastTrigger-tdcTimeSinceLastTrigger);
						}
						if(fei4hit==timeStampFei4.end()) break;
						
						
						//require at least 80% of trigger times to match. The number of triggers must be > 10 in each frame, to remove "noise" (1-hit frames etc).
						
					}
					
					if(nMatched == 1*(timeStampFei4.size())){
						frameFound=true;
						std::cout<<"fei4 frame size is "<<timeStampFei4.size()<<" and there were "<<nMatched<<" matched triggers, or "<<100*nMatched/timeStampFei4.size()<<" % "<<std::endl;
						
						frame->detectorId(std::string("FEI4"));
						frame->sourceName(std::string("TDC"));
						TDCTriggers::iterator itr=triggers->begin();
						for(fei4hit = timeStampFei4.begin();fei4hit!=timeStampFei4.end();fei4hit++){
	//						std::cout<<"time stamp is "<<fei4hit->second<<std::endl;						
							
							do{
								TDCTrigger* trigger=(*itr);
							float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
							matched=false;
							if(triggers->begin()==itr){
								matched=true;
							}else{
								float gate=0.1;//us 
								tdcTimeSinceLastTrigger=0.001*(thisTDCTime-prevTDCTime);
								fei4TimeSinceLastTrigger=0.0256*(fei4hit->second - (fei4hit-1)->second);
								//										std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger<<std::endl; //dhynds
								if(fabs(tdcTimeSinceLastTrigger-fei4TimeSinceLastTrigger)<gate){
									matched=true;
//									std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger<<std::endl; //dhynds
									
									for(int dat = fei4hit->first;dat<(fei4hit+1)->first;dat++){
										if(dat >= fei4Data.size())continue;
										std::string dataline = fei4Data[dat];
										std::vector<std::string> datalineContent;
										split(dataline," ",datalineContent);
										
										
										if(0==dataline.find("TD")){ //new trigger read out
											hit_no=-1;  
											tdid = atoi(lineContent[3].c_str());
											lv1id=-1; 
										}else if(0==dataline.find("DH")){ //new set of data. For each trigger the FEI4 can read up to 16 bunch crossings of data
											lv1id++;
											bcid=atoi(lineContent[3].c_str());
										}else if(0==dataline.find("DR")){ //data records - the real data for each bunch crossing
											hit_no++;
											int col = atoi(datalineContent[1].c_str());
											int row = atoi(datalineContent[2].c_str()); 
											int tot = atoi(datalineContent[3].c_str())+1;  
													
											if(datalineContent.size()>4){
												int tot2 = atoi(datalineContent[4].c_str());
												int tot3=tot2+1;
												int shifted_row=row+1;
//												std::cout<<"Attempting to make new trigger with bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << row << " tot " << tot << std::endl;
												trigger->push_back(tdid,lv1id,bcid,col,shifted_row,tot3,hit_no);
												hit_no++;
											}
//											std::cout<<"Attempting to make new trigger with tdid "<<tdid<<" bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << row << " tot " << tot << std::endl;
											trigger->push_back(tdid,lv1id,bcid,col,row,tot,hit_no);
										}
//										std::cout<<"Looking at line "<<dat<<": "<<fei4Data[dat]<<std::endl;
									}
									//										std::cout<<"trigger matched"<<std::endl;
								}
								else{
									//		std::cout<<"not matched"<<std::endl;
								}
							}
								if(matched){prevTDCTime=thisTDCTime;nMatched++;tdc.hFEI4MatchTime->Fill(fei4TimeSinceLastTrigger-tdcTimeSinceLastTrigger);}
								itr++;
							}while(!matched);
	

						}
					
					}
					else{
		//							std::cout<<"NOT MATCHED. fei4 frame size is "<<timeStampFei4.size()<<" and there were "<<nMatched<<" matched triggers, or "<<100*nMatched/timeStampFei4.size()<<" % "<<std::endl;
					}
					
					if(frameFound)break;
				}
			}
		
		
	

			
			
			
			
			
			
			
			
			timeStampFei4.clear();
			newFrameFei4=false;
		}
		i_x++;
	}
	
			
//
//		std::cout<<"We have "<<fei4frames->size()<<" frames in this spill"<<std::endl;
//	for(std::vector< std::vector <TDCTrigger> >::iterator fFrame=fei4frames->begin(); fFrame!=fei4frames->end();fFrame++){
//		std::vector <TDCTrigger> fei4triggers = (*fFrame);
//		std::cout<<"Matching frame with "<<fei4triggers.size()<<" triggers"<<std::endl;
//
//			bool frameFound=false;
//			double nMatchedSave=0.;
//			
//			//loop over all files
//			for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
//				if(frameFound)continue;
//				TDCFile* file=(*ifi);
//				//loop over all frames
//				for(TDCFrames::iterator ifr=file->frames()->begin();ifr!=file->frames()->end();ifr++){
//					float fei4TimeSinceLastTrigger=0;
//					float tdcTimeSinceLastTrigger=0;
//					float prevTDCTime=0;
//					bool matched=false;
//					
//					TDCFrame* frame=(*ifr);
//					TDCTriggers* triggers = frame->triggers();
//					std::vector<TDCTrigger>::iterator fei4Trig=fei4triggers.begin();
//					double nMatched=0.;
//					//check if times between triggers in tdc match time between triggers in scifi files
//					for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
//						float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
//						matched=false;
//						if(triggers->begin()==itr){
//							matched=true;
//						}else{
//							float gate=0.1;//us 
//							tdcTimeSinceLastTrigger=0.001*(thisTDCTime-prevTDCTime);
//							fei4TimeSinceLastTrigger=0.0256*((*fei4Trig).firstFei4Hit()-(*(fei4Trig-1)).firstFei4Hit());
//			//				std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger/1000<<std::endl; //dhynds
//							if(fabs(tdcTimeSinceLastTrigger-fei4TimeSinceLastTrigger)<gate){
//								matched=true;
//				//				std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger<<" and for fei4 "<<fei4TimeSinceLastTrigger<<std::endl; //dhynds
//					//			std::cout<<"trigger matched"<<std::endl;
//							}
//							else{
//								//		std::cout<<"not matched"<<std::endl;
//							}
//						}
//						if(matched){fei4Trig++;prevTDCTime=thisTDCTime;nMatched++;tdc.hFEI4MatchTime->Fill(fei4TimeSinceLastTrigger-tdcTimeSinceLastTrigger);
//}
//							if(fei4Trig==fei4triggers.end()) break;
//						
//							
//						//require at least 80% of trigger times to match. The number of triggers must be > 10 in each frame, to remove "noise" (1-hit frames etc).
//						
//					}
//						
//						if(nMatched >= 0.8*(fei4triggers.size())){
//							frameFound=true;
//		//					std::cout<<"fei4 frame size is "<<fei4triggers.size()<<" and there were "<<nMatched<<" matched triggers, or "<<100*nMatched/fei4triggers.size()<<" % "<<std::endl;
//						}
//						else{
//				//			std::cout<<"NOT MATCHED. fei4 frame size is "<<fei4triggers.size()<<" and there were "<<nMatched<<" matched triggers, or "<<100*nMatched/fei4triggers.size()<<" % "<<std::endl;
//						}
//					
//					if(frameFound)break;
//				}
//			}
//	}
//	}
//	
//	
//	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
//	
//	
//	
//	for(std::vector<std::string>::iterator i=fei4Data.begin(); i!=fei4Data.end(); i++){
//		
//		std::string *line2;
//		line2=&*i;
//		std::string line=*i;
//		std::vector<std::string> lineContent;
//		split(line," ",lineContent);
//		
//		
//		if(0==line.find("TD")){ //new trigger read out
//			//			std::cout<<"New trigger"<<std::endl;
//			if(fei4trigger->nFei4Hits() != 0){
//				fei4triggers.push_back(*fei4trigger);
//				fei4trigger->clear();
//				newTrigger=true;
//			}
//			prevBcid1=bcid;
//			lv1id=-1;
//			hit_no=-1;  
//			tdid = atoi(lineContent[3].c_str());
//			
//		}else if(0==line.find("DH")){ //new set of data. For each trigger the FEI4 can read up to 16 bunch crossings of data
//			//			std::cout<<"New bc"<<std::endl;
//			lv1id++;
//			bcid=atoi(lineContent[3].c_str());
//			if(newTrigger){
//				timeStampFei4->push_back(std::make_pair(line2,bcid));
//			}
//			if(0.0256*(bcid-prevBcid1) > 5000){ //window between frames
//				if(fei4triggers.size() > 0){
//					std::cout<<"Pushing back frame with "<<fei4triggers.size()<<" triggers"<<std::endl;
//					fei4frames->push_back(fei4triggers);
//					fei4triggers.clear();
//				}
//				prevBcid1=0;
//			}
//			
//		}else if(0==line.find("DR")){ //data records - the real data for each bunch crossing
//			//			std::cout<<"New data record"<<std::endl;
//			hit_no++;
//			int col = atoi(lineContent[1].c_str());
//			int row = atoi(lineContent[2].c_str()); 
//			int tot = atoi(lineContent[3].c_str())+1;  
//			
//			if(lineContent.size()>4){
//				int tot2 = atoi(lineContent[4].c_str());
//				int tot3=tot2+1;
//				int shifted_row=row+1;
//				//				std::cout<<"Attempting to make new trigger with bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << row << " tot " << tot << std::endl;
//				fei4trigger->push_back(tdid,lv1id,bcid,col,shifted_row,tot3,hit_no);
//				hit_no++;
//			}
//			//			std::cout<<"Attempting to make new trigger with tdid "<<tdid<<" bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << row << " tot " << tot << std::endl;
//			fei4trigger->push_back(tdid,lv1id,bcid,col,row,tot,hit_no);
//		}
//		
//	}
//	fei4Data.clear();
//	if(fei4trigger){
//		fei4triggers.push_back(*fei4trigger);
//		fei4frames->push_back(fei4triggers);
//		fei4trigger->clear();
//		fei4triggers.clear();
//	}
//
//	
//	
//	
	
	
	
	
//						//now that the right frame is found, loop over triggers and insert scifi data
//						
//						frame->detectorId(std::string("SciFi"));
//						frame->sourceName(std::string("TDC"));
//						sciFiTimeSinceLastTrigger=0;
//						tdcTimeSinceLastTrigger=0;
//						prevTDCTime=0;
//						isft=(*isff).begin();
//						
//						for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
//							float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
//							matched=false;
//							if(triggers->begin()==itr){
//								matched=true;
//							}else{
//								float gate=5000;//ns 
//								tdcTimeSinceLastTrigger=thisTDCTime-prevTDCTime;
//								sciFiTimeSinceLastTrigger=(*isft).timeSince();
//								if(fabs(tdcTimeSinceLastTrigger-sciFiTimeSinceLastTrigger)<gate){
//									matched=true;
//								}
//							}
//							if(matched){
//								std::cout<<"Matched and pushing back scifi data"<<std::endl;
//								for(int ic=0;ic<(*isft).nhits();ic++){
//									(*itr)->push_back((*isft).channel(ic),(*isft).adc(ic),(*isft).time());
//								}
//								isft++; 
//								prevTDCTime=thisTDCTime;
//							}
//							if((*isff).end()==isft) break;
//						}
//						
//						break;
//					}
//				}
//			}
//			
//			if(!frameFound){std::cout<<"Could not match with tdc. There were "<<(*isff).size()<<" triggers"<<std::endl;}

			
			
//	}
	

//	
//	
//	unsigned int fei4TriggerNumber=beginning-fei4Index.begin();
//	
//	
//	
//	
//	int fei4time=fei4TimeStamp[fei4TriggerNumber];//seconds.
//	
//	
	
	
	
//  for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
//    double nmatched=0;
//    double nunmatched=0;
//    std::cout<<"New file!"<<std::endl;
//    TDCFile* file=(*ifi);
//    int tdctime=file->timeStamp();
//    TDCFrames* frames=(*ifi)->frames();
//    bool prev_Odd=false;
//    int nOdd=0;
//    for(TDCFrames::iterator ifr=frames->begin();ifr!=frames->end();ifr++){
//      if(nmatched!=0 || nunmatched!=0){
//        tdc.hFEI4FractionMatched->Fill((nmatched/(nmatched+nunmatched)));
//        std::cout<<"New frame! "<<nmatched<<" matched hits and "<<nunmatched<<" unmatched hits gives fraction "<<(nmatched/(nmatched+nunmatched))<<std::endl;
//      }
//      nmatched=0;
//      nunmatched=0;
//      bool newFrame=true;
//      double time_tdc=0;
//      double time_tdc_prev=0;
//      double time_fei4=0;
//      double time_fei4_prev=0;
//      TDCFrame* frame=(*ifr);
//      frame->detectorId(std::string("FEI4"));
//      frame->sourceName(std::string("TDC"));
//      TDCTriggers* triggers = frame->triggers();
//      for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
//        bool trigger_match=false;
//        bool newTriggerFlagged=true;
//        TDCTrigger* trigger=(*itr);
//        
//        if(newFrame){
//          time_tdc_prev=trigger->timeAfterShutterOpen();
//        }
//        time_tdc=trigger->timeAfterShutterOpen();
//        
//        std::cout<<"TDC time difference "<<time_tdc-time_tdc_prev<<std::endl;
//          
//        //FEI4: files are typically written ~10 seconds after each TDC file is started
//        unsigned int fei4TriggerNumber=beginning-fei4Index.begin();
//        int fei4time=fei4TimeStamp[fei4TriggerNumber];//seconds.
//        
//        //FEI4 time is in fact the timestamp of the FEI4 file!!
//        
//        std::cout<<"FEI4 trigger arrived at "<<fei4time<<" with tdc trigger at "<<tdctime<<std::endl;
//        //FEI4: current TDC trigger is in a file that is timestamp-matched with a FEI4 file that this FEI4 trigger came from. So store any hits found!
//        
//        if(abs(fei4time-tdctime)<10){ //dhynds1
//          int tdid=0; int lv1id=-1; int bcid=0; int hit_no=-1; double RCE_time_first=777465877; double RCE_time=0;//dhynds1
//        //  std::cout<<"starting new event"<<std::endl; //dhynds1
//          for(unsigned int ii=(*beginning);ii<(*end);ii++){
//            std::string line=fei4Data[ii];
//            std::vector<std::string> lineContent;
//            split(line," ",lineContent);
//         //   int tdid=0; int lv1id=0; int bcid=0;
//            if(0==line.find("TD")){ 
//              std::cout<<"new TD flag"<<std::endl; //dhynds1
//                lv1id=-1;
//                hit_no=-1;  
//              tdid = atoi(lineContent[3].c_str());
//            }else if(0==line.find("DH")){
//              lv1id++;
//             // dhynds1 lv1id= atoi(lineContent[2].c_str()); 
//              
//              //BCID is in fact RCE 64-bit timestamp!!!!
//              
//              bcid=  atoi(lineContent[3].c_str());
//              
//              if(newFrame){
//                time_fei4_prev=bcid;
//              }
//              time_fei4=bcid;
//              std::cout<<"time_fei4 "<<time_fei4<<" and time_fei4_prev "<<time_fei4_prev<<std::endl;
//              std::cout<<"FEI4 time difference "<<0.0256*(time_fei4-time_fei4_prev)<<" and tdc time difference "<<0.001*(time_tdc-time_tdc_prev)<<std::endl;
//              
//              if(abs((0.0256*(time_fei4-time_fei4_prev))-(0.001*(time_tdc-time_tdc_prev)))<2){
//                std::cout<<"MATCH!!!"<<std::endl;
//                std::cout<<"Matched time is: "<<0.0256*(RCE_time-RCE_time_first)<<std::endl;
//                if(newTriggerFlagged){
//                  tdc.hFEI4MatchTime->Fill((0.0256*(time_fei4-time_fei4_prev))-(0.001*(time_tdc-time_tdc_prev)));
//                  nmatched++;
//                }
//                trigger_match=true;
//                if(prev_Odd){std::cout<<"N odd was "<<nOdd<<std::endl;}
//                prev_Odd=false;
//                nOdd=0;
//              }
//              else{
//                if(newTriggerFlagged){
//                  nunmatched++;
//                }
//                std::cout<<"NOMATCH - fei4 time difference "<<0.0256*(time_fei4-time_fei4_prev)<<" and tdc time difference "<<0.001*(time_tdc-time_tdc_prev)<<std::endl;
//                if(0.001*(time_tdc-time_tdc_prev)<2){std::cout<<"Too small"<<std::endl;}
//                if(0.0256*(time_fei4-time_fei4_prev)<0.001*(time_tdc-time_tdc_prev)){
//                  std::cout<<"Weird timing"<<std::endl;
//                  beginning++;
//                  end++;
//                  beginning++;
//                  end++; //Try to catch up by 1 place (trigger number will increase by 1 as move to next trigger - increase FEI4 hit by 2 to try and even out)
//                  if(prev_Odd){
//                    nOdd++;
//                  }
//                  prev_Odd=true;
//                }
//                if(prev_Odd){std::cout<<"N odd was "<<nOdd<<std::endl;}
//                prev_Odd=false;
//                nOdd=0;
//                break;}
//              
//              RCE_time=  atoi(lineContent[3].c_str());
//              std::cout<<"RCE time is: "<<0.0256*(RCE_time-RCE_time_first)<<std::endl;
//              std::cout<<"Lv1id is "<<lv1id<<" with bcid "<<bcid<<endl;
//              std::cout<<"Trigger time is: "<<0.0256*(bcid-RCE_time_first)<<std::endl;
//              if(newTriggerFlagged){
//                float thisTime=trigger->timeAfterShutterOpen()+frame->openTime();
//                if(not firstEntry){
//                  int diffFEI4=bcid-prevBcid;
//                  if(prevBcid>bcid){diffFEI4=(bcid+256)-prevBcid;}
//                  while(prevTime>thisTime){prevTime-=(2048);}//DHYNDS - is this right??
//                  float diffTDC=(thisTime-prevTime)/(256*25);
//                  diffTDC=(diffTDC-int(diffTDC))*(256);
//                  tdc.hFEI4TimeDiff->Fill( 25*(diffFEI4-diffTDC) );
//                  if(false){
//                    std::cout<<(itr==triggers->begin()?"\n":"")<<" trigger "<<(itr-triggers->begin())<<" in frame "<<(ifr-frames->begin())<<"\t"<<std::flush;
//                    std::cout<<"Delta(BCID):  FEI4=("<<bcid<<"-"<<prevBcid<<")="<<diffFEI4<<"  \t"<<std::flush;
//                   std::cout<<" TDC=("<<thisTime<<"-"<<prevTime<<")%(256*25)="<<diffTDC<<(fabs(diffTDC-diffFEI4)>0.2?"  ****** ? ******  ":"")<<std::endl;
//                  }
//                }
//                newTriggerFlagged=false;
//                if(firstEntry) firstEntry=false;
//                prevTime=thisTime;
//                prevBcid=bcid;
//              }
//            }else if(0==line.find("DR")){
//                hit_no++;
//              int col = atoi(lineContent[1].c_str());
//              int row = atoi(lineContent[2].c_str()); 
//              int tot = atoi(lineContent[3].c_str())+1; //dhynds1 
//                //dhynds1 from now
//              //  std::cout<<line<<std::endl;
//              //  std::cout<<"line has size "<<lineContent.size()<<std::endl;
//                if(lineContent.size()>4){
//                    int tot2 = atoi(lineContent[4].c_str());
//               //     std::cout<<"entry is : "<<atoi(lineContent[4].c_str())<<std::endl;                
//                        std::cout<<"Extra hit!"<<std::endl;
//                        int tot3=tot2+1;
//               //         std::cout<<"Pushing back extra hit"<<std::endl;
//                    int shifted_row=row+1;
//                    std::cout<<"Attempting to make extra new trigger with bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << shifted_row << " tot " << tot3 << std::endl;
//                        trigger->push_back(tdid,lv1id,bcid,col,shifted_row,tot3,hit_no);
//                    hit_no++;
//                }//till now
//              std::cout<<"Attempting to make new trigger with bcid "<<bcid<<" lv1id "<<lv1id<< " col " << col << " row " << row << " tot " << tot << std::endl;
//              trigger->push_back(tdid,lv1id,bcid,col,row,tot,hit_no);
//            }
//          }
//          // FEI4: increment to next fei4 trigger [i.e. next block of BCIDs and their hits]
//          if(trigger_match){
//            beginning++;
//            end++;
//            time_fei4_prev=time_fei4;
//            time_tdc_prev=time_tdc;
//          }
//        }else if(fei4time<tdctime){
//          beginning++; end++;
//        }//... else fei4time>tdctime, i.e in the future w.r.t. tdctime, spiral over TDCs until it catches up
//        newFrame=false;
//        if(0.0256*(time_fei4_prev-time_fei4)>2000){
//          std::cout<<"BREAKING"<<std::endl;
//          break;
//        }
//      }
//    }
//  }
//	
	
	
	
	
  return matchTDCandTelescope();
}


int Amalgamation::checkTimeRange(int first,int last)
{
  if(parameters->useRelaxd){// I mean, use the telescope
    if(index.size()){
      lastTimepixTime=index.back().roughtime();
      firstTimepixTime=index.front().roughtime();
      if(lastTimepixTime<first || firstTimepixTime>last){
        std::cout<<firstTimepixTime<<" < timepix < "<<lastTimepixTime<<"    "<<first<<" < ext.DUT < "<<last<<"      "<<std::flush;
        std::cout<<de(firstTimepixTime)<<" < timepix < "<<de(lastTimepixTime)<<"    "<<de(first)<<" < ext.DUT < "<<de(last)<<std::endl;
        std::cout<<"No overlap between external DUT and Timepix data. Exiting."<<std::endl;
        return 1;
      }
    }
  }
  return 0;
}


int Amalgamation::indexVetraFile()
{
  float tdc_time=0;
  float tdc_time_prev=0;
  float beetle_time=0;
  float beetle_time_prev=0;
  long int beetle_num=0;
  long int beetle_num_prev=0;  
  float tdc_file_diff=0;
  float tdc_file_prev=0;
  float spill_length=0;
  
  if(parameters->verbose) std::cout << "Loading Vetra information" << std::endl;
  VetraMapping vetraMap;
  // Open the ntuple file that was written by the from VETRA job
  TFile* inputFile=TFile::Open(parameters->vetraFile.c_str(),"READ");
  VetraNtupleClass *ntuple=new VetraNtupleClass((TTree*)inputFile->Get("BeetleTupleDumper/tuple"));
  TTree* chain=ntuple->fChain;
  if(chain==NULL){
    std::cout << "Can't open ntuple in " << parameters->vetraFile << std::endl;
    return 1;
  }
  
  // Sumarize vetra timestamps [begin-spill] in a convenient vector
  int vetratime=0;
  std::vector<int> vetraStartSpillDate;
  std::vector<int> vetraStartSpillTime;
  std::vector<int> vetraStartSpillEntry;
  std::vector<int> vetraStartFrameEntry;
  std::vector<bool> vetraSpillMatched;
  int nEntries=ntuple->fChain->GetEntries();
  float tell1_time=0;
  float tell1_time_prev=0;
  int triggers_per_frame=0;
  int beetle_frames_spill=0;
  for(int e=0;e<nEntries;e++){
    ntuple->GetEntry(e);
    int d=ntuple->spilldate;
    int t=ntuple->spilltime;
    //int year=d/10000;
    int month=d%10000/100;
    int day=d%100;
    int hour=t/10000;
    int min=t%10000/100;
    int sec=t%100;
    int time=(month*100+day)*86400+3600*hour+60*min+sec;
    tell1_time=0.001*25*ntuple->clock;
    if(tell1_time_prev == 0){
      vetraStartFrameEntry.push_back(e);
      tell1_time_prev=tell1_time;
    }
    
    if((tell1_time-tell1_time_prev)<0){
      tell1_time_prev-=(0.001*25*10737418240000);
    }
    
    if((tell1_time-tell1_time_prev)>4000){
      std::cout<<"Triggers in beetle frame "<<triggers_per_frame<<std::endl;
      triggers_per_frame=0;
      vetraStartFrameEntry.push_back(e);
      beetle_frames_spill++;
    }
    if((tell1_time-tell1_time_prev)>4000000){
      std::cout<<"NEW SPILL IN VETRA FROM CLOCK!!!"<<std::endl;     
    }
    
    if(vetratime!=time){
      std::cout<<"Vetra clock difference "<<0.000001*(tell1_time-tell1_time_prev)<<" seconds"<<std::endl;
    }
      
      triggers_per_frame++;
    tell1_time_prev=tell1_time;
    
    if(vetratime==time) continue;
    std::cout<<"There were "<<beetle_frames_spill<<" vetra frames"<<std::endl;
    beetle_frames_spill=0;
    std::cout<<"NEW SPILL IN VETRA FROM TIME!!!"<<std::endl;
    vetraStartSpillDate.push_back(d);
    vetraStartSpillTime.push_back(time);
    vetraStartSpillEntry.push_back(e);
    vetraSpillMatched.push_back(false);
    vetratime=time;
    
    std::cout<<"From .mdf file "<<vetraStartSpillDate.back()<<" on "<<day<<"/"<<month<<" at "<<hour<<":"<<(min>9?"":"0")<<min<<":"<<(sec>9?"":"0")<<sec<<".";
    std::cout<<" T="<<vetraStartSpillTime.back()<<", starting at "<<vetraStartSpillEntry.back();
    if(vetraStartSpillEntry.size()>1) std::cout<<" (+"<<vetraStartSpillEntry.back()-vetraStartSpillEntry[vetraStartSpillEntry.size()-2]<<")";
    std::cout<<std::endl;
  }
  // Add on a last entry marking the end of run.
  vetraStartSpillDate.push_back(ntuple->spilldate);
  vetraStartSpillTime.push_back(vetratime+45);//45s = approx time between spills. Dhynds changed to 35
  vetraStartSpillEntry.push_back(nEntries);
  vetraStartFrameEntry.push_back(nEntries);
  vetraSpillMatched.push_back(false);
  
  // Identify the time of the first/last vetra event 
  // (start of first/end of last fill in vetra nTuple)
  firstDate=vetraStartSpillDate.front();
  lastDate=vetraStartSpillDate.back();
  firstTime=vetraStartSpillTime.front();
  lastTime=vetraStartSpillTime.back();
  
  if(checkTimeRange(firstTime,lastTime)){return 1;}
  
  tdc.createTimeTrendHistos(firstTime,lastTime);
  for(unsigned int ivs=0;ivs<vetraStartSpillTime.size();ivs++){
    tdc.hDUTTime->Fill(vetraStartSpillTime[ivs]-firstTime,3);
  }  
  // Now we have identified the time range over which we want to have 
  // TDC information, read and index the relevant TDC information
  indexTDCFiles();
  
  // Loop over TDCFiles (1/spill) and match them in time to the spill[time] as recorded in the
  // vetra ntuple. Then match [in exact order] the TDCTriggers with the entries in the vetra nTuple.
  int px=0;
  int nBadSpills=0;
  int startSpillAt=-1;
  int startNextSpillAt=-1;
  int startFrameAt=vetraStartFrameEntry[px];
  int startNextFrameAt=vetraStartFrameEntry[px+1];
  int nTriggersTDC=0;
  int nTriggersBeetle=0;
  int ntupleEntry=0;
  int frameNo=1;
  int spillNo=0;
  // loop over TDCFiles noting the rough (precise to seconds) time of the file[=spill]
  for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
    std::cout<<std::endl<<"New tdc file"<<std::endl;

    int no_bad=0;
    TDCFile* file=(*ifi);
    
    ///////
    TDCFrames* frames=(*ifi)->frames();
    int nZero=0;
    std::vector<int>* zeroFrames=(*ifi)->zeroFrames();
    std::vector<int> zeroFramesTemp;
    int i=0;
    for(TDCFrames::iterator ifr=frames->begin();ifr!=frames->end();ifr++){
      TDCFrame* frame=(*ifr);
      nTriggersTDC=frame->nTriggersInFrame();
      if(nTriggersTDC == 0){// && i > 10){ //added i > 10
        nZero++;
        zeroFramesTemp.push_back(i);
        zeroFrames->push_back(i);
        std::cout<<"Zero frame is number "<<i<<std::endl;
      }
      i++;
    }
    std::cout<<"attached zero size is "<<zeroFrames->size()<<std::endl;
    for(i=0;i<zeroFramesTemp.size();i++){
      std::cout<<"Frames has size "<<frames->size()<<", deleting entry "<<zeroFramesTemp[i]<<std::endl;
      frames->erase(frames->begin()+zeroFramesTemp[i]);
			std::cout<<"New frames size is "<<frames->size()<<std::endl;
      for(int j=i+1;j<zeroFramesTemp.size();j++){
        zeroFramesTemp[j]--;
        std::cout<<"Subsequent zero frame reference is "<<zeroFramesTemp[j]<<std::endl;
      }
    }
    
//    std::cout<<"There were "<<nZero<<" tdc frames with zero triggers"<<std::endl;
    
    std::cout<<"There were "<<frames->size()<<" tdc frames"<<std::endl;
    std::cout<<"with "<<file->nTriggersInFile()<<" triggers"<<std::endl;
    
    
    int tdctime=file->timeStamp();
//    std::cout<<"TDC filestamp was "<<1000000000*tdctime<<std::endl;
    if(tdc_file_prev == 0){
      tdc_file_prev=tdctime;
    }
    tdc_file_diff=tdctime-tdc_file_prev;
    tdc_time+=(1000000000*tdc_file_diff-spill_length);
//    std::cout<<"first tdc time "<<tdc_time<<". Time stamp was "<<tdctime<<" and prev tdc time was "<<tdc_time_prev<<std::endl;
    tdc_time_prev=tdc_time;
    tdc_file_prev=tdctime;
    // loop over pre-recorded vetra blocks (each block = one mdf file = one spill)
    bool doesSomething=false;

    
    for(unsigned int ivs=0;ivs<vetraStartSpillTime.size();ivs++){
      // Require the TDC file to have been written after the mdf file
      // and before the the next mdf file was started. This makes a unique match
      std::cout<<"tdc time is "<<tdctime<<std::endl;
      std::cout<<"Vetra spill time is "<<vetraStartSpillTime[ivs]<<", the next spill is at "<<vetraStartSpillTime[ivs+1]<<std::endl;
      
      if(vetraStartSpillTime[ivs]<tdctime&&vetraStartSpillTime[ivs+1]<tdctime){
        for(int k=vetraStartSpillEntry[ivs];k<vetraStartSpillEntry[ivs+1];k++){
          if(k == vetraStartFrameEntry[px]){
            px++;
          }
        }
      }
            
      if(vetraStartSpillTime[ivs]<tdctime&&vetraStartSpillTime[ivs+1]>tdctime){
        if(vetraSpillMatched[ivs]){
          std::cout<<" WARNING vetra block "<<ivs<<"/"<<vetraStartSpillTime.size()
          <<" already matched! Passing to next block without storing."<<std::endl;
          continue;
        }
        vetraSpillMatched[ivs]=true;
        vetratime=vetraStartSpillTime[ivs];
        startSpillAt=vetraStartSpillEntry[ivs];
        startNextSpillAt=vetraStartSpillEntry[ivs+1];
        std::cout<<"Passed loop"<<std::endl;
        doesSomething=true;
        break;
      }
    }
    
    if(!doesSomething) continue;
    
    
    
//    vetraSpillMatched[spillNo]=true;
//    startSpillAt=vetraStartSpillEntry[spillNo];
//    startNextSpillAt=vetraStartSpillEntry[spillNo+1];
//    doesSomething=true;
    spillNo++;
    
    
    
    
    if(doesSomething){
      std::cout<<"The thing did something!"<<std::endl;
    }
    else{
      std::cout<<"Bastaaard"<<std::endl;
    }
    //std::cout <<  << std::endl;
    
    // We rely wholy on the number of sync. triggers in a TDCfile [=spill] to match exactly
    // with the number of triggers [Gaudi-events] from each mdf file [=spill]
    int nUnmatched=( startNextSpillAt-startSpillAt ) - ( file->nTriggersInFile() );
    file->nUnmatchedTriggers( nUnmatched );
    std::cout<<"Number of unmatched triggers is "<<nUnmatched<<std::endl;
    
    
    int nFramesVetra=0;
    int ntupleEntry=startSpillAt;

    std::cout<<"Debug: frameNo "<<vetraStartFrameEntry[px]<<", startSpill "<<startSpillAt<<", startNextSpill "<<startNextSpillAt<<std::endl;
    int px_or=px;
    std::cout<<"px is "<<px<<std::endl;
    for(int k=startSpillAt;k<startNextSpillAt;k++){
      if(k == vetraStartFrameEntry[px]){
        nFramesVetra++;
        px++;
      }
    }
    px=px_or;
    std::cout<<"px is "<<px<<std::endl;

    std::cout<<"There were "<<nFramesVetra<<" vetra frames in the match"<<std::endl;
    std::cout<<"Current ntuple entry "<<ntupleEntry<<", started new spill at entry "<<startSpillAt<<" and finishes at "<<startNextSpillAt<<std::endl;
    
    
//dhynds    if(frames->size() != nFramesVetra) continue;//dhynds
    
    tdc.hUnmatched->Fill(nUnmatched);
    if(nUnmatched){ 
      nBadSpills++;
      std::cout<<"\r"<<nBadSpills<<" spill"<<(1==nBadSpills?" doesn't":"s don't")<<" match.  nT(vetra-tdc): "
      <<(startNextSpillAt-startSpillAt)<<"-"<<file->nTriggersInFile()
      <<".  "<<abs(nUnmatched)<<" triggers "<<(nUnmatched>0?"more":"less")<<" in .mdf file"<<std::endl;
    }
		else{
			std::cout<<"==== No unmatched triggers ===="<<std::endl;
		}
    // Loop over each frame and each TDCtrigger and populate the vetra hit information
    // The Gaudi-events from Vetra are simply read in order and given to the TDCTriggers 
    // in order ... so skipped/extra triggers are really bad news.
//d    int ntupleEntry=startSpillAt;
    
    if(nUnmatched !=44444440){
//dhynds    TDCFrames* frames=(*ifi)->frames();
    spill_length=0;
    beetle_time_prev=-10737418240000;
    beetle_num_prev=0;
    bool sync=false;
    while(ntupleEntry<startNextSpillAt){

      for(TDCFrames::iterator ifr=frames->begin();ifr!=frames->end();ifr++){


        startFrameAt=vetraStartFrameEntry[px];
        startNextFrameAt=vetraStartFrameEntry[px+1];
        nTriggersBeetle=startNextFrameAt-startFrameAt;

/*        if(!sync){
          TDCFrame* frame=(*ifr);
          std::cout<<"TDC syncing with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
          std::cout<<"Beetle syncing with "<<nTriggersBeetle<<" triggers"<<std::endl;
          ntupleEntry+=(nTriggersBeetle);
          px++;
          startFrameAt=vetraStartFrameEntry[px];
          startNextFrameAt=vetraStartFrameEntry[px+1];
          nTriggersBeetle=startNextFrameAt-startFrameAt;
          ntupleEntry+=(nTriggersBeetle);
          px++;
          startFrameAt=vetraStartFrameEntry[px];
          startNextFrameAt=vetraStartFrameEntry[px+1];
          nTriggersBeetle=startNextFrameAt-startFrameAt;
          do {
            ifr++;
            std::cout<<"syncing..."<<std::endl;
          } while ((*ifr)->nTriggersInFrame() != nTriggersBeetle && (ifr+2)!=frames->end());
        }
*/        
        sync=true;
        
        bool newframe=true;
        double tdc_trigger_time_prev=0;
        double tdc_trigger_time=0;
        tdc_time+=10000000;
        spill_length+=10000000;
        TDCFrame* frame=(*ifr);

        frame->detectorId(std::string("Vetra"));
        frame->sourceName(std::string("TDC"));
        TDCTriggers* triggers = frame->triggers();
        
        nTriggersTDC=frame->nTriggersInFrame();
        
   //     if(((ifr+1)!=frames->end()) && ((*(ifr+1))->nTriggersInFrame() == 0)){
   //       continue;
   //     }
        
   //     if(((ifr+1)!=frames->end()) && (nTriggersTDC == 0)){
   //       ifr++;
   //       continue;
   //     }
        
        
//        std::cout<<"New tdc frame with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
//        std::cout<<"New beetle frame with "<<nTriggersBeetle<<" triggers"<<std::endl;
//        std::cout<<"Looking at frame starting beetle entry "<<startFrameAt<<std::endl;
//        std::cout<<"Next spill at entry "<<startNextSpillAt<<std::endl;
	
        if(nTriggersTDC == nTriggersBeetle){
          
          std::cout<<"New tdc frame with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
          std::cout<<"New beetle frame with "<<nTriggersBeetle<<" triggers"<<std::endl;

          
          std::cout<<"MATCH"<<std::endl;
          for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
          TDCTrigger* trigger=(*itr);
          tdc_trigger_time=trigger->timeAfterShutterOpen();
          tdc_time+=(tdc_time+trigger->timeAfterShutterOpen()-tdc_time_prev);
          spill_length+=(tdc_time+trigger->timeAfterShutterOpen()-tdc_time_prev);
          //do something here with timing
//          std::cout<<"TDC time is "<<frame->openTime()<<" . "<<trigger->timeAfterShutterOpen()<<std::endl;
//          std::cout<<"Frame open time "<<frame->openTime()<<std::endl;
//          std::cout<<"File timestamp is "<<(1000000000*(file->timeStamp()))<<std::endl;
          if((1000000000*(file->timeStamp())) < 0){
//            std::cout<<"Negative file timestamp with ntuple entry "<<ntupleEntry<<std::endl;
          }
          //std::cout<<"TDC time is "<<tdc_time<<std::endl;
          tdc_time_prev=tdc_time;
          
          beetle_time=25*ntuple->clock;
          beetle_num=ntuple->eventinspill;
          if(beetle_time < beetle_time_prev){
            beetle_time+=107374182400;
          }
          
          if(newframe){
            tdc_trigger_time_prev=tdc_trigger_time;
            beetle_time_prev=beetle_time;
            newframe=false;
          }
//          std::cout<<"beetle_time "<<beetle_time<<" and beetle_time_prev "<<beetle_time_prev<<std::endl;
//          std::cout<<"Ntuple clock is "<<(ntuple->clock)<<std::endl;
//          std::cout<<std::endl;
//          std::cout<<"Eventinspill is "<<ntuple->eventinspill<<std::endl;
//          std::cout<<"TDC time difference "<<0.001*(tdc_trigger_time-tdc_trigger_time_prev)<<std::endl;
          int mod = floor(0.001*(tdc_trigger_time-tdc_trigger_time_prev)/8)*8;
          if(mod==0 && tdc_trigger_time_prev!=tdc_trigger_time){
            mod=8;
          }
          
//          cout<<"TDC mod 8: "<<mod<<std::endl;
//          std::cout<<"Event num difference "<<(beetle_num-beetle_num_prev)<<std::endl;
          if(abs(beetle_num-beetle_num_prev)>1){
//            std::cout<<"GOTCHA"<<std::endl;
          }
//          std::cout<<"Beetle time difference "<<0.001*(beetle_time-beetle_time_prev)<<std::endl;
          if(abs(0.001*(beetle_time-beetle_time_prev)-mod)<1){
//            std::cout<<"MATCH"<<std::endl;
          }
          
          
          if(0.001*(beetle_time-beetle_time_prev) > 10000){
//            std::cout<<"New beetle frame!!"<<std::endl;
//get rid            break;
          }
          if(0.001*(beetle_time-beetle_time_prev) > 4000000){
//            std::cout<<"New Spill!!"<<std::endl;
          }
          
          trigger->clock(ntuple->clock);
//dhynds						std::cout<<"before get entry there are "<<ntuple->nhits<<" hits"<<std::endl;
          ntuple->GetEntry(ntupleEntry);
//dhynds					std::cout<<"Picked up ntuple entry "<<ntupleEntry<<" with "<<ntuple->nhits<<" hits"<<std::endl;
            int testbit=0;
          for(int ihit=0;ihit<ntuple->nhits;ihit++){
						testbit++;
            int link=(int)ntuple->link[ihit];
            int chan=(int)ntuple->chan[ihit];
            int adc=(int)ntuple->adc[ihit];
            int strip=vetraMap.Tell1chToStrip[link*32+chan];
//dhynds						std::cout<<"Pushed back hit "<<testbit<<" with link "<<link<<" chan "<<chan<<" adc "<<adc<<" strip "<<strip<<std::endl;
            trigger->push_back(link,chan,adc,strip);
          }
          //beetle_time_prev=25*ntuple->clock;
          beetle_time_prev=beetle_time;
          beetle_num_prev=beetle_num;
          tdc_trigger_time_prev=tdc_trigger_time;
          ntupleEntry++;
//            std::cout<<"NtupleEntry is "<<ntupleEntry<<std::endl;
        }
        
          
          
        }
        
        else{
          
          if(nTriggersTDC != 0){
            ntupleEntry+=(nTriggersBeetle);
            std::cout<<"JUMPING! NtupleEntry is "<<ntupleEntry<<" and there were "<<nTriggersTDC<<" tdc triggers"<<std::endl;
          }
          
          if(abs(nTriggersTDC-nTriggersBeetle)<2){
            std::cout<<"Only small difference"<<std::endl;
            std::cout<<"TDC frame not matched with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
            std::cout<<"Beetle frame not matched with "<<nTriggersBeetle<<" triggers"<<std::endl;
          }
					else{
						std::cout<<"Big difference. TDC frame not matched with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
						std::cout<<"Beetle frame not matched with "<<nTriggersBeetle<<" triggers"<<std::endl;
					}
/*          else{
//            std::cout<<"Dont match"<<std::endl;
//            std::cout<<"TDC frame not matched with "<<frame->nTriggersInFrame()<<" triggers"<<std::endl;
//            std::cout<<"Beetle frame not matched with "<<nTriggersBeetle<<" triggers"<<std::endl;
//
//          
//            if(frame->nTriggersInFrame() > nTriggersBeetle){
//              double time_prev=0;
//              for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
//                TDCTrigger* trigger=(*itr);
//                tdc_trigger_time=trigger->timeAfterShutterOpen();
//                std::cout<<"TDC trigger timing is "<<tdc_trigger_time<<" which is "<<tdc_trigger_time-time_prev<<" after the last trigger"<<std::endl;
//                if(tdc_trigger_time-time_prev < 150){
//                  no_bad++;
//                }
//                time_prev=tdc_trigger_time;
//              }
//            }
//            else{
//              std::cout<<"Beetle gained hits"<<std::endl;
//            }
//          }
*/        
				}
        
        if(nTriggersTDC != 0){ //dhynds added this condition
					px++;
				}
        
      }
      std::cout<<"Breaking"<<std::endl;
      break;
    }
    }
    else{
      std::cout<<"CRAZY"<<std::endl;
      int ntupleEntry=startSpillAt;
      TDCFrames* frames=(*ifi)->frames();
      spill_length=0;
      beetle_time_prev=0;
      while(ntupleEntry<startNextSpillAt){
        for(TDCFrames::iterator ifr=frames->begin();ifr!=frames->end();ifr++){
          tdc_time+=10000000;
          spill_length+=10000000;
          TDCFrame* frame=(*ifr);
          frame->detectorId(std::string("Vetra"));
          frame->sourceName(std::string("TDC"));
          TDCTriggers* triggers = frame->triggers();    
          for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
            TDCTrigger* trigger=(*itr);
            
            tdc_time+=(tdc_time+trigger->timeAfterShutterOpen()-tdc_time_prev);
            spill_length+=(tdc_time+trigger->timeAfterShutterOpen()-tdc_time_prev);
            //do something here with timing
            std::cout<<"TDC time is "<<frame->openTime()<<" . "<<trigger->timeAfterShutterOpen()<<std::endl;
            //          std::cout<<"Frame open time "<<frame->openTime()<<std::endl;
            //          std::cout<<"File timestamp is "<<(1000000000*(file->timeStamp()))<<std::endl;
            if((1000000000*(file->timeStamp())) < 0){
              //            std::cout<<"Negative file timestamp with ntuple entry "<<ntupleEntry<<std::endl;
            }
            //std::cout<<"TDC time is "<<tdc_time<<std::endl;
            tdc_time_prev=tdc_time;
            
            beetle_time=25*ntuple->clock;
            if(beetle_time < beetle_time_prev){
              beetle_time+=107374182400;
            }
            //    std::cout<<"Beetle time difference "<<(beetle_time-beetle_time_prev)<<std::endl;
            
            trigger->clock(ntuple->clock);
            ntuple->GetEntry(ntupleEntry);
            for(int ihit=0;ihit<ntuple->nhits;ihit++){
              int link=(int)ntuple->link[ihit];
              int chan=(int)ntuple->chan[ihit];
              int adc=(int)ntuple->adc[ihit];
              int strip=vetraMap.Tell1chToStrip[link*32+chan];
              trigger->push_back(link,chan,adc,strip);
            }
            beetle_time_prev=25*ntuple->clock;
            ntupleEntry++;
          }
        }
        //      std::cout<<"Spill length was "<<spill_length<<std::endl;
        break;
      }
      
    }
    std::cout<<"Number of bad triggers was "<<no_bad<<std::endl;
  }
  if(parameters->verbose) std::cout<<"\n"<<std::endl;
  
  return matchTDCandTelescope();
}
   





int Amalgamation::indexSciFiData()
{
  std::string prefix("SciFi_");
  std::vector<std::string> files;
  
  // Fibre tracker: find files in directory structure
  CommonTools::getdir(parameters->sciFiDirectory,files);
  if(files.size()==0){
    std::cout <<"'"<<parameters->sciFiDirectory<<"' is empty!"<< std::endl;
    return 1;
  }
  
  bool isZS=false;
  bool checkedForZS=false;
  // Fibre tracker: loop over files noting file time
  std::vector< std::pair< std::string,int > > filesAndTimes;
  for (unsigned int i = 0;i < files.size();i++) {
    if( files[i].find(".txt")==std::string::npos){ continue; }
    
    std::string dateandtimeline=files[i].substr(prefix.size(),files[i].rfind(".")-prefix.size());
    std::string dateline=dateandtimeline.substr(0,dateandtimeline.find("_"));
    std::string timeline=dateandtimeline.substr(dateandtimeline.find("_")+1,dateandtimeline.size()-dateandtimeline.find("_"));
    
    std::vector<std::string> lineContent;
    split(dateline," ",lineContent);
    std::vector<std::string>::iterator it=lineContent.begin();
    
    int year =atoi( dateline.substr(0,4).c_str() );
    int month=atoi( dateline.substr(4,2).c_str() );
    int day  =atoi( dateline.substr(6,2).c_str() );
    int hour =atoi( timeline.substr(0,2).c_str() );
    int min  =atoi( timeline.substr(2,2).c_str() );
    int sec  =atoi( timeline.substr(4,2).c_str() );
    int date=year*10000+month*100+day;
    int time=hour*3600+min*60+sec;
    int fulltime=(month*100+day)*86400+time;
    if(0==firstDate){
      firstDate=date; 
      lastDate=date;
      firstTime=fulltime;
      lastTime=fulltime;
    }else{
      if(fulltime<firstTime){
        firstDate=date;
        firstTime=fulltime;
      }
      if(fulltime>lastTime){
        lastDate=date;
        lastTime=fulltime;
      }
    }    
    if(parameters->verbose){std::cout<<"Logging SciFi file "<<i+1<<"/"<<files.size()<<"  "<<files[i]<<" ar "<<de(fulltime)<<std::endl;}
    std::string fullpath(parameters->sciFiDirectory+slash+files[i]);
    filesAndTimes.push_back(std::make_pair(fullpath,fulltime));
    
    if(not checkedForZS){
      isZS=CommonTools::fileContains("#",fullpath,512);
      checkedForZS=true;
    }
  }
  
  if(checkTimeRange(firstTime,lastTime)){return 1;}
  tdc.createTimeTrendHistos(firstTime,lastTime);
  for(unsigned int ivs=0;ivs<filesAndTimes.size();ivs++){
    
    tdc.hDUTTime->Fill(filesAndTimes[ivs].second-firstTime,3);
  }  
  indexTDCFiles();  

  // We rely wholy on the number of sync. triggers in a TDCfile [=spill] to match exactly
  // with the number of entries in the SciFi file
  for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
    TDCFile* file=(*ifi);
    int tdctime=file->timeStamp();
    
    //find relevant SciFi file
    std::vector< std::pair< std::string,int > >::iterator iclosestpair=filesAndTimes.begin();
    std::vector< std::pair< std::string,int > >::iterator ipair=filesAndTimes.begin();
    for(;ipair!=filesAndTimes.end();ipair++){
      int closestSoFar=(*iclosestpair).second;
      int sciFiTime=(*ipair).second;
      if(abs(tdctime-sciFiTime) <
         abs(tdctime-closestSoFar)){
        iclosestpair=ipair;
      }
    }
    if(abs(tdctime-((*iclosestpair).second))>40) continue;

     std::cout<<"TDC file at: "<<de(tdctime)<<" with "<<file->nTriggersInFile()<<" triggers across "<<file->nFramesInFile()<<" frames <---> SciFi at: "
              <<de((*iclosestpair).second)<<" which has "<<CommonTools::countLines((*iclosestpair).first)/128<<" triggers"<<std::endl;


		// Quickly loop over SciFi file and load in data
    std::fstream file_op(((*iclosestpair).first).c_str(),std::ios::in);
    SciFiTriggers* sciFiTriggers=0;
    bool newTriggerFlagged=true;
    SciFiFrames sciFiFrames;
		int nTriggers=0;
    while(not file_op.eof()){
      char str[2048];
      file_op.getline(str,2048);
      if( isZS&&std::string(str)==hash){newTriggerFlagged=true;continue;}

      std::vector<double> values;
      char* pch = strtok (str," ,");
      while (pch != NULL){
        values.push_back(atof(pch));
        pch = strtok (NULL," ,");
      }
      if(0==values.size()){continue;}
      float sciFiTimeSinceLastTrigger=0;
      if(newTriggerFlagged){
				nTriggers++;
				std::cout<<"New scifi trigger"<<std::endl; //dhynds
        newTriggerFlagged=false;
        long int thisSciFiTime=long(values[2]);
        long int prevSciFiTime=thisSciFiTime;
        if(sciFiTriggers){
	        if(sciFiTriggers->size()){
  	        prevSciFiTime=sciFiTriggers->back().time();
    	    }
        }
        sciFiTimeSinceLastTrigger=thisSciFiTime-prevSciFiTime;
				std::cout<<"scifi time since last trigger "<<sciFiTimeSinceLastTrigger/1000000<<std::endl; //dhynds
				if(sciFiTimeSinceLastTrigger/1e6 > 30){
					std::cout<<"Huge time difference between scifi triggers"<<std::endl;
				}
        bool newFrameFlagged=false;
        if(sciFiTimeSinceLastTrigger/1000>10000/*us*/){
					std::cout<<" New frame flagged. Time between scifi triggers was "<<sciFiTimeSinceLastTrigger/1e6<<" and "<<nTriggers<<" triggers were seen"<<std::endl;
					nTriggers=0;
					newFrameFlagged=true; //dhynds changed 15000 to 10000
				}
        if(0==sciFiFrames.size()) newFrameFlagged=true;
        if(newFrameFlagged){
          sciFiFrames.push_back(SciFiTriggers());
        }
        sciFiTriggers=&sciFiFrames.back();
        sciFiTriggers->push_back(SciFiTrigger(thisSciFiTime,sciFiTimeSinceLastTrigger));
      }
      int chan=int(values[0]);
      double adc=values[1];
      if(adc>1){
        sciFiTriggers->back().addhit(chan,adc);
      }
      if(!isZS&&127==chan){newTriggerFlagged=true;continue;}
    }

    // LOOP OVER TDC, MATCHING TO SCIFI DATA
    bool debug=false;

    SciFiFrames::iterator isff=sciFiFrames.begin();
		std::cout<<"About to match scifi and tdc. Number of tdc frames is "<<file->frames()->size()<<" and number of scifi frames is "<<sciFiFrames.size()<<std::endl;
		int nUnmatchedFrames=0;
		TDCFrames::iterator ifr_saved;
  
		//dhynds switched round iteration order - loop through scifi frames and look for matching tdc frames (since scifi drops frames!). Seems to be quite a big problem with
		//dropped frames and/or triggers and data being mixed up between spills. Now, for each scifi frame, loop through all tdc files and then frames until a match is found
		//(>80% matching required)
		
		for(;isff!=sciFiFrames.end();isff++){
			
			if((*isff).size() < 10)continue;
			
			bool frameFound=false;
			double nMatchedSave=0.;
			
			//loop over all files
			for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
				if(frameFound)continue;
				TDCFile* file=(*ifi);
				//loop over all frames
				for(TDCFrames::iterator ifr=file->frames()->begin();ifr!=file->frames()->end();ifr++){
					float sciFiTimeSinceLastTrigger=0;
					float tdcTimeSinceLastTrigger=0;
					float prevTDCTime=0;
					bool matched=false;

					TDCFrame* frame=(*ifr);
					SciFiTriggers::iterator isft=(*isff).begin();
					TDCTriggers* triggers = frame->triggers();
					double nMatched=0.;
					//check if times between triggers in tdc match time between triggers in scifi files
					for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
						float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
						matched=false;
						if(triggers->begin()==itr){
							matched=true;
						}else{
							float gate=5000;//ns 
							tdcTimeSinceLastTrigger=thisTDCTime-prevTDCTime;
							sciFiTimeSinceLastTrigger=(*isft).timeSince();
//						std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger/1e6<<" and for scifi "<<sciFiTimeSinceLastTrigger/1e6<<std::endl; //dhynds
							if(fabs(tdcTimeSinceLastTrigger-sciFiTimeSinceLastTrigger)<gate){
								matched=true;
							}
							else{
//							std::cout<<"not matched"<<std::endl;
							}
						}
						if(matched){isft++; prevTDCTime=thisTDCTime;nMatched++;}
						if((*isff).end()==isft) break;
					}
					//require at least 80% of trigger times to match. The number of triggers must be > 10 in each frame, to remove "noise" (1-hit frames etc).
					if(nMatched >= 0.8*((*isff).size())){
						frameFound=true;
						std::cout<<"scifi frames size is "<<(*isff).size()<<" and there were "<<nMatched<<" matched triggers, or "<<100*nMatched/(*isff).size()<<" % "<<std::endl;
						
						//now that the right frame is found, loop over triggers and insert scifi data
						
						frame->detectorId(std::string("SciFi"));
						frame->sourceName(std::string("TDC"));
						sciFiTimeSinceLastTrigger=0;
						tdcTimeSinceLastTrigger=0;
						prevTDCTime=0;
						isft=(*isff).begin();
						
						for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
							float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();
							matched=false;
							if(triggers->begin()==itr){
								matched=true;
							}else{
								float gate=5000;//ns 
								tdcTimeSinceLastTrigger=thisTDCTime-prevTDCTime;
								sciFiTimeSinceLastTrigger=(*isft).timeSince();
								if(fabs(tdcTimeSinceLastTrigger-sciFiTimeSinceLastTrigger)<gate){
									matched=true;
								}
							}
							if(matched){
								std::cout<<"Matched and pushing back scifi data"<<std::endl;
								for(int ic=0;ic<(*isft).nhits();ic++){
									(*itr)->push_back((*isft).channel(ic),(*isft).adc(ic),(*isft).time());
								}
								isft++; 
								prevTDCTime=thisTDCTime;
							}
							if((*isff).end()==isft) break;
						}
					
						break;
					}
				}
			}

			if(!frameFound){std::cout<<"Could not match with tdc. There were "<<(*isff).size()<<" triggers"<<std::endl;}
			
		}
			
	/*		
			
			float sciFiTimeSinceLastTrigger=0;
      float tdcTimeSinceLastTrigger=0;
      float prevTDCTime=0;			std::cout<<"TDC frame number "<<(ifr-file->frames()->begin())<<std::endl;
			TDCFrame* frame=(*ifr);
			frame->detectorId(std::string("SciFi"));
      frame->sourceName(std::string("TDC"));
			SciFiTriggers::iterator isft=(*isff).begin();
      TDCTriggers* triggers = frame->triggers();
			bool matched=false;
			bool somethingMatched=false;
			for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
        float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();//dhynds added -(*itr)->syncDelay()
				matched=false;
        
        bool oosync=false;
        if(triggers->begin()==itr){
          matched=true;
        }else{
          float gate=5000;//ns  //dhynds changed to 60us from 5us
          tdcTimeSinceLastTrigger=thisTDCTime-prevTDCTime;
          sciFiTimeSinceLastTrigger=(*isft).timeSince();
					std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger/1e6<<" and for scifi "<<sciFiTimeSinceLastTrigger/1e6<<std::endl; //dhynds
          if(fabs(tdcTimeSinceLastTrigger-sciFiTimeSinceLastTrigger)<gate){
            matched=true;
						somethingMatched=true;
          }
					else{
						std::cout<<"not matched"<<std::endl;
					}
					if( (not matched) and (tdcTimeSinceLastTrigger>sciFiTimeSinceLastTrigger) ) oosync=true;
        }
				
				
				if(matched && triggers->begin()!=itr){ //temp - get rid of begin trigger
        	for(int ic=0;ic<(*isft).nhits();ic++){
          	(*itr)->push_back((*isft).channel(ic),(*isft).adc(ic),(*isft).time());
        	}
          prevTDCTime=thisTDCTime;
          isft++; if((*isff).end()==isft) break;
        }
      }
   
			if(!somethingMatched){
				std::cout<<"Frame missing from scifi data"<<std::endl;
				if(nUnmatchedFrames == 0){
					std::cout<<"saving frame number"<<std::endl;
					ifr_saved = ifr;
				}
				nUnmatchedFrames++;
				if(nUnmatchedFrames > 100){
					std::cout<<"Have tried to match this scifi frame but have missed too many tdc frames. Skipping this scifi entry and returning to save point"<<std::endl;
					nUnmatchedFrames=0;
					ifr=ifr_saved--;
					isff++;
				}
				if(isff == sciFiFrames.begin()){
					std::cout<<"Ignore first scifi frame"<<std::endl;
				}
				else{
					isff--;
				}
			}
			else{
				std::cout<<"Frame MATCHED"<<std::endl;
			}
			ifr++;
		}
*/		
		
/*		for(TDCFrames::iterator ifr=file->frames()->begin();ifr!=file->frames()->end();ifr++){
      if(debug) std::cout<<"\n\n TFCFrame #"<<(ifr-file->frames()->begin())<<std::endl;
			std::cout<<"TDC frame number "<<(ifr-file->frames()->begin())<<std::endl;
      TDCFrame* frame=(*ifr);
      frame->detectorId(std::string("SciFi"));
      frame->sourceName(std::string("TDC"));
      float sciFiTimeSinceLastTrigger=0;
      float tdcTimeSinceLastTrigger=0;
      float prevTDCTime=0;
			SciFiTriggers::iterator isft=(*isff).begin();
      TDCTriggers* triggers = frame->triggers();
			bool matched=false;
			bool somethingMatched=false;
			std::cout<<"Number of tdc triggers in frame "<<triggers->size()<<" with "<<(*isff).size()<<" scifi triggers"<<std::endl; //dhynds
      for(TDCTriggers::iterator itr=triggers->begin();itr!=triggers->end();itr++){
        float thisTDCTime=(*itr)->timeAfterShutterOpen()-(*itr)->syncDelay();//dhynds added -(*itr)->syncDelay()
				matched=false;
        
        bool oosync=false;
        if(triggers->begin()==itr){
          matched=true;
        }else{
          float gate=5000;//ns  //dhynds changed to 60us from 5us
          tdcTimeSinceLastTrigger=thisTDCTime-prevTDCTime;
          sciFiTimeSinceLastTrigger=(*isft).timeSince();
					std::cout<<"Time since last tdc trigger "<<tdcTimeSinceLastTrigger/1e6<<" and for scifi "<<sciFiTimeSinceLastTrigger/1e6<<std::endl; //dhynds
          if(fabs(tdcTimeSinceLastTrigger-sciFiTimeSinceLastTrigger)<gate){
            matched=true;
						somethingMatched=true;
          }
					else{
						std::cout<<"not matched"<<std::endl;
					}
          //matched=true;//FORCE MATCHED FLAG
          if( (not matched) and (tdcTimeSinceLastTrigger>sciFiTimeSinceLastTrigger) ) oosync=true;
        }
        
        if(debug)
          std::cout<<"  TFCFrame #"<<(ifr-file->frames()->begin())<<"/"<<file->frames()->size()
                  <<" TFCTrigger #"<<(itr-triggers->begin())+1<<"/"<<triggers->size()
                  <<" at +"<<std::setprecision(7)<<tdcTimeSinceLastTrigger/1000<<"us "
                  <<" SciFiFrame #"<<isff-sciFiFrames.begin()<<"/"<<sciFiFrames.size()
                  <<" SciFiTrigger #"<<isft-(*isff).begin()<<"/"<<(*isff).size()
                  <<" has "<<(*isft).nhits()<<"hits at "<<std::setprecision(7)<<(*isft).timeSince()/1000
                  <<"us"<<(matched?" MATCHED":(oosync?" FAR OUT OF SYNC":""))<<std::endl;

        if(matched){
        	for(int ic=0;ic<(*isft).nhits();ic++){
          	(*itr)->push_back((*isft).channel(ic),(*isft).adc(ic),(*isft).time());
        	}
          prevTDCTime=thisTDCTime;
          isft++; if((*isff).end()==isft) break;
        }
      }
      if(!somethingMatched){
				std::cout<<"Frame missing from scifi data"<<std::endl;
				if(nUnmatchedFrames == 0){
					std::cout<<"saving frame number"<<std::endl;
					ifr_saved = ifr;
				}
				nUnmatchedFrames++;
				if(nUnmatchedFrames > 6){
					std::cout<<"Have counted "<<nUnmatchedFrames<<" unmatched frames. Going back to save point and discarding this frame"<<std::endl;
					nUnmatchedFrames=0;
					ifr=ifr_saved;
					isff++;
				}
			}
			if(somethingMatched)isff++; if(sciFiFrames.end()==isff) break; //dhynds added if(matched)
      //char dummy; std::cin>>dummy;
    }
*/    
    for(SciFiFrames::iterator isff=sciFiFrames.begin();isff!=sciFiFrames.end();isff++){
    	for(SciFiTriggers::iterator isft=(*isff).begin();isft!=(*isff).end();isft++){(*isft).clear();}
      (*isff).clear();
    }sciFiFrames.clear();
  }
  if(parameters->verbose) std::cout<<"\n"<<std::endl;
  return matchTDCandTelescope();
}




int Amalgamation::matchTDCandTelescope()
{
  bool debug=true;
  
  //build a quick vector of the first and last relaxd rough-time for each fram
  int prevTime=0;
  int currentTime=0;
  bool firstpass=true;
  std::vector<int> frameEnds;
  std::vector<int> frameBegins;
  int checkFrames=0;
  for(Index::iterator iit=index.begin();iit!=index.end();iit++){
    if((*iit).detector()!=summary->detectorId(0)) continue;
    currentTime=(*iit).roughtime();
    if(currentTime-prevTime>10){
      checkFrames++;
      frameBegins.push_back(currentTime);
      if(!firstpass) frameEnds.push_back(prevTime);
    }
    prevTime=currentTime;
    firstpass=false;
  }
  std::cout<<"Looping over timepix frames gave "<<checkFrames<<std::endl;
  frameEnds.push_back(currentTime);
  
  //loop over TDC spills (and then TDCFrames) and match in order with telescope frames in that time interval
  unsigned long int origIndexSize=index.size();
  index.reserve(2*origIndexSize);
  Index::iterator iit=index.begin();
  for(TDCFiles::iterator ifi=tdc.files()->begin();ifi!=tdc.files()->end();ifi++){
    int ifile=ifi-tdc.files()->begin();
    TDCFile* spill=(*ifi);
    std::vector<int>* zeroFrames=(*ifi)->zeroFrames();
    std::cout<<"In tdc match to telescope there are "<<zeroFrames->size()<<" zero frames in this spill"<<std::endl;
    int tdctime=spill->timeStamp();

    std::vector<int>::iterator ib=frameBegins.begin();
    std::vector<int>::iterator ie=frameEnds.begin();
    int closestBeginDistance=99999;
    int closestEndDistance=99999;
    while(ib!=frameBegins.end()){
      int beginDistance = (*ib)-tdctime;
      int endDistance = (*ie)-tdctime;
      if(abs(beginDistance)<abs(closestBeginDistance)){
        closestBeginDistance=beginDistance;
        closestEndDistance=endDistance;
      }
      if(debug)std::cout<<"TDC= "<<de(tdctime)<<" TPX= "<<de((*ib))<<" ("<<closestBeginDistance<<") "<<"  "<<de((*ie))<<" ("<<closestEndDistance<<")  "<<(closestBeginDistance==beginDistance?"closer...":"...further")<<std::endl;
      ib++;ie++;
    }
    if(parameters->useMedipix||parameters->useRelaxd){
	    if(closestBeginDistance>30||closestBeginDistance<-30){
  	    std::cout<<"...Rejecting TDC frame at "<<de(tdctime)<<" as it starts more than 30 seconds away from any telescope frame"<<std::endl;
    	  continue;
    	}
    }
    int  lowerbound=closestBeginDistance-1;
    int  higherbound=closestEndDistance+1;
    // Finally, loop through the [ordered] timepix frames and identify those
    // that fall between the dut[spill]time and the tdc[spill]time +/- a margin.
    // The TDCFrames are then matched in order in the spill[spill] and so inherit
    // the GPS time-stamp of the zeroeth [arbitrary choice] timepix chip

    Index::iterator extra_iit=iit;

    TDCFrames* frames=(*ifi)->frames();
    int frameNum=-1;
    for(TDCFrames::iterator ifr=frames->begin();ifr!=frames->end();ifr++){
      frameNum++;
//      bool skipFrame=false;
      int iframe=(ifr)-frames->begin();
//      std::cout<<"Checking for zero frame.."<<std::endl;
//      for(std::vector<int>::iterator i=zeroFrames->begin();i!=zeroFrames->end();i++){
//        std::cout<<"iframe is "<<iframe<<", and iterator is "<<(*i)<<std::endl;
//        if(iframe==(*i)){
//          std::cout<<"Reached zero frame"<<std::endl;
//          skipFrame=true;
//        }
//      }
//      if(skipFrame){
//        std::cout<<"Skipping zero frame"<<std::endl;
//        skipFrame=false;
//        continue;
//      }
//      std::cout<<"Still going"<<std::endl;
      bool writeToIndex=false;
      TDCFrame* frame=(*ifr);
      if(frame->sourceName()==std::string("NULL")){ frame->sourceName(std::string("TDC")); }
      if(frame->detectorId()==std::string("NULL")){ frame->detectorId(std::string("TDC")); }
      bool timepixTDCmatch=false;
      int r=0;
      int indexIters=0;
      if(parameters->useMedipix||parameters->useRelaxd){
        std::cout<<"Frame number is "<<frameNum<<std::endl;
        for(;iit!=index.end();){
          if((*iit).detector()==summary->detectorId(0)){
            r=(*iit).roughtime();
            if(r>tdctime+higherbound){if(debug)std::cout<<"------------END OF TDC BOUND ------------"<<std::endl;break;}
            if(r>=tdctime+lowerbound){
              frame->timeStamp((*iit).time());
              timepixTDCmatch=true;
            }
            if(debug)std::cout<<"T(tdc)="<<de(tdctime)<<" so:  "<<de(tdctime+lowerbound)<<" <   "<<de(r)<<"   < "<<de(tdctime+higherbound)
                              <<" index="<<(iit-index.begin())<<"/"<<index.size()<<"(orig:"<<origIndexSize<<") in frame="<<iframe<<(timepixTDCmatch?" MATCH!":"")<<std::endl;
          }
          ++iit;
          indexIters++;
          if(timepixTDCmatch) break;
        }
        if(timepixTDCmatch) writeToIndex=true;
      }else{
        writeToIndex=true;
      }
      std::cout<<"iter number is "<<indexIters<<std::endl;
      if(indexIters>10){
        std::cout<<"Ignoring merged frame"<<std::endl;
        writeToIndex=true;
      }
      if(writeToIndex){
        std::stringstream s; s<<ifile;
        index.push_back(IndexEntry(r,frame->timeStamp(),frame->sourceName(),frame->detectorId(),s.str(),iframe));
        if(parameters->verbose){
          if(!debug) std::cout<<"\r...Indexing TDC spill "<<ifile+1<<"/"<<tdc.files()->size()<<" frame "<<iframe<<" at "<<de(tdctime)<<" <--> "<<de(r)<<" (relaxd) "<<std::flush;
        }
      }
    }
    //Loop through the Timepix data and monitor if there is a
    // difference in the number of frames in the TDC and Timepix data
    if(parameters->useMedipix||parameters->useRelaxd){
      std::map<std::string,int> nTimepixFramesInSpill;
      for(int idec=0;idec<summary->nDetectors();idec++){
        nTimepixFramesInSpill.insert(std::make_pair(summary->detectorId(idec),0));
      }
      for(;extra_iit<index.end();extra_iit++){
        IndexEntry entry=(*extra_iit);
        if( entry.roughtime()>(tdctime+higherbound) ) break;
        if( entry.roughtime()<(tdctime+ lowerbound) ) continue;
        nTimepixFramesInSpill[entry.detector()]++;
      }
      int nAdditionalTimepix=0;
      for(int idec=0;idec<summary->nDetectors();idec++){
        int diff=(nTimepixFramesInSpill[summary->detectorId(idec)])-frames->size()-zeroFrames->size();//dhynds added zeroFrames->size()
        summary->tdcTimepixDiff(summary->detectorId(idec),diff);
        nAdditionalTimepix+=bool(diff);
      }
      if(parameters->verbose && nAdditionalTimepix){
        std::cout<<"WARNING: "<<nAdditionalTimepix<<" timepix chips see a different number of frames (";
        for(int idec=0;idec<summary->nDetectors();idec++){std::cout<<nTimepixFramesInSpill[summary->detectorId(idec)]<<(idec+1==summary->nDetectors()?"":",");}
        std::cout<<") to the TDC's "<<frames->size()<<" frames"<<std::endl;
      }
    }
    if(debug) std::cout<<"\n   --------------| NEXT SPILL! |---------------- "; std::cout<<std::endl;
  }
  if(parameters->useMedipix||parameters->useRelaxd){
	  for(Index::iterator iit=index.begin();iit!=index.end();iit++){
  	  if((*iit).detector()!=summary->detectorId(0)) continue;
    	int ibin = tdc.hMedipixTime->FindBin((*iit).roughtime()-firstTime);
    	tdc.hMedipixTime->SetBinContent(ibin,2);
  	}
  }
  return 0;
}





int Amalgamation::buildEventFile()
{
  if(parameters->verbose){std::cout<<"\n\nWill use '"<<parameters->eventFile<<"' as a destination file [Amalgamation::buildEventFile()]"<<std::endl;}
  // Open the new event file and register the event branch
  TFile tbfile(parameters->eventFile.c_str(),"RECREATE");
  TTree tbtree("tbtree","A tree of TestBeamEvents");
  TestBeamEvent *event=new TestBeamEvent();
  tbtree.Branch("event_branch",&event,16000,0);
  
  bool debug=false;
    int file_num=0;
  // Loop over index, reading data and zero-suppressing before adding to event file
  char str[2000];
  int evtCounter=1;
  float latestTDC=0;
  float latestTOA=0;
  Index::iterator iter=index.begin();
  double currentTime = (*iter).time();
  
  for(;iter<index.end();iter++){
    unsigned int pos=iter-index.begin();
    if(((pos+1)/1. - int((pos+1)/1.)==0)||(pos>index.size()-2)){
      std::cout <<"\rAmalgamating: "<<iter-index.begin()+1<<"/"<<index.size()<<" items"<< std::flush;
    }
    
    if((*iter).source()==std::string("TDC")){
      int ifile  = atoi(std::string((*iter).file()).c_str());
      int iframe = (*iter).entry();
      TDCFile* file = tdc.files()->at(ifile);
      TDCFrame* frame = file->frames()->at(iframe);
      event->push_back(frame);
      
      if(debug){ std::cout.precision(15);
  	    std::cout<<". TDC spill "<<ifile<<", frame "<<iframe<<" at "<<de((*iter).roughtime())<<" ("<<(*iter).time()<<")   \t "
    	  <<"with "<<frame->nTriggersInFrame()<<" triggers in "<<frame->timeShutterIsOpen()/1000.
      	<<"us.\t Spill has: "<<file->nFramesInFile()<<" frames & "<<file->nTriggersInFile()<<" triggers "<<std::endl;
      	latestTDC=frame->timeShutterIsOpen()/1000.;
      }
    }else if((*iter).source()==std::string("timepix")||(*iter).source()==std::string("medipix")){
      std::fstream file_op((*iter).file().c_str(),std::ios::in);
      RowColumnEntries data;
      if(compressedRawFormat){ //2010-11 running
        int frameNumInDscFile=(*iter).entry();
        int frameNumInDataFile=0;
        std::string token;
	      float largestValue=0;
        while(!file_op.eof()) {
          file_op.getline(str,2000);
          if(0==std::string(str).find("#")){
            frameNumInDataFile++;
            continue;
          }
          if(frameNumInDataFile==frameNumInDscFile){
            std::stringstream in ( str );
            int row=-1;
            int column=-1;
            int value=-1;
            int field=0;
            while ( std::getline ( in, token, '\t' ) ){
              if(0==field) column=atoi(token.c_str());
              if(1==field) row=atoi(token.c_str());
              if(2==field) value=atoi(token.c_str());
              ++field;
            }
            if(3==field){
            	if(value>largestValue) largestValue=value;
              //if(value <0) std::cout << "Warning: data entry is unexpectedly negative: "<< value << std::endl;
//							if(value!=0) data.push_back(new RowColumnEntry(float(row),float(column),float(value))); //ZERO SUPPRESSION
							data.push_back(new RowColumnEntry(float(row),float(column),float(value))); //ZERO SUPPRESSION dhynds removed cut on value
            }else if(field!=0){
              std::cout<<std::endl<<"  Strange: "<<field<<" fields in frame "<<frameNumInDataFile<<" of "<<(*iter).file()<<" = '"<<str<<"'"<<std::endl;
            }
          }else if(frameNumInDataFile>frameNumInDscFile){
            break;
          }
        }
        file_op.close();
        if(debug){
	        if((*iter).detector()==std::string("D04-W0015")){ 
  	      	std::cout<<". Relaxd: "<<(*iter).detector()<<"           "<<de((*iter).roughtime())<<" ("<<(*iter).time()<<")                           "<<((*iter).detector()==std::string("D04-W0015")?Form("%3.1fus",largestValue*0.025):"")<<std::endl;
						latestTOA=largestValue*0.025;
      	  }
      	}
      }else{ // 2009 running without the compressed file format
        
        int row=0;
        while(!file_op.eof()) {
          if(row>256){
            std::cout<<"  Strangness in "<<(*iter).file()<<", counting more than 256 rows. Ignoring them."<<std::endl;
            break;
          }
          int column=0;
          std::string token;
          file_op.getline(str,2000);
          std::stringstream in ( str );
          while ( std::getline ( in, token, ' ' ) ){
            if(column>256){
              std::cout<<"  Strangness in "<<(*iter).file()<<", counting more than 256 columns. Row "<<row<<std::endl;
              break;
            }
            int value=atoi(token.c_str());
            if(value <0) std::cout << "Warning: data entry is unexpectedly negative: "<< value << std::endl;
            if(value!=0) data.push_back(new RowColumnEntry(float(row),float(column),float(value))); //ZERO SUPPRESSION
            column++;
          }
          row++;
        }
      }
      file_op.close();
      /* if(data.size()) */ event->push_back((*iter).time(),(*iter).source(),(*iter).detector(),data);
    }else{
      std::cout << " ... but don't know what to do with source type '"<< (*iter).source() <<"'"<< std::endl;
      continue;
    }
    double nextTime=(*(iter+1)).time();
      file_num++;
//		std::cout<<"detector ID "<<(*iter).detector()<<" at time "<<nextTime-1278344000<<std::endl; //dhynds
    if( nextTime-currentTime > parameters->eventWindow ){
//    if( file_num == 7 ){ //dhynds replaced previous line with this
      if(debug){
	      if(fabs(latestTOA-latestTDC)>1) std::cout<<"\n\n\n\n\n\n\n\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n\n\n\n\n\n\n"<<std::endl;
  	    std::cout << std::endl;
      }
//        std::cout<<std::endl<<"making event with "<<file_num<<" files"<<std::endl; //dhynds
      summary->eventSummary(event,currentTime);
      currentTime=nextTime;
      tbtree.Fill();
      event->clear();
      evtCounter++;
        file_num=0;
      if(evtCounter>parameters->numberOfEvents && parameters->numberOfEvents>0) break;
    }
  }
  
  if(parameters->verbose) std::cout <<"\n"<< std::endl;
  tbtree.GetUserInfo()->Add(summary);
  tbfile.Write();
  tbfile.Close();
  if(parameters->verbose) std::cout <<"Event file complete and closed.\n"<< std::endl;
  std::cout<<std::endl;
  return 0;
}

