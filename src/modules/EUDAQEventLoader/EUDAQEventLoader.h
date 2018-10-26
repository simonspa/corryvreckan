#ifndef EUDAQEventLoader_H
#define EUDAQEventLoader_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "eudaq/FileReader.hh"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EUDAQEventLoader : public Module {

    public:
        // Constructors and destructors
        EUDAQEventLoader(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EUDAQEventLoader() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // EUDAQ file reader instance:
        eudaq::FileReader* reader;

        // Member variables
        int m_eventNumber;
        std::string m_filename{};
        bool m_longID;
    };
} // namespace corryvreckan
#endif // EUDAQEventLoader_H
