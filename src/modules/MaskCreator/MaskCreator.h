#ifndef MASKCREATOR_H
#define MASKCREATOR_H 1

#include <iostream>
#include "TCanvas.h"
#include "TH2D.h"
#include "core/module/Module.hpp"
#include "objects/Pixel.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class MaskCreator : public Module {

    public:
        // Constructors and destructors
        MaskCreator(Configuration config, std::vector<Detector*> detectors);
        ~MaskCreator() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

    private:
        void localDensityEstimator();

        /** Write a smoothed density estimate to the density histogram.
         */
        static void estimateDensity(const TH2D* values, int bandwidthX, int bandwidthY, TH2D* density);

        /** Estimate the value at the given position from surrounding values.
         *
         * Use kernel density estimation w/ an Epanechnikov kernel to estimate the
         * density at the given point without using the actual value.
         */
        static double estimateDensityAtPosition(const TH2D* values, int i, int j, int bwi, int bwj);

        void globalFrequencyFilter();

        // Write out mask files for all detectors]
        void writeMaskFiles();

        // Member variables
        std::map<std::string, TH2D*> m_occupancy;
        std::map<std::string, TH1D*> m_occupancyDist;
        std::map<std::string, TH2D*> m_density;
        std::map<std::string, TH2D*> m_significance;
        std::map<std::string, TH1D*> m_significanceDist;

        std::map<std::string, TH2F*> maskmap;

        std::string m_method;
        double m_frequency, bandwidth;
        std::map<std::string, double> m_bandwidthCol, m_bandwidthRow;
        double m_sigmaMax, m_rateMax;
        int m_numEvents, binsOccupancy;

        static inline void fillDist(const TH2D* values, TH1D* dist);
    };
} // namespace corryvreckan
#endif // MASKCREATOR_H
