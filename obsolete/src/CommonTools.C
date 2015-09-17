\
// Include files
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "TColor.h"
#include "TStyle.h"
#include "TSystem.h"

#include "CommonTools.h"

bool CommonTools::exists(std::string file)
{
  FileStat_t x;
  return (not gSystem->GetPathInfo(file.c_str(),x));
}

bool CommonTools::isADirectory(std::string dir)
{
  return (bool)opendir(dir.c_str());
}

void CommonTools::suppressNoisyPixels(TH2 *map,int nmax)
{
  for(int count=0;count<nmax;count++){
    int i,j,k;
    map->GetBinXYZ(map->GetMaximumBin(),i,j,k);
    int thisBinContent = (int)map->GetBinContent(i,j);
    if(((0==map->GetBinContent(i+1,j  )) ||
        (0==map->GetBinContent(i+1,j+1)) ||
        (0==map->GetBinContent(i  ,j+1)) ||
        (0==map->GetBinContent(i-1,j+1)) ||
        (0==map->GetBinContent(i-1,j  )) ||
        (0==map->GetBinContent(i-1,j-1)) ||
        (0==map->GetBinContent(i-1,j  )) ||
        (0==map->GetBinContent(i-1,j+1)))&&
       (thisBinContent>10)){
      map->SetBinContent(i,j,0);
      map->SetEntries(map->GetEntries()-thisBinContent-1);
    }else{
      count=nmax;
    }
  }
}

void CommonTools::defineColorMap()
{
  int nColors=51;
  Double_t s[5] = {0.00, 0.25, 0.50, 0.75, 1.00};
  Double_t r[5] = {0.99, 0.00, 0.87, 1.00, 0.70};
  Double_t g[5] = {0.99, 0.81, 1.00, 0.20, 0.00};
  Double_t b[5] = {0.99, 1.00, 0.12, 0.00, 0.00};
  TColor::CreateGradientColorTable(5, s, r, g, b, nColors);
  gStyle->SetNumberContours(nColors);
}

int CommonTools::getdir (std::string dir, std::vector<std::string> &files)
{
  DIR *dp;
  struct stat st;
  struct dirent *dirp;

  if((dp  = opendir(dir.c_str())) == NULL) {
    std::cout << "Error(" << errno << ") opening " << dir << std::endl;
    return errno;
  }
  while ((dirp = readdir(dp)) != NULL) {
    std::string fullpath=dir+std::string("/")+std::string(dirp->d_name);
    stat (fullpath.c_str(), &st);
    time_t modified_last = st.st_ctime;
    std::string modTime(asctime(localtime ( &modified_last ) ) );
    std::string file(dirp->d_name);
    files.push_back(file);
  }
  closedir(dp);
  return 0;
}

void CommonTools::rtrim(char* string)
{
    char* original = string + strlen(string);
    while(*--original == ' ') ;
    *(original + 1) = '\0';
}
void CommonTools::ltrim(char *string)
{
    char* original = string;
    char *p = original;
    int trimmed = 0;
    do{
      if (*original != ' ' || trimmed){
        trimmed = 1;
        *p++ = *original;
      }
    }
    while (*original++ != '\0');
}
void CommonTools::trim(char *string)
{
  CommonTools::ltrim(string);
  CommonTools::rtrim(string);
}


std::string& CommonTools::stringtrim(std::string& str)
{
  std::string::size_type pos = str.find_last_not_of(' ');
  if(pos != std::string::npos) {
    str.erase(pos+1,str.size());
    pos = str.find_first_not_of(' ');
    if(pos != std::string::npos) str.erase(0, pos);
  }else{
  	str.erase(str.begin(), str.end());
  }
  return str;
}

int CommonTools::countLines(const char* name)
{
  std::string line;
  int number_of_lines = 0;
  std::ifstream myfile(name);  
  while (std::getline(myfile,line)){
    ++number_of_lines;
  }
  myfile.close();
  return number_of_lines;
}

int CommonTools::countLines(std::string s)
{
  return CommonTools::countLines(s.c_str());
}

int CommonTools::fileContains(const char* s,const char* name,int n)
{
  std::string line;
  int number_of_lines = 0;
  int number_of_occurencies = 0;
  std::ifstream myfile(name);
  while (std::getline(myfile,line)){
  	++number_of_lines;
  	if(line.find(s)!=std::string::npos)
    	++number_of_occurencies;
    if(number_of_lines==n) 
    	break;
  }
  myfile.close();
  return number_of_occurencies;
}


int CommonTools::fileContains(const char* s,std::string name,int n)
{
  return fileContains(s,name.c_str(),n);
}

