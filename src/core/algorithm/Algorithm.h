#ifndef ALGORITHM_H
#define ALGORITHM_H 1

// Include files
#include <string>
#include "TStopwatch.h"
#include "core/Clipboard.h"
#include "core/Parameters.h"
#include "core/config/Configuration.hpp"
#include "exceptions.h"

//-------------------------------------------------------------------------------
// The algorithm class is the base class that all user algorithms are built on.
// It
// allows the analysis class to hold algorithms of different types, without
// knowing
// what they are, and provides the functions initialise, run and finalise. It
// also
// gives some basic tools like the "tcout" replacement for "cout" (appending the
// algorithm name) and the stopwatch for timing measurements.
//-------------------------------------------------------------------------------

namespace corryvreckan {

    enum StatusCode {
        Success,
        NoData,
        Failure,
    };

    class Algorithm {

    public:
        // Constructors and destructors
        Algorithm() {}
        Algorithm(Configuration config, std::vector<Detector*> detectors) {
            m_name = config.getName();
            m_config = config;
            m_detectors = detectors;
            m_stopwatch = new TStopwatch();
            IFLOG(TRACE) {
                std::stringstream det;
                for(auto& d : m_detectors) {
                    det << d->name() << ", ";
                }
                LOG(TRACE) << "Algorithm determined to run on detectors: " << det.str();
            }
        }
        virtual ~Algorithm() {}

        // Three main functions - initialise, run and finalise. Called for every
        // algorithm
        virtual void initialise(Parameters*) = 0;
        virtual StatusCode run(Clipboard*) = 0;
        virtual void finalise() = 0;

        // Methods to get member variables
        std::string getName() { return m_name; }
        Configuration getConfig() { return m_config; }
        TStopwatch* getStopwatch() { return m_stopwatch; }

    protected:
        // Member variables
        Parameters* parameters;
        TStopwatch* m_stopwatch;
        std::string m_name;
        Configuration m_config;

        std::vector<Detector*> get_detectors() { return m_detectors; };
        Detector* get_detector(std::string name) {
            auto it =
                find_if(m_detectors.begin(), m_detectors.end(), [&name](Detector* obj) { return obj->name() == name; });
            if(it == m_detectors.end()) {
                throw AlgorithmError("Device with detector ID " + name + " is not registered.");
            }

            return (*it);
        }

        std::vector<Detector*> m_detectors;
    };
}
#endif
