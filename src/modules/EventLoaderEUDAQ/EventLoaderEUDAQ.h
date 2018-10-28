#ifndef EventLoaderEUDAQ_H
#define EventLoaderEUDAQ_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "eudaq/FileReader.hh"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderEUDAQ : public Module {

    public:
        // Constructors and destructors
        EventLoaderEUDAQ(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EventLoaderEUDAQ() {}

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
#endif // EventLoaderEUDAQ_H
