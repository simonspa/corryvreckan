#ifndef Timepix1Clustering_H
#define Timepix1Clustering_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/Algorithm.h"
#include "objects/Cluster.h"

namespace corryvreckan {
    class Timepix1Clustering : public Algorithm {

    public:
        // Constructors and destructors
        Timepix1Clustering(Configuration config, Clipboard* clipboard);
        ~Timepix1Clustering() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        void calculateClusterCentre(Cluster*);

        // Member variables
        int m_eventNumber;
    };
}
#endif // Timepix1Clustering_H
