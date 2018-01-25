#ifndef SpatialClustering_H
#define SpatialClustering_H 1

#include <iostream>
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

        // Member variables
        int m_eventNumber;
    };
} // namespace corryvreckan
#endif // SpatialClustering_H
