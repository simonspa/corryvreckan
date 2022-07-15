/**
 * @file
 * @brief Definition of module Tracking4D
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TRACKING4D_H
#define TRACKING4D_H 1

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
    class Tracking4D : public Module {

    public:
        // Constructors and destructors
        Tracking4D(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);
        ~Tracking4D() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        // Histograms
        TH1F* trackChi2;
        TH1F* clustersPerTrack;
        TH1F* trackChi2ndof;
        TH1F* trackTime;
        TH1F* trackTimeTrigger;
        TH2F* trackTimeTriggerChi2;
        TH1F* tracksPerEvent;
        TH1F* trackAngleX;
        TH1F* trackAngleY;
        TH1F* tracksVsTime;
        std::map<std::string, TH1F*> residualsX_local;
        std::map<std::string, TH1F*> residualsXwidth1_local;
        std::map<std::string, TH1F*> residualsXwidth2_local;
        std::map<std::string, TH1F*> residualsXwidth3_local;
        std::map<std::string, TH1F*> pullY_local;
        std::map<std::string, TH1F*> residualsY_local;
        std::map<std::string, TH1F*> residualsYwidth1_local;
        std::map<std::string, TH1F*> residualsYwidth2_local;
        std::map<std::string, TH1F*> residualsYwidth3_local;
        std::map<std::string, TH1F*> pullX_local;

        std::map<std::string, TH1F*> residualsX_global;
        std::map<std::string, TH2F*> residualsX_vs_positionX_global;
        std::map<std::string, TH2F*> residualsX_vs_positionY_global;
        std::map<std::string, TH1F*> residualsXwidth1_global;
        std::map<std::string, TH1F*> residualsXwidth2_global;
        std::map<std::string, TH1F*> residualsXwidth3_global;
        std::map<std::string, TH1F*> pullX_global;
        std::map<std::string, TH1F*> residualsY_global;
        std::map<std::string, TH2F*> residualsY_vs_positionY_global;
        std::map<std::string, TH2F*> residualsY_vs_positionX_global;
        std::map<std::string, TH1F*> residualsYwidth1_global;
        std::map<std::string, TH1F*> residualsYwidth2_global;
        std::map<std::string, TH1F*> residualsYwidth3_global;
        std::map<std::string, TH1F*> pullY_global;
        std::map<std::string, TH1F*> residualsZ_global;

        std::map<std::string, TH1F*> kinkX;
        std::map<std::string, TH1F*> kinkY;

        std::map<std::string, TH2F*> local_intersects_;

        // Cuts for tracking
        double momentum_;
        double max_plot_chi2_;
        double volume_radiation_length_;
        size_t min_hits_on_track_;
        bool exclude_DUT_;
        bool use_volume_scatterer_;
        bool reject_by_ROI_;
        bool unique_cluster_usage_;
        std::vector<std::string> require_detectors_;
        std::vector<std::string> exclude_from_seed_;
        std::map<std::shared_ptr<Detector>, double> time_cuts_;
        std::map<std::shared_ptr<Detector>, XYVector> spatial_cuts_;
        std::string timestamp_from_;
        std::string track_model_;

        // Function to calculate the weighted average timestamp from the clusters of a track
        double calculate_average_timestamp(const Track* track);
    };
} // namespace corryvreckan
#endif // TRACKING4D_H
