#ifndef FileWriter_H
#define FileWriter_H 1

#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "core/Algorithm.h"

namespace corryvreckan {
    class FileWriter : public Algorithm {

    public:
        // Constructors and destructors
        FileWriter(Configuration config, Clipboard* clipboard);
        ~FileWriter() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Member variables
        int m_eventNumber;
        string m_fileName;
        TFile* m_outputFile;

        // Flags for which data types to write out
        bool m_writeClusters;
        bool m_writePixels;
        bool m_writeTracks;
        bool m_onlyDUT;

        // Map of trees which holds the output objects
        map<string, TTree*> m_outputTrees;

        // Objects which the trees will point to (when
        // the branch address is set
        long long int m_time;
        map<string, TestBeamObject*> m_objects;

        // List of objects to write out
        vector<string> m_objectList;
    };
}
#endif // FileWriter_H
