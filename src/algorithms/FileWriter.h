#ifndef FileWriter_H
#define FileWriter_H 1

#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "core/algorithm/Algorithm.h"

namespace corryvreckan {
    class FileWriter : public Algorithm {

    public:
        // Constructors and destructors
        FileWriter(Configuration config, std::vector<Detector*> detectors);
        ~FileWriter() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int m_eventNumber;
        std::string m_fileName;
        TFile* m_outputFile;

        // Flags for which data types to write out
        bool m_writeClusters;
        bool m_writePixels;
        bool m_writeTracks;
        bool m_onlyDUT;

        // Map of trees which holds the output objects
        std::map<std::string, TTree*> m_outputTrees;

        // Objects which the trees will point to (when
        // the branch address is set
        long long int m_time;
        std::map<std::string, TestBeamObject*> m_objects;

        // List of objects to write out
        std::vector<std::string> m_objectList;
    };
}
#endif // FileWriter_H
