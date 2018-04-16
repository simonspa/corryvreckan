#ifndef MODULE_H
#define MODULE_H 1

// Include files
#include <string>
#include "TStopwatch.h"
#include "core/Clipboard.h"
#include "core/Detector.h"
#include "core/config/Configuration.hpp"
#include "exceptions.h"

//-------------------------------------------------------------------------------
// The module class is the base class that all user modules are built on.
// It
// allows the analysis class to hold modules of different types, without
// knowing
// what they are, and provides the functions initialise, run and finalise. It
// also
// gives some basic tools like the "tcout" replacement for "cout" (appending the
// module name) and the stopwatch for timing measurements.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    enum StatusCode {
        Success,
        NoData,
        Failure,
    };

    /** Base class for all modules
     *  @defgroup Modules Modules
     */
    class Module {

    public:
        // Constructors and destructors
        Module() {}
        Module(Configuration config, std::vector<Detector*> detectors) {
            m_name = config.getName();
            m_config = config;
            m_detectors = detectors;
            m_stopwatch = new TStopwatch();
            IFLOG(TRACE) {
                std::stringstream det;
                for(auto& d : m_detectors) {
                    det << d->name() << ", ";
                }
                LOG(TRACE) << "Module determined to run on detectors: " << det.str();
            }
        }
        virtual ~Module() {}

        // Three main functions - initialise, run and finalise. Called for every
        // module
        virtual void initialise() = 0;
        virtual StatusCode run(Clipboard*) = 0;
        virtual void finalise() = 0;

        // Methods to get member variables
        std::string getName() { return m_name; }
        Configuration getConfig() { return m_config; }
        TStopwatch* getStopwatch() { return m_stopwatch; }

    protected:
        // Member variables
        TStopwatch* m_stopwatch;
        std::string m_name;
        Configuration m_config;

        std::vector<Detector*> get_detectors() { return m_detectors; };
        std::size_t num_detectors() { return m_detectors.size(); };
        Detector* get_detector(std::string name) {
            auto it =
                find_if(m_detectors.begin(), m_detectors.end(), [&name](Detector* obj) { return obj->name() == name; });
            if(it == m_detectors.end()) {
                throw ModuleError("Device with detector ID " + name + " is not registered.");
            }

            return (*it);
        }
        bool has_detector(std::string name) {
            auto it =
                find_if(m_detectors.begin(), m_detectors.end(), [&name](Detector* obj) { return obj->name() == name; });
            if(it == m_detectors.end()) {
                return false;
            }
            return true;
        }

    private:
        std::vector<Detector*> m_detectors;
    };
} // namespace corryvreckan
#endif
