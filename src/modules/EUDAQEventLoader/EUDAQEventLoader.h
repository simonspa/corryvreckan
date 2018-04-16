#ifndef EUDAQEventLoader_H
#define EUDAQEventLoader_H 1

#include <iostream>
#include "core/algorithm/Algorithm.h"
#include "eudaq/FileReader.hh"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class EUDAQEventLoader : public Algorithm {

    public:
        // Constructors and destructors
        EUDAQEventLoader(Configuration config, std::vector<Detector*> detectors);
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
