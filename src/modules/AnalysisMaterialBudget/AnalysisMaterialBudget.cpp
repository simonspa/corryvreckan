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

using namespace corryvreckan;

AnalysisMaterialBudget::AnalysisMaterialBudget(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(std::move(config), std::move(detectors)) {
    cell_size = m_config.get<ROOT::Math::XYVector>(
        "cell_size", ROOT::Math::XYVector(Units::get<double>(50, "um"), Units::get<double>(50, "um")));
    image_size = m_config.get<ROOT::Math::XYVector>("image_size", ROOT::Math::XYVector(10, 10));

    angle_cut = m_config.get<double>("angle_cut", Units::get<double>(100, "mrad"));
    write_3D = m_config.get<bool>("write_3D", false);
    min_cell_content = m_config.get<int>("min_cell_content", 20);
}

void AnalysisMaterialBudget::initialise() {

    sixletKinkX = new TH1F("sixletKinkX", "Sixlet Kink X", 200, -200, 200);
    sixletKinkY = new TH1F("sixletKinkY", "Sixlet Kink Y", 200, -200, 200);

    n_cells_x = int(floor(static_cast<double>(Units::convert(image_size.x(), "mm")) /
                          static_cast<double>(Units::convert(cell_size.x(), "mm"))));
    n_cells_y = int(floor(static_cast<double>(Units::convert(image_size.y(), "mm")) /
                          static_cast<double>(Units::convert(cell_size.y(), "mm"))));

    MBIpreview = new TProfile2D("MBIpreview",
                                "Kink vs incidence position, medium",
                                n_cells_x,
                                -static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                                static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                                n_cells_y,
                                -static_cast<double>(Units::convert(image_size.y(), "mm")) / 2,
                                static_cast<double>(Units::convert(image_size.y(), "mm")) / 2,
                                0,
                                100);
    MBI = new TH2F("MBI",
                   "Material Budget Image; x [mm]; y [mm]; AAD90^{2} [mrad^{2}]",
                   n_cells_x,
                   -static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                   static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                   n_cells_y,
                   -static_cast<double>(Units::convert(image_size.y(), "mm")) / 2,
                   static_cast<double>(Units::convert(image_size.y(), "mm")) / 2);
    meanAngles = new TH2F("meanAngles",
                          "Mean Angles; x [mm]; y [mm]; <kink> [mrad]",
                          n_cells_x,
                          -static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                          static_cast<double>(Units::convert(image_size.x(), "mm")) / 2,
                          n_cells_y,
                          -static_cast<double>(Units::convert(image_size.y(), "mm")) / 2,
                          static_cast<double>(Units::convert(image_size.y(), "mm")) / 2);

    if(write_3D) {
        MBI3D = new TH3F(
            "MBI3D", "Kink vs incidence position; x [mm]; y [mm]; kink [mrad]", 500, -10, 10, 250, -5, 5, 1000, -30, 30);
    }

    for(int ix = 0; ix < n_cells_x; ++ix) {
        for(int iy = 0; iy < n_cells_y; ++iy) {
            m_all_kinks.insert(std::make_pair(std::make_pair(ix, iy), std::vector<double>()));
            m_all_entries.insert(std::make_pair(std::make_pair(ix, iy), 0));
            m_all_sum.insert(std::make_pair(std::make_pair(ix, iy), 0));
            MBI->SetBinContent(ix, iy, 0);
        }
    }

    entriesPerCell = new TH1F("entriesPerCell", "Entries per image cell; entries; #cells", 800, -0.5, 799.5);
    entriesPerCell->SetBinContent(0, n_cells_x * n_cells_y);

    m_eventNumber = 0;
}

double AnalysisMaterialBudget::getAAD(int cell_x, int cell_y) {

    // First let's sort the vector
    std::vector<double> vec = m_all_kinks.at(std::make_pair(cell_x, cell_y));

    std::sort(vec.begin(), vec.end());

    // Then chop off the outer 5% on both sides
    int cut_off = int(round(double(vec.size()) * 0.05));
    for(int i = 0; i < cut_off; ++i) {
        vec.erase(vec.begin());
        vec.pop_back();
    }

    // Now calculate the AAD
    double AAD = 0;

    for(auto const& val : vec) {
        AAD +=
            (fabs(val - (m_all_sum.at(std::make_pair(cell_x, cell_y)) / m_all_entries.at(std::make_pair(cell_x, cell_y)))));
    }
    AAD /= double(vec.size());

    return AAD;
}

StatusCode AnalysisMaterialBudget::run(std::shared_ptr<Clipboard> clipboard) {

    auto tracks = clipboard->getData<Track>();

    if(tracks == nullptr) {
        return StatusCode::Success;
    }

    for(auto& track : (*tracks)) {
        double pos_x = static_cast<double>(Units::convert(track->getPositionAtScatterer().x(), "mm"));
        double pos_y = static_cast<double>(Units::convert(track->getPositionAtScatterer().y(), "mm"));
        double kink_x = static_cast<double>(Units::convert(track->getKinkAtScatterer().x(), "mrad"));
        double kink_y = static_cast<double>(Units::convert(track->getKinkAtScatterer().y(), "mrad"));

        sixletKinkX->Fill(kink_x);
        sixletKinkY->Fill(kink_y);

        MBIpreview->Fill(pos_x, pos_y, kink_x * kink_x);
        MBIpreview->Fill(pos_x, pos_y, kink_y * kink_y);

        if(write_3D) {
            MBI3D->Fill(pos_x, pos_y, kink_x);
            MBI3D->Fill(pos_x, pos_y, kink_y);
        }

        int cell_x = int((pos_x + image_size.x() / 2) / cell_size.x());
        int cell_y = int((pos_y + image_size.y() / 2) / cell_size.y());
        LOG(DEBUG) << "Sixlet is in cell " << cell_x << " " << cell_y;

        if(cell_x < 0 || cell_x >= n_cells_x) {
            LOG(DEBUG) << "Cell out of boundaries in x. cell=" << cell_x << "   n_cells_x=" << n_cells_x;
            continue;
        }
        if(cell_y < 0 || cell_y >= n_cells_y) {
            LOG(DEBUG) << "Cell out of boundaries in y. cell=" << cell_y << "   n_cells_y=" << n_cells_y;
            continue;
        }

        // FIXME: In-situ sorting by checking where to put the element?
        int filled_angles = 0;
        if(fabs(kink_x) < static_cast<double>(Units::convert(angle_cut, "mrad"))) {
            m_all_kinks.at(std::make_pair(cell_x, cell_y)).push_back(kink_x);
            m_all_entries.at(std::make_pair(cell_x, cell_y))++;
            ++filled_angles;
        }
        if(fabs(kink_y) < static_cast<double>(Units::convert(angle_cut, "mrad"))) {
            m_all_kinks.at(std::make_pair(cell_x, cell_y)).push_back(kink_y);
            m_all_entries.at(std::make_pair(cell_x, cell_y))++;
            ++filled_angles;
        }

        m_all_sum.at(std::make_pair(cell_x, cell_y)) += (kink_x + kink_y);
        int entries = m_all_entries.at(std::make_pair(cell_x, cell_y));

        // Calculate AAD and set image value
        if(entries >= min_cell_content) {
            MBI->SetBinContent(cell_x, cell_y, getAAD(cell_x, cell_y));
            meanAngles->SetBinContent(cell_x,
                                      cell_y,
                                      m_all_sum.at(std::make_pair(cell_x, cell_y)) /
                                          m_all_entries.at(std::make_pair(cell_x, cell_y)));
        }
        entriesPerCell->SetBinContent(entries - filled_angles, entriesPerCell->GetBinContent(entries - filled_angles) - 1);
        entriesPerCell->SetBinContent(entries, entriesPerCell->GetBinContent(entries) + 1);
    }

    m_eventNumber++;

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisMaterialBudget::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
