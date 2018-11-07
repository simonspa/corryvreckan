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
#include "objects/Cluster.h"
#include "objects/Track.h"

namespace corryvreckan {

    /** @ingroup Modules
     */
    class AlignmentTrackChi2 : public Module {

    public:
        // Constructors and destructors
        AlignmentTrackChi2(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~AlignmentTrackChi2() {}

        // Functions
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        static void MinimiseTrackChi2(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag);

        // Member variables
        Tracks m_alignmenttracks;
        int m_discardedtracks{};

        size_t nIterations;
        size_t m_numberOfTracksForAlignment;
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

        std::map<std::string, TGraph*> align_correction_shiftX;
        std::map<std::string, TGraph*> align_correction_shiftY;
        std::map<std::string, TGraph*> align_correction_shiftZ;
        std::map<std::string, TGraph*> align_correction_rotX;
        std::map<std::string, TGraph*> align_correction_rotY;
        std::map<std::string, TGraph*> align_correction_rotZ;
    };
} // namespace corryvreckan
#endif // AlignmentTrackChi2_H
