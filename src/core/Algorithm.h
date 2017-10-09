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
        Algorithm(Configuration config, Clipboard* clipboard) {
            m_name = config.getName();
            m_config = config;
            m_clipboard = clipboard;
            m_stopwatch = new TStopwatch();
        }
        virtual ~Algorithm() {}

        // Three main functions - initialise, run and finalise. Called for every
        // algorithm
        virtual void initialise(Parameters*) {}
        virtual StatusCode run(Clipboard*) {}
        virtual void finalise() {}

        // Methods to get member variables
        string getName() { return m_name; }
        TStopwatch* getStopwatch() { return m_stopwatch; }

    protected:
        // Member variables
        Parameters* parameters;
        TStopwatch* m_stopwatch;
        string m_name;
        Configuration m_config;
        Clipboard* m_clipboard;
    };
}
#endif
