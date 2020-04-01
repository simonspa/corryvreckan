/**
 * @file
 * @brief Definition of module TrackingMultiplet
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/KDTree.hpp"
#include "objects/Multiplet.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */

    // enum to differentiate between up- and downstream arm in functions
    enum streams { upstream, downstream };

    class TrackingMultiplet : public Module {

    public:
        // Constructors and destructors
        TrackingMultiplet(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~TrackingMultiplet() {}

        // Init, run and finalise functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

        /**
         * @brief Find tracklets for upstream or downstream arm
         */
        TrackVector findMultipletTracklets(streams stream, std::map<std::string, KDTree*>& cluster_tree);

        /**
         * @brief Fill histograms for upstream or downstream arm
         */
        void fillMultipletArmHistograms(streams stream, TrackVector);

    private:
        // Configuration members
        std::map<std::shared_ptr<Detector>, double> time_cuts_;
        std::map<std::shared_ptr<Detector>, XYVector> spatial_cuts_;

        std::vector<std::string> m_upstream_detectors;
        std::vector<std::string> m_downstream_detectors;

        double scatterer_position_;
        double scatterer_matching_cut_;

        size_t min_hits_upstream_;
        size_t min_hits_downstream_;

        // Member histograms
        std::map<streams, TH1F*> trackletMultiplicity;
        std::map<streams, TH1F*> clustersPerTracklet;

        std::map<streams, TH1F*> trackletAngleX;
        std::map<streams, TH1F*> trackletAngleY;
        std::map<streams, TH1F*> trackletPositionAtScattererX;
        std::map<streams, TH1F*> trackletPositionAtScattererY;

        std::map<std::string, TH1F*> residualsX;
        std::map<std::string, TH1F*> residualsY;

        TH1F* multipletMultiplicity;

        TH1F* matchingDistanceAtScattererX;
        TH1F* matchingDistanceAtScattererY;

        TH1F* multipletOffsetAtScattererX;
        TH1F* multipletOffsetAtScattererY;

        TH1F* multipletKinkAtScattererX;
        TH1F* multipletKinkAtScattererY;
    };

} // namespace corryvreckan
