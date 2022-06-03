/**
 * @file
 * @brief Definition of module AlignmentTrackChi2
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef AlignmentTrackChi2_H
#define AlignmentTrackChi2_H 1

// ROOT includes
#include <Math/Functor.h>
#include <Minuit2/Minuit2Minimizer.h>
#include <TError.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TProfile.h>

#include "core/module/Module.hpp"
#include "core/utils/ThreadPool.hpp"
#include "objects/Cluster.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {

    /** @ingroup Modules
     */
    class AlignmentTrackChi2 : public Module {

    public:
        // Constructors and destructors
        AlignmentTrackChi2(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors);
        ~AlignmentTrackChi2() {}

        // Functions
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        static void MinimiseTrackChi2(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag);
        // Member variables
        int m_discardedtracks{};

        // Static members
        static TrackVector globalTracks;
        static std::shared_ptr<Detector> globalDetector;
        static int detNum;
        static ThreadPool* thread_pool;

        unsigned int m_workers;
        size_t nIterations;
        bool m_pruneTracks;
        bool m_alignPosition;
        bool m_alignOrientation;
        size_t m_maxAssocClusters;
        double m_maxTrackChi2;

        std::map<std::string, TGraph*> align_correction_shiftX;
        std::map<std::string, TGraph*> align_correction_shiftY;
        std::map<std::string, TGraph*> align_correction_rotX;
        std::map<std::string, TGraph*> align_correction_rotY;
        std::map<std::string, TGraph*> align_correction_rotZ;
    };
} // namespace corryvreckan
#endif // AlignmentTrackChi2_H
