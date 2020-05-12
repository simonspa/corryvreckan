/**
 * @file
 * @brief Definition of module ImproveReferenceTimestamp
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ImproveReferenceTimestamp_H
#define ImproveReferenceTimestamp_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <cmath>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class ImproveReferenceTimestamp : public Module {

    public:
        // Constructors and destructors
        ImproveReferenceTimestamp(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~ImproveReferenceTimestamp() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

        // Histograms for several devices
        std::map<std::string, TH2F*> plotPerDevice;

        // Single histograms
        TH1F* singlePlot;

        // Member variables
        int m_eventNumber;
        int m_method;
        bool m_stop;
        std::string m_source;
        double m_triggerLatency;
    };
} // namespace corryvreckan
#endif // ImproveReferenceTimestamp_H
