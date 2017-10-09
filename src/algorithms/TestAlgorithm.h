#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    class TestAlgorithm : public Algorithm {

    public:
        // Constructors and destructors
        TestAlgorithm(Configuration config, Clipboard* clipboard);
        ~TestAlgorithm() {}

        // Functions
        void initialise(Parameters*);
        StatusCode run(Clipboard*);
        void finalise();

        // Pixel histograms
        std::map<std::string, TH2F*> hitmap;
        std::map<std::string, TH1F*> eventTimes;

        // Cluster histograms
        std::map<std::string, TH1F*> clusterSize;
        std::map<std::string, TH1F*> clusterTot;
        std::map<std::string, TH2F*> clusterPositionGlobal;

        // Correlation plots
        std::map<std::string, TH1F*> correlationX;
        std::map<std::string, TH1F*> correlationY;
        std::map<std::string, TH1F*> correlationTime;
        std::map<std::string, TH1F*> correlationTimeInt;

        // Parameters which can be set by user
        bool makeCorrelations;
    };
}
#endif // TESTALGORITHM_H
