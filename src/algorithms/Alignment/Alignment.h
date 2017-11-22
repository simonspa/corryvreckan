#ifndef ALIGNMENT_H
#define ALIGNMENT_H 1

// ROOT includes
#include "Math/Functor.h"
#include "Minuit2/Minuit2Minimizer.h"
#include "TError.h"
#include "TGraph.h"
// Local includes
#include "core/algorithm/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

namespace corryvreckan {
    class Alignment : public Algorithm {

    public:
        // Constructors and destructors
        Alignment(Configuration config, std::vector<Detector*> detectors);
        ~Alignment() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        static void MinimiseTrackChi2(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag);
        static void MinimiseResiduals(Int_t& npar, Double_t* grad, Double_t& result, Double_t* par, Int_t flag);

        // Member variables
        Tracks m_alignmenttracks;
        int nIterations;
        int m_numberOfTracksForAlignment;
        int alignmentMethod;

        std::map<std::string, TGraph*> align_correction_shiftX;
        std::map<std::string, TGraph*> align_correction_shiftY;
        std::map<std::string, TGraph*> align_correction_shiftZ;
        std::map<std::string, TGraph*> align_correction_rotX;
        std::map<std::string, TGraph*> align_correction_rotY;
        std::map<std::string, TGraph*> align_correction_rotZ;
    };
} // namespace corryvreckan
#endif // ALIGNMENT_H
