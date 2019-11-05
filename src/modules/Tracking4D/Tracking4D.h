#ifndef TRACKING4D_H
#define TRACKING4D_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Tracking4D : public Module {

    public:
        // Constructors and destructors
        Tracking4D(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Tracking4D() {}

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
        size_t minHitsOnTrack;
        bool excludeDUT;
        std::vector<std::string> requireDetectors;
        std::string timestampFrom;
        std::string trackModel;
    };
} // namespace corryvreckan
#endif // TRACKING4D_H
