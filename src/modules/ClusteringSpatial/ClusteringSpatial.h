#ifndef ClusteringSpatial_H
#define ClusteringSpatial_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class ClusteringSpatial : public Module {

    public:
        // Constructors and destructors
        ClusteringSpatial(Configuration config, std::shared_ptr<Detector> detector);
        ~ClusteringSpatial() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        std::shared_ptr<Detector> m_detector;

        void calculateClusterCentre(Cluster*);

        // Cluster histograms
        TH1F* clusterSize;
        TH1F* clusterWidthRow;
        TH1F* clusterWidthColumn;
        TH1F* clusterCharge;
        TH2F* clusterPositionGlobal;
    };
} // namespace corryvreckan
#endif // ClusteringSpatial_H
