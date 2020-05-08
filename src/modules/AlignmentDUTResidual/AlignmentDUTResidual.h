/**
 * @file
 * @brief Definition of module AlignmentDUTResidual
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
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
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class AlignmentDUTResidual : public Module {

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param detector Pointer to the detector for this module instance
         */
        AlignmentDUTResidual(Configuration config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(std::shared_ptr<Clipboard> clipboard) override;

        /**
         * @brief [Finalise module]
         */
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        static void MinimiseResiduals(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag);

        std::shared_ptr<Detector> m_detector;
        int m_discardedtracks{};

        // Global container declarations
        static TrackVector globalTracks;
        static std::shared_ptr<Detector> globalDetector;

        size_t nIterations;
        bool m_pruneTracks;
        bool m_alignPosition;
        bool m_alignOrientation;
        size_t m_maxAssocClusters;
        double m_maxTrackChi2;

        TH1F* residualsXPlot;
        TH1F* residualsYPlot;

        TProfile* profile_dY_X;
        TProfile* profile_dY_Y;
        TProfile* profile_dX_X;
        TProfile* profile_dX_Y;
    };

} // namespace corryvreckan
