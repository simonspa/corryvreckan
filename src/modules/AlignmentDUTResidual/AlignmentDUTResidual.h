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
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <iostream>

#include "core/module/Module.hpp"
#include "core/utils/ThreadPool.hpp"
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
        AlignmentDUTResidual(Configuration& config, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

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
        static ThreadPool* thread_pool;

        unsigned int m_workers;
        size_t nIterations;
        bool m_pruneTracks;
        bool m_alignPosition;
        bool m_alignOrientation;
        std::string m_alignPosition_axes;
        std::string m_alignOrientation_axes;
        size_t m_maxAssocClusters;
        double m_maxTrackChi2;

        TH1F* residualsXPlot;
        TH1F* residualsYPlot;

        TProfile* profile_dY_X;
        TProfile* profile_dY_Y;
        TProfile* profile_dX_X;
        TProfile* profile_dX_Y;
        TGraph* align_correction_shiftX;
        TGraph* align_correction_shiftY;
        TGraph* align_correction_rotX;
        TGraph* align_correction_rotY;
        TGraph* align_correction_rotZ;
    };

} // namespace corryvreckan
