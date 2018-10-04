#ifndef DUTAssociation_H
#define DUTAssociation_H 1

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
    class DUTAssociation : public Module {

    public:
        // Constructors and destructors
        DUTAssociation(Configuration config, std::vector<Detector*> detectors);
        ~DUTAssociation() = default;

        // Functions
        StatusCode run(Clipboard* clipboard);

    private:
        std::string m_DUT;
        double timingCut, spatialCut;
    };
} // namespace corryvreckan
#endif // DUTAssociation_H
