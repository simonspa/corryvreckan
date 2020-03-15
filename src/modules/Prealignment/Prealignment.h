#ifndef PREALIGNMENT_H
#define PREALIGNMENT_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"

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
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
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
        double timeCut;
	std::string prealign_method;
	double fit_high;
	double fit_low;
    };
} // namespace corryvreckan
#endif // PREALIGNMENT_H
