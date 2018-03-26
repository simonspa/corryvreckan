#ifndef MASKCREATOR_H
#define MASKCREATOR_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH2D.h"
#include "core/algorithm/Algorithm.h"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Algorithms
     */
    class MaskCreator : public Algorithm {

    public:
        // Constructors and destructors
        MaskCreator(Configuration config, std::vector<Detector*> detectors);
        ~MaskCreator() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        std::map<std::string, TH2D*> m_occupancy;
        std::map<std::string, TH2F*> maskmap;
        double m_frequency;
    };
} // namespace corryvreckan
#endif // MASKCREATOR_H
