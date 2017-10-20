#ifndef PREALIGNMENT_H
#define PREALIGNMENT_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    class Prealignment : public Algorithm {

    public:
        // Constructors and destructors
        Prealignment(Configuration config, std::vector<Detector*> detectors);
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
    };
}
#endif // PREALIGNMENT_H
