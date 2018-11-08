#ifndef TIMEPIX3MASKCREATOR_H
#define TIMEPIX3MASKCREATOR_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class MaskCreatorTimepix3 : public Module {

    public:
        // Constructors and destructors
        MaskCreatorTimepix3(Configuration config, std::shared_ptr<Detector> detector);
        ~MaskCreatorTimepix3() {}

        // Functions
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;
        std::map<int, int> pixelhits;
    };
} // namespace corryvreckan
#endif // TIMEPIX3MASKCREATOR_H
