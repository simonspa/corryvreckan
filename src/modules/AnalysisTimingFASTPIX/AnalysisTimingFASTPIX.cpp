/**
 * @file
 * @brief Implementation of module AnalysisTimingFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisTimingFASTPIX.h"
#include "objects/Waveform.hpp"

using namespace corryvreckan;

struct Cfd {
    double e1, e2, min, cfd_pos;
};

std::vector<Cfd> find_npeaks(const Waveform::waveform_t& w, double th, double frac);

std::vector<Cfd> find_npeaks(const Waveform::waveform_t& w, double th, double frac) {
    std::vector<size_t> redge;
    std::vector<size_t> fedge;

    for(size_t i = 0; i < w.waveform.size() - 1; i++) {
        if(w.waveform[i] <= th && w.waveform[i + 1] > th) {
            fedge.push_back(i);
        }

        if(w.waveform[i] >= th && w.waveform[i + 1] < th) {
            redge.push_back(i);
        }
    }

    std::vector<std::pair<size_t, size_t>> edges;

    if(redge.size() != fedge.size()) { // FIXME
        size_t size = std::min(redge.size(), fedge.size());
        redge.resize(size);
        fedge.resize(size);
    }

    std::transform(redge.begin(), redge.end(), fedge.begin(), std::back_inserter(edges), [](const auto a, const auto b) {
        return std::make_pair(a, b);
    });

    std::vector<Cfd> out;

    for(auto& i : edges) {
        double min = w.waveform[i.first];
        size_t min_pos = i.first;

        for(size_t j = i.first; j <= i.second; j++) {
            if(w.waveform[j] < min) {
                min = w.waveform[j];
                min_pos = j;
            }
        }

        size_t cfd_pos = i.first;

        for(size_t j = i.first; j <= min_pos - 1; j++) {
            if(w.waveform[j] >= min * frac && w.waveform[j + 1] < min * frac) {
                cfd_pos = j;
                break;
            }
        }

        double e1 = w.x0 + w.dx * (static_cast<double>(i.first) +
                                   (th - w.waveform[i.first]) / (w.waveform[i.first + 1] - w.waveform[i.first]));
        double e2 = w.x0 + w.dx * (static_cast<double>(i.second) +
                                   (th - w.waveform[i.second]) / (w.waveform[i.second + 1] - w.waveform[i.second]));
        double cfd = w.x0 + w.dx * (static_cast<double>(cfd_pos) +
                                    (min * frac - w.waveform[cfd_pos]) / (w.waveform[cfd_pos + 1] - w.waveform[cfd_pos]));

        out.emplace_back(Cfd{e1, e2, min, cfd});
    }

    return out;
}

std::vector<std::pair<double, double>> find_edges(const Waveform::waveform_t& w, double th);

std::vector<std::pair<double, double>> find_edges(const Waveform::waveform_t& w, double th) {
    std::vector<double> redge;
    std::vector<double> fedge;

    for(size_t i = 0; i < w.waveform.size() - 1; i++) {
        if(w.waveform[i] <= th && w.waveform[i + 1] > th) {
            redge.push_back(w.x0 +
                            w.dx * (static_cast<double>(i) + (th - w.waveform[i]) / (w.waveform[i + 1] - w.waveform[i])));
        }

        if(w.waveform[i] >= th && w.waveform[i + 1] < th) {
            fedge.push_back(w.x0 +
                            w.dx * (static_cast<double>(i) + (th - w.waveform[i]) / (w.waveform[i + 1] - w.waveform[i])));
        }
    }

    std::vector<std::pair<double, double>> out;

    if(redge.size() != fedge.size()) { // FIXME
        size_t size = std::min(redge.size(), fedge.size());
        redge.resize(size);
        fedge.resize(size);
    }

    std::transform(redge.begin(), redge.end(), fedge.begin(), std::back_inserter(out), [](const auto a, const auto b) {
        return std::make_pair(a, b);
    });

    return out;
}

// Histogram consisting of regular triangles, covering a hexagon in 6*n² triangles
template <typename T> void triangle_hist(double pitch, T* profile, size_t n) {
    std::vector<Double_t> x_coords(2 * n + 1);
    std::vector<Double_t> y_coords(4 * n + 1);

    double height = 2. / std::sqrt(3) * pitch;

    double px = pitch / (static_cast<double>(x_coords.size()) - 1);
    double py = height / (static_cast<double>(y_coords.size()) - 1);

    for(size_t x = 0; x < x_coords.size(); x++) {
        x_coords[x] = static_cast<double>(x) * px - pitch / 2;
    }

    for(size_t y = 0; y < y_coords.size(); y++) {
        y_coords[y] = static_cast<double>(y) * py - height / 2;
    }

    Double_t c_x[3], c_y[3];

    for(size_t x = 0; x < x_coords.size() - 1; x++) {
        for(size_t y = 1; y < y_coords.size() - 1; y++) {
            if((y + x % 2 + n % 2) % 2 == 0) {
                c_x[0] = x_coords[x + 1];
                c_x[1] = x_coords[x];
                c_x[2] = x_coords[x + 1];

                c_y[0] = y_coords[y - 1];
                c_y[1] = y_coords[y];
                c_y[2] = y_coords[y + 1];

                profile->AddBin(3, c_x, c_y);
            } else {
                c_x[0] = x_coords[x];
                c_x[1] = x_coords[x];
                c_x[2] = x_coords[x + 1];

                c_y[0] = y_coords[y - 1];
                c_y[1] = y_coords[y + 1];
                c_y[2] = y_coords[y];

                profile->AddBin(3, c_x, c_y);
            }
        }
    }
}

template <typename T> Int_t AnalysisTimingFASTPIX::fillTriangle(T* hist, double x, double y, double val) {
    double px = pitch / (static_cast<double>(2 * triangle_bins_ + 1) - 1);
    double py = height / (static_cast<double>(4 * triangle_bins_ + 1) - 1);

    int bin_x = static_cast<int>((x + pitch / 2.0) / px);
    int bin_y = static_cast<int>((y + height / 2.0) / py);

    x += pitch / 2.0;
    y += height / 2.0;

    // TODO: test for different n, add overflow bins
    if((bin_x % 2 + bin_y % 2) % 2 == 0) {
        if(py * (bin_y + 1 - (x / px - bin_x)) > y) {
            bin_y--;
        }
    } else {
        if(py * (bin_y + (x / px - bin_x)) > y) {
            bin_y--;
        }
    }

    int bin = bin_x * (4 * static_cast<int>(triangle_bins_) - 1) + bin_y + 1;
    // hist->AddBinContent(bin, val); // Does not work for TH2Poly, TProfile2Poly
    // hist->SetBinContent(bin, hist->GetBinContent(bin)+val); // Does not work for TProfile2Poly

    // Calculate center of bin and fill manually...
    double bx = bin_x * px + px / 2. - pitch / 2.0;
    double by = bin_y * py + py - height / 2.0;

    int i = hist->Fill(bx, by, val);

    if(i < 0 && !std::is_same<T, TProfile2Poly>::value) {
        LOG(INFO) << "Unbinned entry in " << hist->GetName() << " bin: " << i << " x: " << (x - pitch / 2.0)
                  << " y: " << (y - height / 2.0) << " bx: " << bx << " by: " << by << " bin_x: " << bin_x
                  << " bin_y: " << bin_y << " bin: " << bin;
    }

    return i;
    // return bin;
}

bool AnalysisTimingFASTPIX::inRoi(PositionVector3D<Cartesian3D<double>> p) {
    auto hex = m_detector->getInterceptPixel(p);
    auto pixels = m_detector->nPixels();

    int col = hex.first;
    int row = hex.second;

    col = col + (row - (row & 1)) / 2;

    return col > 0 && col < pixels.X() - 1 && row > 0 && row < pixels.Y() - 1;
}

AnalysisTimingFASTPIX::AnalysisTimingFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {}

void AnalysisTimingFASTPIX::initialize() {

    config_.setDefault<double>("chi2ndof_cut", 3.);
    chi2_ndof_cut_ = config_.get<double>("chi2ndof_cut");

    config_.setDefault<size_t>("triangle_bins", 15);
    triangle_bins_ = config_.get<size_t>("triangle_bins");

    // Initialise member variables
    tree = new TTree("tree", "");
    tree->Branch("pixel_col", &pixel_col_);
    tree->Branch("pixel_row", &pixel_row_);
    tree->Branch("tot", &tot_);
    tree->Branch("dt", &dt_);
    tree->Branch("seed_dist", &seed_dist_);
    tree->Branch("track_x", &track_x_);
    tree->Branch("track_y", &track_y_);
    tree->Branch("track_x_inpix", &track_x_inpix_);
    tree->Branch("track_y_inpix", &track_y_inpix_);

    m_eventNumber = 0;

    pitch = m_detector->getPitch().X();
    height = 2. / std::sqrt(3) * pitch;

    // Convert to um
    pitch *= 1000.0;
    height *= 1000.0;

    timewalk = new TH2F("timewalk", "timewalk (all pixels);ToT [ns];#Deltat [ns];", 700, -0.5, 350.5, 200, -20, 0);
    timewalk_inner =
        new TH2F("timewalk_inner", "timewalk (inner pixels);ToT [ns];#Deltat [ns];", 700, -0.5, 350.5, 200, -20, 0);
    timewalk_outer =
        new TH2F("timewalk_outer", "timewalk (outer pixels);ToT [ns];#Deltat [ns];", 700, -0.5, 350.5, 200, -20, 0);

    dt_hist = new TH1F("dt", "#Deltat (all pixels);#Deltat [ns];", 1000, -50, 50);
    dt_inner = new TH1F("dt_inner", "#Deltat (inner pixels);#Deltat [ns];", 1000, -50, 50);
    dt_outer = new TH1F("dt_outer", "#Deltat (outer pixels);#Deltat [ns];", 1000, -50, 50);
    seedDistance = new TH1F("seedDistance", "seed distance", 100, 0, 100);
    seedStatus = new TH1F("seedStatus", "seed status", 2, 0, 1);
    seedStatus->SetCanExtend(TH1::kAllAxes);

    mcp_amp = new TH1F("mcp_amp", ";Max. Amplitude [V];", 1000, 0, 5);
    cfd_amp = new TH1F("cfd_amp", ";Max. Amplitude [V];", 1000, 0, 5);
    cfd_peaks = new TH1F("cfd_peaks", "Peaks", 10, -0.5, 9.5);

    hitmapLocal =
        new TH2F("hitmapLocal", "hitmap;x_{track} [#mum];y_{track} [#mum]", 200, -250.5, 249.5, 200, -250.5, 249.5);

    hitmapLocalInner =
        new TH2F("hitmapLocalInner", "hitmap;x_{track} [#mum];y_{track} [#mum]", 200, -250.5, 249.5, 200, -250.5, 249.5);

    hitmapLocalOuter =
        new TH2F("hitmapLocalOuter", "hitmap;x_{track} [#mum];y_{track} [#mum]", 200, -250.5, 249.5, 200, -250.5, 249.5);

    dtMap = new TProfile2D("dtMap", "hitmap;x_{track} [#mum];y_{track} [#mum]", 200, -250.5, 249.5, 200, -250.5, 249.5, "s");

    std::string title;
    std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

    title = "Mean #Deltat map;" + mod_axes + "<#Deltat> [ns]";
    dt_inpix = new TProfile2Poly();
    dt_inpix->SetName("dt_inpix");
    dt_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, dt_inpix, triangle_bins_);

    dt_inpix->SetErrorOption(kERRORSPREAD);
}

StatusCode AnalysisTimingFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto waveforms = clipboard->getData<Waveform>("MCP_0");
    auto tracks = clipboard->getData<Track>();
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());

    LOG(DEBUG) << "====== Event: " << m_eventNumber << " ======";
    LOG(DEBUG) << "Waveforms: " << waveforms.size() << " Pixels: " << pixels.size() << " Tracks: " << tracks.size();

    if(waveforms.size() == 2 && pixels.size() > 0) {
        auto mcp_waveform = waveforms[1]->waveform();

        auto ch1 = find_edges(waveforms[0]->waveform(), 0.1);
        auto cfd = find_npeaks(mcp_waveform, -0.15, 0.2);

        mcp_amp->Fill(std::fabs(*std::min_element(mcp_waveform.waveform.begin(), mcp_waveform.waveform.end())));

        LOG(DEBUG) << "Peaks: " << cfd.size();
        cfd_peaks->Fill(static_cast<double>(cfd.size()));

        if(cfd.size() == 1 && ch1.size() > 0) {
            cfd_amp->Fill(std::fabs(cfd[0].min));

            for(const auto& track : tracks) {
                if(track->getChi2ndof() > chi2_ndof_cut_) {
                    LOG(DEBUG) << " - track discarded due to Chi2/ndof";
                    continue;
                }

                auto fp_seed = pixels[0];

                LOG(DEBUG) << "Clusters: " << track->getAssociatedClusters(m_detector->getName()).size();

                // Loop over all associated DUT clusters:
                for(auto assoc_cluster : track->getAssociatedClusters(m_detector->getName())) {
                    // use closest cluster
                    if(track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                        continue;
                    }

                    auto globalIntercept = m_detector->getIntercept(track.get());
                    auto localIntercept = m_detector->globalToLocal(globalIntercept);

                    // Calculate in-pixel position of track in microns
                    auto inpixel = m_detector->inPixel(localIntercept);
                    auto xmod_um = inpixel.X() * 1000.; // convert nm -> um
                    auto ymod_um = inpixel.Y() * 1000.; // convert nm -> um

                    auto seed = assoc_cluster->getSeedPixel();

                    if(seed->column() != fp_seed->column() || seed->row() != fp_seed->row()) {
                        seedStatus->Fill("unmatched", 1);
                        LOG(DEBUG) << "Unmatched seed pixel";
                        continue;
                    }
                    seedStatus->Fill("matched", 1);

                    auto position = m_detector->getLocalPosition(seed->column(), seed->row());

                    double xdist = std::fabs(localIntercept.X() - position.x());
                    double ydist = std::fabs(localIntercept.Y() - position.y());
                    double dist = std::sqrt(xdist * xdist + ydist * ydist) * 1000.0;

                    double tot = seed->charge();
                    double dt = cfd[0].cfd_pos - ch1.front().first;

                    int col = seed->column();
                    int row = seed->row();

                    if(std::fabs(dt) > 100.0) {
                        LOG(INFO) << "dt > 100";
                        continue;
                    }

                    col = col + (row - (row & 1)) / 2;
                    int idx = col + row * 16;

                    LOG(DEBUG) << " MCP: " << cfd[0].cfd_pos << " FP: " << ch1.front().first << " Dt: " << dt
                               << " Col: " << col << " Row: " << row << " ToT " << seed->charge() << " Idx: " << idx;
                    LOG(DEBUG) << "ToT: " << ch1.back().first - ch1.front().first;

                    pixel_col_ = col;
                    pixel_row_ = row;
                    tot_ = tot;
                    dt_ = dt;
                    seed_dist_ = dist;
                    track_x_ = localIntercept.X();
                    track_y_ = localIntercept.Y();
                    track_x_inpix_ = inpixel.X();
                    track_y_inpix_ = inpixel.Y();
                    tree->Fill();

                    timewalk->Fill(tot, dt);
                    dt_hist->Fill(dt);
                    seedDistance->Fill(dist);

                    if(inRoi(localIntercept)) {
                        fillTriangle(dt_inpix, xmod_um, ymod_um, dt);
                    }

                    hitmapLocal->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                    dtMap->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0, dt);

                    if(col == 0 || col == 15 || row == 0 || row == 3) {
                        timewalk_outer->Fill(tot, dt);
                        dt_outer->Fill(dt);
                        hitmapLocalOuter->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                    } else {
                        timewalk_inner->Fill(tot, dt);
                        dt_inner->Fill(dt);
                        hitmapLocalInner->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                    }
                }
            }
        }
    }

    m_eventNumber++;
    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisTimingFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    dt_inpix->Write();
    dtMap->ProjectionXY("resolutionMap", "C=E");
}
