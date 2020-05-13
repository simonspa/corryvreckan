/**
 * @file
 * @brief Definition of module Clustering4D
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CLUSTERING4D_H
#define CLUSTERING4D_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class Clustering4D : public Module {

    public:
        // Constructors and destructors
        Clustering4D(Configuration config, std::shared_ptr<Detector> detector);
        ~Clustering4D() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;
        static bool sortByTime(const std::shared_ptr<Pixel>& pixel1, const std::shared_ptr<Pixel>& pixel2);
        void calculateClusterCentre(Cluster*);
        bool touching(Pixel*, Cluster*);
        bool closeInTime(Pixel*, Cluster*);

        // Cluster histograms
        TH1F* clusterSize;
        TH1F* clusterSeedCharge;
        TH1F* clusterWidthRow;
        TH1F* clusterWidthColumn;
        TH1F* clusterCharge;
        TH1F* clusterCharge_1px;
        TH1F* clusterCharge_2px;
        TH1F* clusterCharge_3px;
        TH2F* clusterPositionGlobal;
        TH2F* clusterPositionLocal;
        TH1F* clusterTimes;
        TH1F* clusterMultiplicity;
        TH1F* pixelTimeMinusClusterTime;

        double time_cut_;
        int neighbor_radius_row_;
        int neighbor_radius_col_;
        bool charge_weighting_;
        bool use_earliest_pixel_;
        bool reject_by_ROI_;
    };
} // namespace corryvreckan
#endif // CLUSTERING4D_H
