#ifndef PREALIGNMENT_H
#define PREALIGNMENT_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Prealignment : public Module {

    public:
        // Constructors and destructors
        Prealignment(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Prealignment() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Correlation plots
        std::map<std::string, TH1F*> correlationX;
        std::map<std::string, TH1F*> correlationY;
        std::map<std::string, TH2F*> correlationX2Dlocal;
        std::map<std::string, TH2F*> correlationY2Dlocal;
        std::map<std::string, TH2F*> correlationX2D;
        std::map<std::string, TH2F*> correlationY2D;

        // Parameters which can be set by user
        double max_correlation_rms;
        double damping_factor;
        double timingCut;
    };
} // namespace corryvreckan
#endif // PREALIGNMENT_H
