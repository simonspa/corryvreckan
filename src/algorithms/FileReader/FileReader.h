#ifndef FileReader_H
#define FileReader_H 1

#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "core/algorithm/Algorithm.h"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class FileReader : public Algorithm {

    public:
        // Constructors and destructors
        FileReader(Configuration config, std::vector<Detector*> detectors);
        ~FileReader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int m_eventNumber;
        std::string m_fileName;
        TFile* m_inputFile;

        // Flags for which data types to write out
        bool m_readClusters;
        bool m_readPixels;
        bool m_readTracks;
        bool m_onlyDUT;
        bool m_readMCParticles;

        // Map of trees which holds the output objects
        std::map<std::string, TTree*> m_inputTrees;

        // Objects which the trees will point to (when
        // the branch address is set
        long long int m_time;
        std::map<std::string, TestBeamObject*> m_objects;

        // List of objects to write out
        std::vector<std::string> m_objectList;

        // Variables to keep track of time and file reading
        long long int m_currentTime;
        std::map<std::string, long long int> m_currentPosition;
        double m_timeWindow;
    };
} // namespace corryvreckan
#endif // FileReader_H
