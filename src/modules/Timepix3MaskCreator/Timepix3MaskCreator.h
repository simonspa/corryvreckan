#ifndef TIMEPIX3MASKCREATOR_H
#define TIMEPIX3MASKCREATOR_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Timepix3MaskCreator : public Module {

    public:
        // Constructors and destructors
        Timepix3MaskCreator(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Timepix3MaskCreator() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        std::map<std::string, std::map<int, int>> pixelhits;
    };
} // namespace corryvreckan
#endif // TIMEPIX3MASKCREATOR_H
