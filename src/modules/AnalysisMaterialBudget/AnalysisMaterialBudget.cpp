/**
 * @file
 * @brief Implementation of module AnalysisMaterialBudget
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisMaterialBudget.h"
#include "objects/Multiplet.hpp"

using namespace corryvreckan;

AnalysisMaterialBudget::AnalysisMaterialBudget(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {
    cell_size_ = m_config.get<ROOT::Math::XYVector>(
        "cell_size", ROOT::Math::XYVector(Units::get<double>(50, "um"), Units::get<double>(50, "um")));
    image_size_ = m_config.get<ROOT::Math::XYVector>("image_size", ROOT::Math::XYVector(10, 10));

    angle_cut_ = m_config.get<double>("angle_cut", Units::get<double>(100, "mrad"));
    double quantiles = m_config.get<double>("quantile", 0.9);
    quantile_cut_ = (1.0 - quantiles) / 2.0;
    min_cell_content_ = m_config.get<int>("min_cell_content", 20);
    update_ = m_config.get<bool>("update", false);
}

void AnalysisMaterialBudget::initialize() {

    trackKinkX = new TH1F("trackKinkX", "Track Kink X; kink x [mrad]; Tracks", 200, -200, 200);
    trackKinkY = new TH1F("trackKinkY", "Track Kink Y; kink y [mrad]; Tracks", 200, -200, 200);

    n_cells_x = int(floor(static_cast<double>(Units::convert(image_size_.x(), "mm")) /
                          static_cast<double>(Units::convert(cell_size_.x(), "mm"))));
    n_cells_y = int(floor(static_cast<double>(Units::convert(image_size_.y(), "mm")) /
                          static_cast<double>(Units::convert(cell_size_.y(), "mm"))));

    double angle_cut_mrad = static_cast<double>(Units::convert(angle_cut_, "mrad"));

    kinkVsX = new TProfile("kinkVsX",
                           "Kink vs position x; x [mm]; <kink^{2}> [mrad^{2}]",
                           n_cells_x,
                           -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                           static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                           0,
                           angle_cut_mrad * angle_cut_mrad);
    kinkVsY = new TProfile("kinkVsY",
                           "Kink vs position y; y [mm]; <kink^{2}> [mrad^{2}]",
                           n_cells_y,
                           -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                           static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                           0,
                           angle_cut_mrad * angle_cut_mrad);

    MBIpreview = new TProfile2D("MBIpreview",
                                "Square kink vs incidence position, medium; x [mm]; y [mm]; <kink^{2}> [mrad^{2}]",
                                n_cells_x,
                                -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                                static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                                n_cells_y,
                                -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                                static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                                0,
                                angle_cut_mrad * angle_cut_mrad);
    MBI = new TH2F("MBI",
                   "Material Budget Image (AAD^{2}); x [mm]; y [mm]; AAD(kink)^{2} [mrad^{2}]",
                   n_cells_x,
                   -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                   static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                   n_cells_y,
                   -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                   static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2);

    MBIpreviewSqrt = new TProfile2D("MBIpreviewSqrt",
                                    "Kink vs incidence position, medium; x [mm]; y [mm]; RMS(kink) [mrad]",
                                    n_cells_x,
                                    -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                                    static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                                    n_cells_y,
                                    -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                                    static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                                    0,
                                    angle_cut_mrad * angle_cut_mrad);
    MBISqrt = new TH2F("MBISqrt",
                       "Material Budget Image (AAD); x [mm]; y [mm]; AAD(kink) [mrad]",
                       n_cells_x,
                       -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                       static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                       n_cells_y,
                       -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                       static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2);

    meanAngles = new TH2F("meanAngles",
                          "Mean Angles; x [mm]; y [mm]; <kink> [mrad]",
                          n_cells_x,
                          -static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                          static_cast<double>(Units::convert(image_size_.x(), "mm")) / 2,
                          n_cells_y,
                          -static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2,
                          static_cast<double>(Units::convert(image_size_.y(), "mm")) / 2);

    for(int ix = 0; ix < n_cells_x; ++ix) {
        for(int iy = 0; iy < n_cells_y; ++iy) {
            m_all_kinks.insert(std::make_pair(std::make_pair(ix, iy), std::multiset<double>()));
            m_all_sum.insert(std::make_pair(std::make_pair(ix, iy), 0));
            MBI->SetBinContent(ix, iy, 0);
        }
    }

    entriesPerCell = new TH1F("entriesPerCell", "Entries per image cell; entries; #cells", 500, -0.5, 999.5);
    entriesPerCell->SetBinContent(0, n_cells_x * n_cells_y);

    m_eventNumber = 0;
}

double AnalysisMaterialBudget::get_aad(int cell_x, int cell_y) {

    // Calculate the quantile offsets
    size_t entries = m_all_kinks.at(std::make_pair(cell_x, cell_y)).size();
    size_t cut_off = size_t(round(double(entries) * quantile_cut_));

    double AAD = 0;

    size_t i = 0;
    for(auto it = m_all_kinks.at(std::make_pair(cell_x, cell_y)).begin();
        it != m_all_kinks.at(std::make_pair(cell_x, cell_y)).end();
        ++it) {
        if(i >= cut_off && i < entries - cut_off) {
            AAD += fabs(*it);
        }
        ++i;
    }
    AAD /= double(entries - 2 * cut_off);

    return AAD;
}

StatusCode AnalysisMaterialBudget::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto tracks = clipboard->getData<Track>();

    for(auto& track : tracks) {
        auto multiplet = std::dynamic_pointer_cast<Multiplet>(track);
        if(multiplet == nullptr) {
            LOG_ONCE(WARNING) << "Stored track is not a Multiplet. Skipping this track.";
            continue;
        };

        double pos_x = static_cast<double>(Units::convert(multiplet->getPositionAtScatterer().x(), "mm"));
        double pos_y = static_cast<double>(Units::convert(multiplet->getPositionAtScatterer().y(), "mm"));
        if(fabs(pos_x) > image_size_.x() / 2. || fabs(pos_y) > image_size_.y() / 2.) {
            continue;
        }
        double kink_x = static_cast<double>(Units::convert(multiplet->getKinkAtScatterer().x(), "mrad"));
        double kink_y = static_cast<double>(Units::convert(multiplet->getKinkAtScatterer().y(), "mrad"));

        double kink_x_sq = kink_x * kink_x;
        double kink_y_sq = kink_y * kink_y;

        trackKinkX->Fill(kink_x);
        trackKinkY->Fill(kink_y);

        kinkVsX->Fill(pos_x, kink_x_sq);
        kinkVsX->Fill(pos_x, kink_y_sq);
        kinkVsY->Fill(pos_y, kink_x_sq);
        kinkVsY->Fill(pos_y, kink_y_sq);

        MBIpreview->Fill(pos_x, pos_y, kink_x_sq);
        MBIpreview->Fill(pos_x, pos_y, kink_y_sq);

        MBIpreviewSqrt->Fill(pos_x, pos_y, kink_x);
        MBIpreviewSqrt->Fill(pos_x, pos_y, kink_y);

        int cell_x = int((pos_x + image_size_.x() / 2) / cell_size_.x());
        int cell_y = int((pos_y + image_size_.y() / 2) / cell_size_.y());
        LOG(DEBUG) << "Track is in cell " << cell_x << " " << cell_y;

        if(cell_x < 0 || cell_x >= n_cells_x) {
            LOG(DEBUG) << "Cell out of boundaries in x. cell=" << cell_x << "   n_cells_x=" << n_cells_x;
            continue;
        }
        if(cell_y < 0 || cell_y >= n_cells_y) {
            LOG(DEBUG) << "Cell out of boundaries in y. cell=" << cell_y << "   n_cells_y=" << n_cells_y;
            continue;
        }

        // Fill multiset of kinks
        int filled_angles = 0;
        if(fabs(kink_x) < static_cast<double>(Units::convert(angle_cut_, "mrad"))) {
            m_all_kinks.at(std::make_pair(cell_x, cell_y)).insert(kink_x);
            ++filled_angles;
        }
        if(fabs(kink_y) < static_cast<double>(Units::convert(angle_cut_, "mrad"))) {
            m_all_kinks.at(std::make_pair(cell_x, cell_y)).insert(kink_y);
            ++filled_angles;
        }

        m_all_sum.at(std::make_pair(cell_x, cell_y)) += (kink_x + kink_y);

        auto entries = static_cast<int>(m_all_kinks.at(std::make_pair(cell_x, cell_y)).size());

        entriesPerCell->SetBinContent(entries - filled_angles, entriesPerCell->GetBinContent(entries - filled_angles) - 1);
        entriesPerCell->SetBinContent(entries, entriesPerCell->GetBinContent(entries) + 1);

        if(update_) {
            // Calculate AAD and set image value
            if(entries >= min_cell_content_) {
                auto aad = get_aad(cell_x, cell_y);
                MBI->SetBinContent(cell_x, cell_y, aad * aad);
                MBISqrt->SetBinContent(cell_x, cell_y, aad);
                meanAngles->SetBinContent(cell_x, cell_y, m_all_sum.at(std::make_pair(cell_x, cell_y)) / entries);
            }
        }
    }

    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisMaterialBudget::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";

    if(!update_) {
        for(int cell_x = 0; cell_x < n_cells_x; ++cell_x) {
            for(int cell_y = 0; cell_y < n_cells_y; ++cell_y) {
                auto entries = static_cast<int>(m_all_kinks.at(std::make_pair(cell_x, cell_y)).size());
                if(entries >= min_cell_content_) {
                    auto aad = get_aad(cell_x, cell_y);
                    MBI->SetBinContent(cell_x, cell_y, aad * aad);
                    MBISqrt->SetBinContent(cell_x, cell_y, aad);
                    meanAngles->SetBinContent(cell_x, cell_y, m_all_sum.at(std::make_pair(cell_x, cell_y)) / entries);
                }
            }
        }
    }
}
