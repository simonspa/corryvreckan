#ifndef SpatialTracking_H
#define SpatialTracking_H 1

// Includes
#include <iostream>
// ROOT includes
#include "Math/Functor.h"
#include "Minuit2/Minuit2Minimizer.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
// Local includes
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

namespace corryvreckan {
    class SpatialTracking : public Algorithm {

    public:
        // Constructors and destructors
        SpatialTracking(Configuration config, std::vector<Detector*> detectors);
        ~SpatialTracking() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Histograms
        TH1F* trackChi2;
        TH1F* clustersPerTrack;
        TH1F* trackChi2ndof;
        TH1F* tracksPerEvent;
        TH1F* trackAngleX;
        TH1F* trackAngleY;
        std::map<std::string, TH1F*> residualsX;
        std::map<std::string, TH1F*> residualsY;

        // Member variables
        int m_eventNumber;
        double spatialCut;
        int minHitsOnTrack;
        double nTracksTotal;
        bool excludeDUT;
    };
}
#endif // SpatialTracking_H
