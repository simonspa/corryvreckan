#ifndef Metronome_H
#define Metronome_H 1

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
    class Metronome : public Module {

    public:
        // Constructors and destructors
        Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Metronome() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);

    private:
        double m_eventStart, m_eventEnd, m_eventLength;
    };
} // namespace corryvreckan
#endif // Metronome_H
