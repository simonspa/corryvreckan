#ifndef ANALYSIS_H
#define ANALYSIS_H 1

#include <fstream>
#include <map>
#include <vector>

#include "TBrowser.h"
#include "TDirectory.h"
#include "TFile.h"

#include "Clipboard.h"
#include "DetectorParameters.h"
#include "Parameters.h"
#include "algorithm/Algorithm.h"
#include "config/ConfigManager.hpp"
#include "config/Configuration.hpp"

//-------------------------------------------------------------------------------
// The analysis class is the core class which allows the event processing to
// run. It basically contains a vector of algorithms, each of which is
// initialised,
// run on each event and finalised. It does not define what an event is, merely
// runs each algorithm sequentially and passes the clipboard between them
// (erasing
// it at the end of each run sequence). When an algorithm returns a 0, the event
// processing will stop.
//-------------------------------------------------------------------------------

namespace corryvreckan {
    class Analysis {

    public:
        // Constructors and destructors
        Analysis(std::string config_file_name);
        virtual ~Analysis(){};

        // Member functions
        void load();

        void run();
        void timing();
        void initialiseAll();
        void finaliseAll();

        void terminate();
        void reset(){};

        TBrowser* browser;

    protected:
        // Member variables
        Parameters* m_parameters;
        Clipboard* m_clipboard;
        Configuration global_config;
        std::vector<Detector*> detectors;

    private:
        void load_detectors();
        void load_algorithms();

        // Log file if specified
        std::ofstream log_file_;

        TFile* m_histogramFile;
        TDirectory* m_directory;
        int m_events;

        std::vector<Algorithm*> m_algorithms;
        std::map<std::string, void*> loaded_libraries_;

        std::atomic<bool> m_terminate;
        std::unique_ptr<corryvreckan::ConfigManager> conf_mgr_;
        std::unique_ptr<corryvreckan::ConfigManager> det_mgr_;

        Algorithm* create_algorithm(void* library, corryvreckan::Configuration config);
        std::tuple<LogLevel, LogFormat> set_algorithm_before(const std::string&, const Configuration& config);
        void set_algorithm_after(std::tuple<LogLevel, LogFormat> prev);
    };
}
#endif // ANALYSIS_H
