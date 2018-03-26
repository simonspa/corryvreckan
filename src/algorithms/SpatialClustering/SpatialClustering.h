#ifndef SpatialClustering_H
#define SpatialClustering_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class SpatialClustering : public Algorithm {

    public:
        // Constructors and destructors
        SpatialClustering(Configuration config, std::vector<Detector*> detectors);
        ~SpatialClustering() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        void calculateClusterCentre(Detector* detector, Cluster*);

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
