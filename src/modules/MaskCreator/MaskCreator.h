/**
 * @file
 * @brief Definition of module MaskCreator
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef MASKCREATOR_H
#define MASKCREATOR_H 1

#include <TCanvas.h>
#include <TH2D.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Pixel.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class MaskCreator : public Module {

    public:
        // Constructors and destructors
        MaskCreator(Configuration config, std::shared_ptr<Detector> detector);
        ~MaskCreator() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

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

        std::shared_ptr<Detector> m_detector;
        TH2D* m_occupancy;
        TH1D* m_occupancyDist;
        TH2D* m_density;
        TH2D* m_significance;
        TH1D* m_significanceDist;

        TH2F* maskmap;

        std::string m_method;
        double m_frequency, bandwidth;
        int m_bandwidthCol, m_bandwidthRow;
        double m_sigmaMax, m_rateMax;
        int m_numEvents, binsOccupancy;

        static inline void fillDist(const TH2D* values, TH1D* dist);
    };
} // namespace corryvreckan
#endif // MASKCREATOR_H
