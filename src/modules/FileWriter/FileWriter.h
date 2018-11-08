#ifndef FileWriter_H
#define FileWriter_H 1

#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class FileWriter : public Module {

    public:
        // Constructors and destructors
        FileWriter(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~FileWriter() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
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
        double m_time;
        std::map<std::string, Object*> m_objects;

        // List of objects to write out
        std::vector<std::string> m_objectList;
    };
} // namespace corryvreckan
#endif // FileWriter_H
