#ifndef MASKCREATOR_H
#define MASKCREATOR_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH2F.h"
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
        std::map<std::string, std::map<int, int>> pixelhits;
        std::map<std::string, TH2F*> maskmap;
        double m_frequency;
    };
} // namespace corryvreckan
#endif // MASKCREATOR_H
