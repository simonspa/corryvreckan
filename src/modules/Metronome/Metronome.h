#ifndef Metronome_H
#define Metronome_H 1

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
    class Metronome : public Module {

    public:
        // Constructors and destructors
        Metronome(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Metronome() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

    private:
        double m_eventStart, m_eventEnd, m_eventLength;
    };
} // namespace corryvreckan
#endif // Metronome_H
