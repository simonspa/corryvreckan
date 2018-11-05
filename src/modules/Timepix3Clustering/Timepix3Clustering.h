#ifndef TIMEPIX3CLUSTERING_H
#define TIMEPIX3CLUSTERING_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Timepix3Clustering : public Module {

    public:
        // Constructors and destructors
        Timepix3Clustering(Configuration config, std::vector<Detector*> detectors);
        ~Timepix3Clustering() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        static bool sortByTime(Pixel* pixel1, Pixel* pixel2);
        void calculateClusterCentre(Cluster*);
        bool touching(Pixel*, Cluster*);
        bool closeInTime(Pixel*, Cluster*);

        // Cluster histograms
        std::map<std::string, TH1F*> clusterSize;
        std::map<std::string, TH1F*> clusterWidthRow;
        std::map<std::string, TH1F*> clusterWidthColumn;
        std::map<std::string, TH1F*> clusterTot;
        std::map<std::string, TH2F*> clusterPositionGlobal;

        double timingCut;
        int neighbour_radius_row;
        int neighbour_radius_col;
    };
} // namespace corryvreckan
#endif // TIMEPIX3CLUSTERING_H
