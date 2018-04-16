#ifndef CLIPBOARD_H
#define CLIPBOARD_H 1

// Include files
#include <iostream>
#include <map>
#include <string>

#include "core/utils/log.h"
#include "objects/TestBeamObject.h"

//-------------------------------------------------------------------------------
// The Clipboard class is used to transfer information between modules during
// the event processing. Any object inheriting from TestBeamObject can be placed
// on the clipboard, and retrieved by its name. At the end of each event, the
// clipboard is wiped clean.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    class Clipboard {

    public:
        // Constructors and destructors
        Clipboard() {}
        virtual ~Clipboard() {}

        // Add objects to clipboard - with name or name + type
        void put(std::string name, TestBeamObjects* objects);
        void put(std::string name, std::string type, TestBeamObjects* objects);
        void put_persistent(std::string name, double value);

        // Get objects from clipboard - with name or name + type
        TestBeamObjects* get(std::string name);
        TestBeamObjects* get(std::string name, std::string type);

        double get_persistent(std::string name);

        // Clear items on the clipboard
        void clear();

        // Quick function to check what is currently held by the clipboard
        void checkCollections();

    private:
        // Container for data, list of all data held
        std::map<std::string, TestBeamObjects*> m_data;
        std::vector<std::string> m_dataID;
        std::map<std::string, double> m_persistent_data;
    };
} // namespace corryvreckan

#endif // CLIPBOARD_H
