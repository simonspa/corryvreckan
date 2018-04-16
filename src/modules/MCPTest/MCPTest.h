#ifndef MCPTest_H
#define MCPTest_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class MCPTest : public Module {

    public:
        // Constructors and destructors
        MCPTest(Configuration config, std::vector<Detector*> detectors);
        ~MCPTest() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int m_eventNumber{};
    };
} // namespace corryvreckan
#endif // MCPTest_H
