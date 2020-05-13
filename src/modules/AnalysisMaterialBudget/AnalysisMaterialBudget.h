/**
 * @file
 * @brief Definition of module AnalysisMaterialBudget
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <iostream>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class AnalysisMaterialBudget : public Module {

    public:
        // Constructor
        AnalysisMaterialBudget(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);

        // Module functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        int m_eventNumber;

        ROOT::Math::XYVector cell_size_;
        ROOT::Math::XYVector image_size_;

        double angle_cut_;
        double quantile_cut_;
        int min_cell_content_;
        bool live_update_;

        int n_cells_x, n_cells_y;

        TH1F* entriesPerCell;

        TH1F* trackKinkX;
        TH1F* trackKinkY;
        TProfile* kinkVsX;
        TProfile* kinkVsY;
        TProfile2D* MBIpreview;
        TH2F* MBI;
        TH2F* meanAngles;

        std::map<std::pair<int, int>, std::vector<double>> m_all_kinks;
        std::map<std::pair<int, int>, double> m_all_sum;
        std::map<std::pair<int, int>, int> m_all_entries;

        /**
         * @brief Method re-calculating the AAD of a given image cell
         * @param Cell ID in x
         * @param Cell ID in y
         */
        double getAAD(int cell_x, int cell_y);
    };

} // namespace corryvreckan
