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
     * @brief Module to do function
     *
     * More detailed explanation of module
     */

    enum streams { upstream, downstream };

    class TrackingMultiplet : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detectors Vector of pointers to the detectors
         */
        TrackingMultiplet(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

        /**
         * @brief [Initialise this module]
         */
        void initialise();

        /**
         * @brief Find tracks for upstream or downstream arm
         */
        TrackVector findMultipletArm(streams stream, std::map<std::string, KDTree*>& cluster_tree);

        /**
         * @brief Fill histograms for upstream or downstream arm
         */
        void fillMultipletArmHistograms(streams stream, TrackVector);

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard);

        /**
         * @brief [Finalise module]
         */
        void finalise();

    private:
        std::map<std::shared_ptr<Detector>, double> time_cuts_;
        std::map<std::shared_ptr<Detector>, XYVector> spatial_cuts_;

        double scatterer_position_;
        double scatterer_matching_cut_;

        std::vector<std::string> m_upstream_detectors;
        std::vector<std::string> m_downstream_detectors;

        size_t min_hits_upstream_;
        size_t min_hits_downstream_;

        MultipletVector m_multiplets;

        TH1F* upstreamMultiplicity;
        TH1F* downstreamMultiplicity;
        TH1F* multipletMultiplicity;

        std::map<std::string, TH1F*> residualsX;
        std::map<std::string, TH1F*> residualsY;

        TH1F* upstreamAngleX;
        TH1F* upstreamAngleY;
        TH1F* downstreamAngleX;
        TH1F* downstreamAngleY;

        TH1F* upstreamPositionAtScattererX;
        TH1F* upstreamPositionAtScattererY;
        TH1F* downstreamPositionAtScattererX;
        TH1F* downstreamPositionAtScattererY;

        TH1F* matchingDistanceAtScattererX;
        TH1F* matchingDistanceAtScattererY;

        TH1F* multipletOffsetAtScattererX;
        TH1F* multipletOffsetAtScattererY;

        TH1F* multipletKinkAtScattererX;
        TH1F* multipletKinkAtScattererY;
    };

} // namespace corryvreckan
