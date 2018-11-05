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
        Prealignment(Configuration config, std::shared_ptr<Detector> detector);
        ~Prealignment() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;

        // Correlation plots
        TH1F* correlationX;
        TH1F* correlationY;
        TH2F* correlationX2Dlocal;
        TH2F* correlationY2Dlocal;
        TH2F* correlationX2D;
        TH2F* correlationY2D;

        // Parameters which can be set by user
        double max_correlation_rms;
        double damping_factor;
        double timingCut;
    };
} // namespace corryvreckan
#endif // PREALIGNMENT_H
