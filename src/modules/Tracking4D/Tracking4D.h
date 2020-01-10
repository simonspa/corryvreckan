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

        std::map<std::string, TH1F*> kinkX;
        std::map<std::string, TH1F*> kinkY;
        std::map<std::string, TH1F*> pullX;
        std::map<std::string, TH1F*> pullY;
        // Cuts for tracking
        double momentum{};
        double volumeScatteringLength{};
        double time_cut_reference_;
        size_t minHitsOnTrack;
        bool excludeDUT;
        bool useVolumeScatterer{};
        std::vector<std::string> requireDetectors;
        std::map<std::shared_ptr<Detector>, double> time_cuts_;
        std::map<std::shared_ptr<Detector>, XYVector> spatial_cuts_;
        std::string timestampFrom;
        std::string trackModel;
    };
} // namespace corryvreckan
#endif // TRACKING4D_H
