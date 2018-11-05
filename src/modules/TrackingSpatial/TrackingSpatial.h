#ifndef SpatialTracking_H
#define SpatialTracking_H 1

#include <Math/Functor.h>
#include <Minuit2/Minuit2Minimizer.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <iostream>

// Local includes
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class SpatialTracking : public Module {

    public:
        // Constructors and destructors
        SpatialTracking(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
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
        double spatialCut, spatialCut_DUT;
        size_t minHitsOnTrack;
        double nTracksTotal;
        bool excludeDUT;
    };
} // namespace corryvreckan
#endif // SpatialTracking_H
