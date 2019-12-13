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
        double timeCut;
        ROOT::Math::XYVector spatialCut;
        bool useClusterCentre;

        TH1F* hCutHisto;

        int num_cluster = 0;
        int assoc_cluster_counter = 0;
        int track_w_assoc_cls = 0;

        TH1F* hNoAssocCls;
        TH1D* hDistX;
        TH1D* hDistY;
        TH1D* hDistX_1px;
        TH1D* hDistY_1px;
        TH1D* hDistX_2px;
        TH1D* hDistY_2px;
        TH1D* hDistX_3px;
        TH1D* hDistY_3px;
    };
} // namespace corryvreckan
#endif // DUTAssociation_H
