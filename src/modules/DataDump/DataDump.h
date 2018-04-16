#ifndef DataDump_H
#define DataDump_H 1

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class DataDump : public Module {

    public:
        // Constructors and destructors
        DataDump(Configuration config, std::vector<Detector*> detectors);
        ~DataDump() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int m_eventNumber;
        std::string m_detector;
    };
} // namespace corryvreckan
#endif // DataDump_H