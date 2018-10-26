#ifndef SpatialClustering_H
#define SpatialClustering_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class SpatialClustering : public Module {

    public:
        // Constructors and destructors
        SpatialClustering(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~SpatialClustering() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        void calculateClusterCentre(std::shared_ptr<Detector> detector, Cluster*);

    private:
        // Cluster histograms
        std::map<std::string, TH1F*> clusterSize;
        std::map<std::string, TH1F*> clusterWidthRow;
        std::map<std::string, TH1F*> clusterWidthColumn;
        std::map<std::string, TH1F*> clusterTot;
        std::map<std::string, TH2F*> clusterPositionGlobal;

        // Member variables
        int m_eventNumber;
    };
} // namespace corryvreckan
#endif // SpatialClustering_H
