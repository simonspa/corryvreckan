#ifndef BASICTRACKING_H
#define BASICTRACKING_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    class BasicTracking : public Algorithm {

    public:
        // Constructors and destructors
        BasicTracking(Configuration config, std::vector<Detector*> detectors);
        ~BasicTracking() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        //  Cluster* getNearestCluster(Cluster*, map<Cluster*, bool>, Clusters*);
        Cluster* getNearestCluster(long long int, Clusters);

        // Member variables

        // Histograms
        TH1F* trackChi2;
        TH1F* clustersPerTrack;
        TH1F* trackChi2ndof;
        TH1F* tracksPerEvent;
        TH1F* trackAngleX;
        TH1F* trackAngleY;
        std::map<std::string, TH1F*> residualsX;
        std::map<std::string, TH1F*> residualsXwidth1;
        std::map<std::string, TH1F*> residualsXwidth2;
        std::map<std::string, TH1F*> residualsXwidth3;
        std::map<std::string, TH1F*> residualsY;
        std::map<std::string, TH1F*> residualsYwidth1;
        std::map<std::string, TH1F*> residualsYwidth2;
        std::map<std::string, TH1F*> residualsYwidth3;

        // Cuts for tracking
        double timingCut;
        double spatialCut;
        int minHitsOnTrack;
        bool excludeDUT;
    };
}
#endif // BASICTRACKING_H
