#ifndef RegionOfInterest_H
#define RegionOfInterest_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "core/module/Module.hpp"
#include "objects/Cluster.h"
#include "objects/Pixel.h"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class RegionOfInterest : public Module {

    public:
        // Constructors and destructors
        RegionOfInterest(Configuration config, std::vector<Detector*> detectors);
        ~RegionOfInterest() {}

        StatusCode run(Clipboard* clipboard);

    private:
        static int winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon);
        inline static int isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2);

        std::vector<std::vector<int>> m_roi;
        std::string m_DUT;
    };
} // namespace corryvreckan
#endif // RegionOfInterest_H
