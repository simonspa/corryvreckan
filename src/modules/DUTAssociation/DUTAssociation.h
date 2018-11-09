#ifndef DUTAssociation_H
#define DUTAssociation_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.h"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class DUTAssociation : public Module {

    public:
        // Constructors and destructors
        DUTAssociation(Configuration config, std::shared_ptr<Detector> detector);
        ~DUTAssociation() = default;

        // Functions
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        std::shared_ptr<Detector> m_detector;
        double timingCut;
        ROOT::Math::XYVector spatialCut;
    };
} // namespace corryvreckan
#endif // DUTAssociation_H
