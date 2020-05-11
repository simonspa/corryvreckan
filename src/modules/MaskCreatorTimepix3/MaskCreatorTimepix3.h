/**
 * @file
 * @brief Definition of module MaskCreatorTimepix3
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TIMEPIX3MASKCREATOR_H
#define TIMEPIX3MASKCREATOR_H 1

#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Pixel.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class MaskCreatorTimepix3 : public Module {

    public:
        // Constructors and destructors
        MaskCreatorTimepix3(Configuration config, std::shared_ptr<Detector> detector);
        ~MaskCreatorTimepix3() {}

        // Functions
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;
        std::map<int, int> pixelhits;
    };
} // namespace corryvreckan
#endif // TIMEPIX3MASKCREATOR_H
