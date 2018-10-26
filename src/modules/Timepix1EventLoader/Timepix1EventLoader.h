#ifndef Timepix1EventLoader_H
#define Timepix1EventLoader_H 1

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Timepix1EventLoader : public Module {

    public:
        // Constructors and destructors
        Timepix1EventLoader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Timepix1EventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        static bool sortByTime(std::string filename1, std::string filename2);
        void processHeader(std::string, std::string&, long long int&);

        // Member variables
        int m_eventNumber;
        std::string m_inputDirectory;
        std::vector<std::string> m_inputFilenames;
        bool m_fileOpen;
        size_t m_fileNumber;
        long long int m_eventTime;
        std::ifstream m_currentFile;
        std::string m_currentDevice;
        std::string m_prevHeader;
    };
} // namespace corryvreckan
#endif // Timepix1EventLoader_H
