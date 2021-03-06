#ifndef TrackingSpatial_H
#define TrackingSpatial_H 1

#include <Math/Functor.h>
#include <Minuit2/Minuit2Minimizer.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <iostream>

// Local includes
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class TrackingSpatial : public Module {

    public:
        // Constructors and destructors
        TrackingSpatial(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~TrackingSpatial() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
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
        std::map<std::shared_ptr<Detector>, XYVector> spatial_cuts_;
        size_t minHitsOnTrack;
        bool excludeDUT;
        std::string trackModel;
    };
} // namespace corryvreckan
#endif // TrackingSpatial_H
