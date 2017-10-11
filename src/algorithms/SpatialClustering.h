#ifndef SpatialClustering_H
#define SpatialClustering_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/Algorithm.h"
#include "objects/Cluster.h"

namespace corryvreckan {
    class SpatialClustering : public Algorithm {

    public:
        // Constructors and destructors
        SpatialClustering(Configuration config, std::vector<Detector*> detectors);
        ~SpatialClustering() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard* clipboard);
        void finalise();

        void calculateClusterCentre(Cluster*);

        // Member variables
        int m_eventNumber;
    };
}
#endif // SpatialClustering_H
