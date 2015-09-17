#ifndef CLIPBOARD_H 
#define CLIPBOARD_H 1

// Include files
#include <map>
#include <string>
#include <iostream>

#include "TestBeamObject.h"
#include "TestBeamCluster.h"
#include "TestBeamChessboardCluster.h"
#include "VetraCluster.h"
#include "SciFiCluster.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamTrack.h"

/** @class Clipboard Clipboard.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

enum {COPIED, CREATED};

class Clipboard{
public:
  Clipboard() {}
  virtual ~Clipboard() {}
  void put(std::string, TestBeamObjects*, bool);
  TestBeamObjects* get(std::string);
  void clear();
private:
  std::map<std::string,TestBeamObjects*> contents;
  std::map<std::string,bool> isOriginalObject;
};

inline void Clipboard::put(std::string dataContainerLabel, TestBeamObjects* o, bool newObject)
{
  if (contents.find(dataContainerLabel) != contents.end()) {
    std::cout << "Clipboard" << std::endl;
    std::cout << "    WARNING: '"<<dataContainerLabel<<"' data already on the clipboard"<<std::endl;
  }
  contents.insert(make_pair(dataContainerLabel, o));
  isOriginalObject.insert(make_pair(dataContainerLabel, newObject));
}

inline void Clipboard::clear()
{
  std::map<std::string,TestBeamObjects*>::iterator iter = contents.begin();
  for (; iter!= contents.end(); ++iter) {
    std::string dataContainerLabel=(*iter).first;
    if( isOriginalObject[dataContainerLabel] ){
      if (dataContainerLabel == std::string("Clusters")) { 
        TestBeamClusters* items = (TestBeamClusters*)(*iter).second;
        TestBeamClusters::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("NonAssociatedClusters")) { 
        //TestBeamClusters* items = (TestBeamClusters*)(*iter).second;
        //TestBeamClusters::iterator it = items->begin();
        //for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("ChClusters")) { 
        TestBeamChessboardClusters* items = (TestBeamChessboardClusters*)(*iter).second;
        TestBeamChessboardClusters::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("ProtoTracks")) { 
        TestBeamProtoTracks* items = (TestBeamProtoTracks*)(*iter).second;
        TestBeamProtoTracks::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("Tracks")) { 
        TestBeamTracks* items = (TestBeamTracks*)(*iter).second;
        TestBeamTracks::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("VetraClusters")) { 
        VetraClusters* items = (VetraClusters*)(*iter).second;
        VetraClusters::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("FEI4Clusters")) { 
        FEI4Clusters* items = (FEI4Clusters*)(*iter).second;
        FEI4Clusters::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
      if (dataContainerLabel == std::string("SciFiClusters")) { 
        SciFiClusters* items = (SciFiClusters*)(*iter).second;
        SciFiClusters::iterator it = items->begin();
        for(; it!=items->end(); ++it) delete *it;
      }
    }
    (*iter).second->clear();
    delete (*iter).second;
  }
  contents.clear();
}
inline TestBeamObjects* Clipboard::get(std::string s)
{
  if (contents.find(s) == contents.end()) {
    return NULL;
  }
  return contents[s];
}

#endif // CLIPBOARD_H
