#ifndef CLIPBOARD_H
#define CLIPBOARD_H 1

// Include files
#include <iostream>
#include <map>
#include <string>

#include "Parameters.h"
#include "objects/TestBeamObject.h"
#include "utils/log.h"

//-------------------------------------------------------------------------------
// The Clipboard class is used to transfer information between algorithms during
// the event processing. Any object inheriting from TestBeamObject can be placed
// on the clipboard, and retrieved by its name. At the end of each event, the
// clipboard is wiped clean.
//-------------------------------------------------------------------------------

class Clipboard {

public:
    // Constructors and destructors
    Clipboard() {}
    virtual ~Clipboard() {}

    // Add objects to clipboard - with name or name + type
    void put(std::string name, TestBeamObjects* objects) {
        m_dataID.push_back(name);
        m_data[name] = objects;
    }
    void put(std::string name, std::string type, TestBeamObjects* objects) {
        m_dataID.push_back(name + type);
        m_data[name + type] = objects;
    }

    // Get objects from clipboard - with name or name + type
    TestBeamObjects* get(std::string name) {
        if(m_data.count(name) == 0)
            return NULL;
        return m_data[name];
    }
    TestBeamObjects* get(std::string name, std::string type) {
        if(m_data.count(name + type) == 0)
            return NULL;
        return m_data[name + type];
    }

    // Clear items on the clipboard
    void clear() {
        for(auto & id : m_dataID) {
            TestBeamObjects* collection = m_data[id];
            for(TestBeamObjects::iterator it = collection->begin(); it != collection->end(); it++)
                delete(*it);
            delete m_data[id];
            m_data.erase(id);
        }
        m_dataID.clear();
    }

    // Quick function to check what is currently held by the clipboard
    void checkCollections() {
        for(auto &name : m_dataID)
            LOG(DEBUG) << "Data held: " << name;
    }

private:
    // Container for data, list of all data held
    std::map<std::string, TestBeamObjects*> m_data;
    std::vector<std::string> m_dataID;
};

#endif // CLIPBOARD_H
