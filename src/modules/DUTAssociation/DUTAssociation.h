#ifndef DUTAssociation_H
#define DUTAssociation_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
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
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;
        double timingCut;
        ROOT::Math::XYVector spatialCut;
        bool useClusterCentre;

        int assoc_cluster_counter = 0;

        TH1D* hX1X2;
        TH1D* hY1Y2;
        TH1D* hX1X2_1px;
        TH1D* hY1Y2_1px;
        TH1D* hX1X2_2px;
        TH1D* hY1Y2_2px;
        TH1D* hX1X2_3px;
        TH1D* hY1Y2_3px;
    };
} // namespace corryvreckan
#endif // DUTAssociation_H
