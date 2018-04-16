#ifndef EUDAQ2EventLoader_H
#define EUDAQ2EventLoader_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "eudaq/FileReader.hh"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EUDAQ2EventLoader : public Module {

    public:
        // Constructors and destructors
        EUDAQ2EventLoader(Configuration config, std::vector<Detector*> detectors);
        ~EUDAQ2EventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // EUDAQ file reader instance, unique pointer:
        eudaq::FileReaderUP reader;

        // Member variables
        int m_eventNumber;
        std::string m_filename{};
    };
} // namespace corryvreckan
#endif // EUDAQ2EventLoader_H
