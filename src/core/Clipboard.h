#ifndef CLIPBOARD_H
#define CLIPBOARD_H 1

// Include files
#include <map>
#include <string>
#include <iostream>

#include "objects/TestBeamObject.h"
#include "Parameters.h"
#include "utils/log.h"

//-------------------------------------------------------------------------------
// The Clipboard class is used to transfer information between algorithms during
// the event processing. Any object inheriting from TestBeamObject can be placed
// on the clipboard, and retrieved by its name. At the end of each event, the
// clipboard is wiped clean.
//-------------------------------------------------------------------------------

class Clipboard{

public:

  // Constructors and destructors
  Clipboard(){}
  virtual ~Clipboard(){}

  // Add objects to clipboard - with name or name + type
  void put(string name, TestBeamObjects* objects){
    m_dataID.push_back(name);
    m_data[name] = objects;
  }
  void put(string name, string type, TestBeamObjects* objects){
    m_dataID.push_back(name+type);
    m_data[name+type] = objects;
  }

  // Get objects from clipboard - with name or name + type
  TestBeamObjects* get(string name){
    if(m_data.count(name) == 0) return NULL;
    return m_data[name];
  }
  TestBeamObjects* get(string name, string type){
    if(m_data.count(name+type) == 0) return NULL;
    return m_data[name+type];
  }

  // Clear items on the clipboard
  void clear(){
    int nCollections = m_dataID.size();
    for(int i=0;i<nCollections;i++){
      TestBeamObjects* collection = m_data[m_dataID[i]];
      int nElements = collection->size();
      for(TestBeamObjects::iterator it=collection->begin(); it!=collection->end(); it++) delete (*it);
      delete m_data[m_dataID[i]];
      m_data.erase(m_dataID[i]);
    }
    m_dataID.clear();
  }

  // Quick function to check what is currently held by the clipboard
  void checkCollections(){
    for(int name=0;name<m_dataID.size();name++) LOG(DEBUG) << "Data held: " << m_dataID[name];
  }

private:

  // Container for data, list of all data held
  map<string,TestBeamObjects*> m_data;
  vector<string> m_dataID;

};

#endif // CLIPBOARD_H
