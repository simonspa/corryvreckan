// $Id: CommonTools.h,v 1.4 2009-07-13 15:11:11 mjohn Exp $
#ifndef COMMONTOOLS_H 
#define COMMONTOOLS_H 1

// Include files
#include <string>
#include "TH2.h"

/** @class CommonTools CommonTools.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-05
 */
namespace CommonTools
{
  bool exists(std::string);
  bool isADirectory(std::string);

  void suppressNoisyPixels(TH2 *map,int nmax);

  void defineColorMap();

  int getdir (std::string, std::vector<std::string> &);

  void ltrim(char*);
  void rtrim(char*);
  void  trim(char*);
  std::string& stringtrim(std::string&);
  
  int countLines(const char*);
  int countLines(std::string);
  
  int fileContains(const char*,const char*,int n=-1);
  int fileContains(const char*,std::string,int n=-1);
};
#endif // COMMONTOOLS_H
