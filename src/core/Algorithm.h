#ifndef ALGORITHM_H
#define ALGORITHM_H 1

// Include files
#include <string>
#include "Clipboard.h"
#include "Parameters.h"
#include "TStopwatch.h"
#include "config/Configuration.hpp"

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
                    det << d->name();
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
        std::vector<Detector*> m_detectors;
    };
}
#endif
