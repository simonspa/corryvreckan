/**
 * @file
 * @brief Implementation of module AnalysisFASTPIX
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisFASTPIX.h"
#include <TEfficiency.h>
#include "objects/SpidrSignal.hpp"

using namespace corryvreckan;

AnalysisFASTPIX::AnalysisFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    config_.setDefault<double>("time_cut_deadtime", Units::get<double>(5, "us"));
    config_.setDefault<double>("time_cut_trigger", Units::get<double>(250, "ns"));
    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<double>("roi_margin_x", 1.0);
    config_.setDefault<double>("roi_margin_y", 0.5);
    config_.setDefault<bool>("use_closest_cluster", true);
    config_.setDefault<size_t>("triangle_bins", 15);
    config_.setDefault<double>("bin_size", Units::get<double>(2.5, "um"));
    config_.setDefault<double>("hist_scale", 1.75);
    config_.setDefault<bool>("roi_inner", true);

    roi_margin_x_ = config_.get<double>("roi_margin_x");
    roi_margin_y_ = config_.get<double>("roi_margin_y");

    auto size = m_detector->getSize();
    pitch = m_detector->getPitch().X();
    height = 2. / std::sqrt(3) * pitch;

    // Cut off roi_margin pixels around edge of matrix
    config_.setDefault<ROOT::Math::XYVector>(
        "roi_min", {-size.X() / 2. + pitch * roi_margin_x_, -size.Y() / 2. + height * roi_margin_y_});
    config_.setDefault<ROOT::Math::XYVector>(
        "roi_max", {size.X() / 2. - pitch * roi_margin_x_, size.Y() / 2. - height * roi_margin_y_});

    // Convert to um
    pitch *= 1000.0;
    height *= 1000.0;

    time_cut_frameedge_ = config_.get<double>("time_cut_frameedge");
    time_cut_deadtime_ = config_.get<double>("time_cut_deadtime");
    time_cut_trigger_ = config_.get<double>("time_cut_trigger");
    chi2_ndof_cut_ = config_.get<double>("chi2ndof_cut");
    use_closest_cluster_ = config_.get<bool>("use_closest_cluster");
    roi_min = config.get<ROOT::Math::XYVector>("roi_min");
    roi_max = config.get<ROOT::Math::XYVector>("roi_max");
    triangle_bins_ = config.get<size_t>("triangle_bins");
    bin_size_ = config.get<double>("bin_size");
    hist_scale_ = config.get<double>("hist_scale");
    roi_inner_ = config.get<bool>("roi_inner");
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

void AnalysisFASTPIX::initialize() {
    std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

    auto size = m_detector->getSize();

    double h_width = size.X() * hist_scale_;
    double h_height = size.Y() * hist_scale_;
    int bins_x = static_cast<int>(h_width / bin_size_);
    int bins_y = static_cast<int>(h_height / bin_size_);
    h_width *= 1000.0;
    h_height *= 1000.0;

    hitmap = new TH2F("hitmap",
                      "hitmap;x_{track} [#mum];y_{track} [#mum]",
                      bins_x,
                      -h_width / 2.0,
                      h_width / 2.0,
                      bins_y,
                      -h_height / 2.0,
                      h_height / 2.0);
    hitmapIntercept = new TH2F("hitmapIntercept",
                               "hitmap;x_{track} [#mum];y_{track} [#mum]",
                               bins_x,
                               -h_width / 2.0,
                               h_width / 2.0,
                               bins_y,
                               -h_height / 2.0,
                               h_height / 2.0);
    hitmapNoIntercept = new TH2F("hitmapNoIntercept",
                                 "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                 bins_x,
                                 -h_width / 2.0,
                                 h_width / 2.0,
                                 bins_y,
                                 -h_height / 2.0,
                                 h_height / 2.0);
    hitmapTimecuts = new TH2F("hitmapTimecuts",
                              "hitmap;x_{track} [#mum];y_{track} [#mum]",
                              bins_x,
                              -h_width / 2.0,
                              h_width / 2.0,
                              bins_y,
                              -h_height / 2.0,
                              h_height / 2.0);
    hitmapTrigger = new TH2F("hitmapTrigger",
                             "hitmap;x_{track} [#mum];y_{track} [#mum]",
                             bins_x,
                             -h_width / 2.0,
                             h_width / 2.0,
                             bins_y,
                             -h_height / 2.0,
                             h_height / 2.0);
    hitmapNoTrigger = new TH2F("hitmapNoTrigger",
                               "hitmap;x_{track} [#mum];y_{track} [#mum]",
                               bins_x,
                               -h_width / 2.0,
                               h_width / 2.0,
                               bins_y,
                               -h_height / 2.0,
                               h_height / 2.0);
    hitmapDeadtime = new TH2F("hitmapDeadtime",
                              "hitmap;x_{track} [#mum];y_{track} [#mum]",
                              bins_x,
                              -h_width / 2.0,
                              h_width / 2.0,
                              bins_y,
                              -h_height / 2.0,
                              h_height / 2.0);
    hitmapDeadtimeTrigger = new TH2F("hitmapDeadtimeTrigger",
                                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                     bins_x,
                                     -h_width / 2.0,
                                     h_width / 2.0,
                                     bins_y,
                                     -h_height / 2.0,
                                     h_height / 2.0);
    hitmapDeadtimeNoTrigger = new TH2F("hitmapDeadtimeNoTrigger",
                                       "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                       bins_x,
                                       -h_width / 2.0,
                                       h_width / 2.0,
                                       bins_y,
                                       -h_height / 2.0,
                                       h_height / 2.0);
    hitmapTriggerAssoc = new TH2F("hitmapTriggerAssoc",
                                  "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                  bins_x,
                                  -h_width / 2.0,
                                  h_width / 2.0,
                                  bins_y,
                                  -h_height / 2.0,
                                  h_height / 2.0);
    hitmapNoTriggerAssoc = new TH2F("hitmapNoTriggerAssoc",
                                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                    bins_x,
                                    -h_width / 2.0,
                                    h_width / 2.0,
                                    bins_y,
                                    -h_height / 2.0,
                                    h_height / 2.0);
    hitmapAssoc = new TH2F("hitmapAssoc",
                           "hitmap;x_{track} [#mum];y_{track} [#mum]",
                           bins_x,
                           -h_width / 2.0,
                           h_width / 2.0,
                           bins_y,
                           -h_height / 2.0,
                           h_height / 2.0);
    hitmapNoAssoc = new TH2F("hitmapNoAssoc",
                             "hitmap;x_{track} [#mum];y_{track} [#mum]",
                             bins_x,
                             -h_width / 2.0,
                             h_width / 2.0,
                             bins_y,
                             -h_height / 2.0,
                             h_height / 2.0);
    hitmapNoAssocMissing = new TH2F("hitmapNoAssocMissing",
                                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                                    bins_x,
                                    -h_width / 2.0,
                                    h_width / 2.0,
                                    bins_y,
                                    -h_height / 2.0,
                                    h_height / 2.0);

    clusterCharge = new TH1F("clusterCharge", "cluster charge;charge [ToT];count", 500, -0.5, 499.5);
    clusterChargeROI = new TH1F("clusterChargeROI", "cluster charge;charge [ToT];count", 500, -0.5, 499.5);

    clusterSize =
        new TH1F("clusterSize", "Cluster size (tracks after cuts), all tracks;cluster size;# entries", 19, 0.5, 19.5);
    clusterSizeROI =
        new TH1F("clusterSizeROI", "Cluster size (tracks after cuts), tracks in ROI;cluster size;# entries", 19, 0.5, 19.5);

    inefficientTriggerAssocTime = new TH1F(
        "inefficientTriggerAssocTime", "Inefficient tracks in ROI;track time - event start [ns];# entries", 100, 0, 10000);
    inefficientTriggerAssocDt = new TH1F(
        "inefficientTriggerAssocDt", "Inefficient tracks in ROI;track time - trigger time [ns];# entries", 100, -5000, 5000);
    inefficientAssocDt = new TH1F(
        "inefficientAssocDt", "Inefficient tracks in ROI;track time - trigger time [ns];# entries", 100, -5000, 5000);

    inefficientAssocDist =
        new TH1F("inefficientAssocDist", "Inefficient tracks in ROI;min(|cluster-track|) [#mum];# entries", 200, 0, 200);

    inefficientAssocEventStatus = new TH1F("inefficientAssocEventStatus", "event status;event status;count", 5, 0, 4);
    inefficientAssocEventStatus->SetCanExtend(TH1::kAllAxes);

    clusterSizeMap = new TProfile2D("clusterSizeMap",
                                    "Mean cluster size map;x_{track} [#mum];y_{track} [#mum];<pixels/cluster>",
                                    bins_x,
                                    -h_width / 2.0,
                                    h_width / 2.0,
                                    bins_y,
                                    -h_height / 2.0,
                                    h_height / 2.0);

    clusterSizeMapROI = new TProfile2D("clusterSizeMapROI",
                                       "Mean cluster size map;x_{track} [#mum];y_{track} [#mum];<pixels/cluster>",
                                       bins_x,
                                       -h_width / 2.0,
                                       h_width / 2.0,
                                       bins_y,
                                       -h_height / 2.0,
                                       h_height / 2.0);

    std::string title;
    title = "Mean cluster size map;" + mod_axes + "<pixels/cluster>";

    seedChargeMap = new TProfile2D("seedChargeMap",
                                   "seed charge;x_{track} [#mum];y_{track} [#mum]",
                                   bins_x,
                                   -h_width / 2.0,
                                   h_width / 2.0,
                                   bins_y,
                                   -h_height / 2.0,
                                   h_height / 2.0);

    clusterChargeMap = new TProfile2D("clusterChargeMap",
                                      "cluster charge;x_{track} [#mum];y_{track} [#mum]",
                                      bins_x,
                                      -h_width / 2.0,
                                      h_width / 2.0,
                                      bins_y,
                                      -h_height / 2.0,
                                      h_height / 2.0);

    title = "Mean cluster size map;" + mod_axes + "<pixels/cluster>";
    clusterSizeMap_inpix = new TProfile2Poly();
    clusterSizeMap_inpix->SetName("clusterSizeMap_inpix");
    clusterSizeMap_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, clusterSizeMap_inpix, triangle_bins_);

    title = "Mean cluster charge map;" + mod_axes + "<cluster charge> [ToT]";
    clusterChargeMap_inpix = new TProfile2Poly();
    clusterChargeMap_inpix->SetName("clusterChargeMap_inpix");
    clusterChargeMap_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, clusterChargeMap_inpix, triangle_bins_);

    title = "Mean seed charge map;" + mod_axes + "<seed charge> [ToT]";
    seedChargeMap_inpix = new TProfile2Poly();
    seedChargeMap_inpix->SetName("seedChargeMap_inpix");
    seedChargeMap_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, seedChargeMap_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapTimecuts_inpix = new TH2Poly();
    hitmapTimecuts_inpix->SetName("hitmapTimecuts_inpix");
    hitmapTimecuts_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapTimecuts_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapTrigger_inpix = new TH2Poly();
    hitmapTrigger_inpix->SetName("hitmapTrigger_inpix");
    hitmapTrigger_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapTrigger_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapNoTrigger_inpix = new TH2Poly();
    hitmapNoTrigger_inpix->SetName("hitmapNoTrigger_inpix");
    hitmapNoTrigger_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapNoTrigger_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapDeadtime_inpix = new TH2Poly();
    hitmapDeadtime_inpix->SetName("hitmapDeadtime_inpix");
    hitmapDeadtime_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapDeadtime_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapDeadtimeTrigger_inpix = new TH2Poly();
    hitmapDeadtimeTrigger_inpix->SetName("hitmapDeadtimeTrigger_inpix");
    hitmapDeadtimeTrigger_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapDeadtimeTrigger_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapDeadtimeNoTrigger_inpix = new TH2Poly();
    hitmapDeadtimeNoTrigger_inpix->SetName("hitmapDeadtimeNoTrigger_inpix");
    hitmapDeadtimeNoTrigger_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapDeadtimeNoTrigger_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapTriggerAssoc_inpix = new TH2Poly();
    hitmapTriggerAssoc_inpix->SetName("hitmapTriggerAssoc_inpix");
    hitmapTriggerAssoc_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapTriggerAssoc_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapNoTriggerAssoc_inpix = new TH2Poly();
    hitmapNoTriggerAssoc_inpix->SetName("hitmapNoTriggerAssoc_inpix");
    hitmapNoTriggerAssoc_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapNoTriggerAssoc_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapAssoc_inpix = new TH2Poly();
    hitmapAssoc_inpix->SetName("hitmapAssoc_inpix");
    hitmapAssoc_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapAssoc_inpix, triangle_bins_);

    title = "hitmap;" + mod_axes;
    hitmapNoAssoc_inpix = new TH2Poly();
    hitmapNoAssoc_inpix->SetName("hitmapNoAssoc_inpix");
    hitmapNoAssoc_inpix->SetTitle(title.c_str());
    triangle_hist(pitch, hitmapNoAssoc_inpix, triangle_bins_);
}

bool AnalysisFASTPIX::inRoi(PositionVector3D<Cartesian3D<double>> p) {
    if(roi_inner_) {
        auto hex = m_detector->getInterceptPixel(p);
        auto pixels = m_detector->nPixels();

        int col = hex.first;
        int row = hex.second;

        col = col + (row - (row & 1)) / 2;

        return col > 0 && col < pixels.X() - 1 && row > 0 && row < pixels.Y() - 1;
    } else {
        return roi_min.X() <= p.X() && roi_min.Y() <= p.Y() && roi_max.X() >= p.X() && roi_max.Y() >= p.Y();
    }
}

Int_t AnalysisFASTPIX::fillTriangle(TH2* hist, double x, double y, double val) {
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

    // int bin = bin_x * (4 * static_cast<int>(triangle_bins_) - 1) + bin_y + 1;
    // hist->AddBinContent(bin, val); // Does not work for TH2Poly, TProfile2Poly
    // hist->SetBinContent(bin, hist->GetBinContent(bin)+val); // Does not work for TProfile2Poly

    // Calculate center of bin and fill manually...
    double bx = bin_x * px + px / 2. - pitch / 2.0;
    double by = bin_y * py + py - height / 2.0;

    return hist->Fill(bx, by, val);
}

int AnalysisFASTPIX::getFlags(std::shared_ptr<Event> event, size_t trigger) {
    auto tagList = event->tagList();
    auto status = tagList.find(std::string("fp_flags:") + std::to_string(trigger));
    int flags = 3;

    if(status != tagList.end()) {
        try {
            flags = std::stoi(status->second);
        } catch(const std::invalid_argument&) {
        }
    }

    return flags;
}

StatusCode AnalysisFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto event = clipboard->getEvent();

    // get the TDC trigger
    auto triggers = event->triggerList();
    // std::vector<std::pair<uint32_t, double>> referenceSpidrSignals(t.begin(), t.end());

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    auto clusters = clipboard->getData<Cluster>(m_detector->getName());

    // Loop over all tracks
    for(auto& track : tracks) {
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        auto x_um = localIntercept.X() * 1000.0;
        auto y_um = localIntercept.Y() * 1000.0;

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod_um = inpixel.X() * 1000.; // convert nm -> um
        auto ymod_um = inpixel.Y() * 1000.; // convert nm -> um

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > chi2_ndof_cut_) {
            continue;
        }

        hitmap->Fill(x_um, y_um);
        if(m_detector->hasIntercept(track.get())) {
            hitmapIntercept->Fill(x_um, y_um);
        } else {
            hitmapNoIntercept->Fill(x_um, y_um);
        }

        // Cut tracks that are close to the edges of the event or when the oscilloscope dead time extends from the previous
        // event
        if(fabs(track->timestamp() - event->end()) < time_cut_frameedge_ ||
           fabs(track->timestamp() - event->start()) < time_cut_frameedge_ ||
           track->timestamp() - last_timestamp < time_cut_deadtime_) {

            continue;
        }

        hitmapTimecuts->Fill(x_um, y_um);

        if(inRoi(localIntercept)) {
            fillTriangle(hitmapTimecuts_inpix, xmod_um, ymod_um);
        }

        // Tracks after timing cuts with SPIDR trigger in the same event
        if(!triggers.empty()) {
            hitmapTrigger->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapTrigger_inpix, xmod_um, ymod_um);
            }
        } else {
            hitmapNoTrigger->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapNoTrigger_inpix, xmod_um, ymod_um);
            }
        }

        // Cut on oscilloscpe dead time in the current event and reject tracks in the specified time window after a trigger
        bool deadtime = false;
        for(auto& trigger : triggers) {
            if(track->timestamp() - trigger.second > time_cut_trigger_ &&
               track->timestamp() - trigger.second < time_cut_deadtime_) {

                deadtime = true;
            }
        }

        if(deadtime) {
            continue;
        }

        // Tracks after deadtime cut
        hitmapDeadtime->Fill(x_um, y_um);

        if(inRoi(localIntercept)) {
            fillTriangle(hitmapDeadtime_inpix, xmod_um, ymod_um);
        }

        // Tracks after deadtime cut with SPIDR trigger in the same event
        if(!triggers.empty()) {
            hitmapDeadtimeTrigger->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapDeadtimeTrigger_inpix, xmod_um, ymod_um);
            }
        } else {
            hitmapDeadtimeNoTrigger->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapDeadtimeNoTrigger_inpix, xmod_um, ymod_um);
            }
        }

        // Find tracks inside of time_cut_trigger time window around SPIDR trigger

        bool triggerAssoc = false;
        for(auto& trigger : triggers) {
            if(fabs(track->timestamp() - trigger.second) < time_cut_trigger_) {
                triggerAssoc = true;
            }
        }

        if(triggerAssoc) {
            hitmapTriggerAssoc->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapTriggerAssoc_inpix, xmod_um, ymod_um);
            }
        } else {
            hitmapNoTriggerAssoc->Fill(x_um, y_um);

            if(inRoi(localIntercept)) {
                fillTriangle(hitmapNoTriggerAssoc_inpix, xmod_um, ymod_um);
                for(auto& trigger : triggers) {
                    inefficientTriggerAssocDt->Fill(track->timestamp() - trigger.second);
                }

                inefficientTriggerAssocTime->Fill(track->timestamp() - event->start());
            }
        }

        // Analyse tracks with associated clusters

        auto assoc_clusters = track->getAssociatedClusters(m_detector->getName());

        if(!assoc_clusters.empty()) {
            hitmapAssoc->Fill(x_um, y_um);
            if(inRoi(localIntercept)) {
                fillTriangle(hitmapAssoc_inpix, xmod_um, ymod_um);
            }
        } else {
            hitmapNoAssoc->Fill(x_um, y_um);
            if(inRoi(localIntercept)) {
                fillTriangle(hitmapNoAssoc_inpix, xmod_um, ymod_um);

                // Check decoder status for all triggers in event
                // This might include efficient triggers if there is more than one in the event
                for(auto& trigger : triggers) {
                    const char* labels[] = {"Complete", "Incomplete", "Undecoded", "Missing"};

                    int flags = getFlags(event, trigger.first);

                    if(flags == 3) {
                        LOG(INFO) << "Missing hit!";
                        hitmapNoAssocMissing->Fill(x_um, y_um);
                    }

                    inefficientAssocEventStatus->Fill(labels[flags], 1);
                }

                for(auto& trigger : triggers) {
                    inefficientAssocDt->Fill(track->timestamp() - trigger.second);
                }

                auto min_dist = std::numeric_limits<double>::infinity();

                for(auto& cluster : clusters) {

                    double xdist = std::fabs(localIntercept.X() - cluster->local().x());
                    double ydist = std::fabs(localIntercept.Y() - cluster->local().y());
                    double dist = std::sqrt(xdist * xdist + ydist * ydist);

                    if(dist < min_dist) {
                        min_dist = dist;
                    }
                }

                if(min_dist < std::numeric_limits<double>::infinity()) {
                    inefficientAssocDist->Fill(min_dist * 1000.0);
                }
            }
        }

        for(auto assoc_cluster : assoc_clusters) {
            // if closest cluster should be used continue if current associated cluster is not the closest one
            if(use_closest_cluster_ && track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                continue;
            }

            clusterSizeMap->Fill(x_um, y_um, static_cast<double>(assoc_cluster->size()));
            clusterChargeMap->Fill(x_um, y_um, assoc_cluster->charge());
            seedChargeMap->Fill(x_um, y_um, assoc_cluster->getSeedPixel()->charge());
            clusterSize->Fill(static_cast<double>(assoc_cluster->size()));
            clusterCharge->Fill(assoc_cluster->charge());

            if(inRoi(localIntercept)) {
                clusterSizeMapROI->Fill(x_um, y_um, static_cast<double>(assoc_cluster->size()));

                fillTriangle(clusterSizeMap_inpix, xmod_um, ymod_um, static_cast<double>(assoc_cluster->size()));
                fillTriangle(clusterChargeMap_inpix, xmod_um, ymod_um, assoc_cluster->charge());
                fillTriangle(seedChargeMap_inpix, xmod_um, ymod_um, assoc_cluster->getSeedPixel()->charge());

                clusterSizeROI->Fill(static_cast<double>(assoc_cluster->size()));
                clusterChargeROI->Fill(assoc_cluster->charge());
            }
        }
    }

    // Update with latest time stamp in case the oscilloscope dead time extends into the next event
    for(auto& trigger : triggers) {
        if(last_timestamp < trigger.second) {
            last_timestamp = trigger.second;
        }
    }

    m_currentEvent++;

    return StatusCode::Success;
}

void AnalysisFASTPIX::printEfficiency(int total_tracks, int matched_tracks) {
    double totalEff = 100 * static_cast<double>(matched_tracks) / (total_tracks > 0 ? total_tracks : 1);
    double lowerEffError = totalEff - 100 * (TEfficiency::ClopperPearson(total_tracks, matched_tracks, 0.683, false));
    double upperEffError = 100 * (TEfficiency::ClopperPearson(total_tracks, matched_tracks, 0.683, true)) - totalEff;
    LOG(STATUS) << "Efficiency: " << totalEff << "(+" << upperEffError << " -" << lowerEffError << ")%, measured with "
                << matched_tracks << "/" << total_tracks << " matched/total tracks";
}

void AnalysisFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    clusterSizeMap_inpix->Write();
    clusterChargeMap_inpix->Write();
    seedChargeMap_inpix->Write();

    hitmapTimecuts_inpix->Write();

    hitmapTrigger_inpix->Write();
    hitmapNoTrigger_inpix->Write();

    hitmapDeadtime_inpix->Write();
    hitmapDeadtimeTrigger_inpix->Write();
    hitmapDeadtimeNoTrigger_inpix->Write();

    hitmapTriggerAssoc_inpix->Write();
    hitmapNoTriggerAssoc_inpix->Write();

    hitmapAssoc_inpix->Write();
    hitmapNoAssoc_inpix->Write();

    // Efficiency

    /*TEfficiency efficiency(*hitmapTrigger, *hitmapTimecuts);
    efficiency.SetName("efficiency");
    efficiency.Write();

    TEfficiency efficiency_inpix(*hitmapTrigger_inpix, *hitmapTimecuts_inpix);
    efficiency_inpix.SetName("efficiency_inpix");
    efficiency_inpix.Write();

    TEfficiency efficiencyDeadtime(*hitmapDeadtime, *hitmapDeadtimeTrigger);
    efficiencyDeadtime.SetName("efficiencyDeadtime");
    efficiencyDeadtime.Write();

    TEfficiency efficiencyDeadtime_inpix(*hitmapDeadtime_inpix, *hitmapDeadtimeTrigger_inpix);
    efficiencyDeadtime_inpix.SetName("efficiencyDeadtime_inpix");
    efficiencyDeadtime_inpix.Write();

    TEfficiency efficiencyTriggerAssoc(*hitmapDeadtime, *hitmapTriggerAssoc);
    efficiencyTriggerAssoc.SetName("efficiencyTriggerAssoc");
    efficiencyTriggerAssoc.Write();

    TEfficiency efficiencyTriggerAssoc_inpix(*hitmapDeadtime_inpix, *hitmapTriggerAssoc_inpix);
    efficiencyTriggerAssoc_inpix.SetName("efficiencyTriggerAssoc_inpix");
    efficiencyTriggerAssoc_inpix.Write();

    TEfficiency efficiencyAssoc(*hitmapDeadtime, *hitmapAssoc);
    efficiencyAssoc.SetName("efficiencyAssoc");
    efficiencyAssoc.Write();

    TEfficiency efficiencyAssoc_inpix(*hitmapDeadtime_inpix, *hitmapAssoc_inpix);
    efficiencyAssoc_inpix.SetName("efficiencyAssoc_inpix");
    efficiencyAssoc_inpix.Write();*/

    printEfficiency(static_cast<int>(hitmapTimecuts_inpix->Integral()), static_cast<int>(hitmapTrigger_inpix->Integral()));
    printEfficiency(static_cast<int>(hitmapDeadtime_inpix->Integral()),
                    static_cast<int>(hitmapDeadtimeTrigger_inpix->Integral()));
    printEfficiency(static_cast<int>(hitmapDeadtime_inpix->Integral()),
                    static_cast<int>(hitmapTriggerAssoc_inpix->Integral()));
    printEfficiency(static_cast<int>(hitmapDeadtime_inpix->Integral()), static_cast<int>(hitmapAssoc_inpix->Integral()));
}
