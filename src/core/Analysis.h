#ifndef ANALYSIS_H
#define ANALYSIS_H 1

#include <map>
#include <vector>

#include "TBrowser.h"
#include "TDirectory.h"
#include "TFile.h"

#include "Algorithm.h"
#include "Clipboard.h"
#include "Parameters.h"
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
        void add(Algorithm*);
        void load();

        void run();
        void timing();
        void initialiseAll();
        void finaliseAll();

        void terminate();
        void reset(){};

        TBrowser* browser;

    protected:
        Algorithm* create_algorithm(void* library, corryvreckan::Configuration config, Clipboard* clipboard);

        // Member variables
        Parameters* m_parameters;
        Clipboard* m_clipboard;
        Configuration m_config;
        vector<Algorithm*> m_algorithms;
        TFile* m_histogramFile;
        TDirectory* m_directory;
        std::map<std::string, void*> loaded_libraries_;
        int m_events;

    private:
        std::atomic<bool> m_terminate;
        std::unique_ptr<corryvreckan::ConfigManager> conf_mgr_;
    };
}
#endif // ANALYSIS_H
