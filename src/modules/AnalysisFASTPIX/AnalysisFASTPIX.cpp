/**
 * @file
 * @brief Implementation of module AnalysisFASTPIX
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisFASTPIX.h"
#include "objects/SpidrSignal.hpp"

using namespace corryvreckan;

AnalysisFASTPIX::AnalysisFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {

    config_.setDefault<double>("time_cut_frameedge", Units::get<double>(20, "ns"));
    config_.setDefault<double>("time_cut_deadtime", Units::get<double>(5, "us"));
    config_.setDefault<double>("chi2ndof_cut", 3.);
    config_.setDefault<bool>("use_closest_cluster", true);

    time_cut_frameedge_ = config_.get<double>("time_cut_frameedge");
    time_cut_deadtime_ = config_.get<double>("time_cut_deadtime");
    chi2_ndof_cut_ = config_.get<double>("chi2ndof_cut");
    use_closest_cluster_ = config_.get<bool>("use_closest_cluster");
}

template<typename T>
void split(Double_t *x, Double_t *y, T *profile, int d) {
    if(d == 0) {
        profile->AddBin(3, x, y);
    } else {
        Double_t x_[3], y_[3];
        Double_t a[3], b[3];

        a[0] = x[0] + (x[1]-x[0])/2.;
        a[1] = x[1] + (x[2]-x[1])/2.;
        a[2] = x[2] + (x[0]-x[2])/2.;

        b[0] = y[0] + (y[1]-y[0])/2.;
        b[1] = y[1] + (y[2]-y[1])/2.;
        b[2] = y[2] + (y[0]-y[2])/2.;


        x_[0] = x[0];
        x_[1] = a[0];
        x_[2] = a[2];

        y_[0] = y[0];
        y_[1] = b[0];
        y_[2] = b[2];

        split(x_, y_, profile, d-1);


        x_[0] = a[0];
        x_[1] = x[1];
        x_[2] = a[1];

        y_[0] = b[0];
        y_[1] = y[1];
        y_[2] = b[1];

        split(x_, y_, profile, d-1);


        x_[0] = a[2];
        x_[1] = a[1];
        x_[2] = x[2];

        y_[0] = b[2];
        y_[1] = b[1];
        y_[2] = y[2];

        split(x_, y_, profile, d-1);


        x_[0] = a[0];
        x_[1] = a[1];
        x_[2] = a[2];

        y_[0] = b[0];
        y_[1] = b[1];
        y_[2] = b[2];

        split(x_, y_, profile, d-1);
    }
}

// Split triangle into n² equal triangles
template<typename T>
void split2(Double_t *x, Double_t *y, T *profile, int n) {
    Double_t x_[3], y_[3];

    Double_t du_x = (x[1] - x[0]) / n;
    Double_t du_y = (y[1] - y[0]) / n;

    Double_t dv_x = (x[2] - x[0]) / n;
    Double_t dv_y = (y[2] - y[0]) / n;

    for(int i=0; i<n; i++) {
        for(int j=0; j<n-i; j++) {
            x_[0] = x[0] + i * dv_x + j * du_x;
            x_[1] = x[0] + i * dv_x + (j+1) * du_x;
            x_[2] = x[0] + (i+1) * dv_x + j * du_x;

            y_[0] = y[0] + i * dv_y + j * du_y;
            y_[1] = y[0] + i * dv_y + (j+1) * du_y;
            y_[2] = y[0] + (i+1) * dv_y + j * du_y;

            profile->AddBin(3, x_, y_);

            if(j<n-i-1) {
                x_[0] = x_[2];
                x_[2] = x[0] + (i+1) * dv_x + (j+1) * du_x;

                y_[0] = y_[2];
                y_[2] = y[0] + (i+1) * dv_y + (j+1) * du_y;

                profile->AddBin(3, x_, y_);
            }
        }
    }

}

// 6*n² equal triangles
template<typename T>
void hexagon(double pitch, T *profile, int n) {
    Double_t x[6], y[6];
    Double_t x_[3], y_[3];

    double height = 2./std::sqrt(3) * pitch;

    x[0] = 0;
    x[1] = pitch/2.;
    x[2] = pitch/2.;
    x[3] = 0;
    x[4] = -pitch/2;
    x[5] = -pitch/2;

    y[0] = -height/2;
    y[1] = -height/4;
    y[2] = height/4;
    y[3] = height/2;
    y[4] = height/4;
    y[5] = -height/4;

    for(int i=0; i<6; i++) {
        x_[0] = x[i];
        x_[1] = x[(i+1)%6];
        x_[2] = 0;

        y_[0] = y[i];
        y_[1] = y[(i+1)%6];
        y_[2] = 0;

        split2(x_, y_, profile, n);
    }
}

void AnalysisFASTPIX::initialize() {
        double pitch = m_detector->getPitch().X() * 1000.0;
        double height = 2./std::sqrt(3) * pitch;

        std::string mod_axes = "in-pixel x_{track} [#mum];in-pixel y_{track} [#mum];";

        hitmapLocal =
            new TH2F("hitmapLocal",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);
        hitmapLocalIntercept =
            new TH2F("hitmapLocalIntercept",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);
        hitmapLocalNoIntercept =
            new TH2F("hitmapLocalNoIntercept",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);
        hitmapLocalTrigger =
            new TH2F("hitmapLocalTrigger",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);
        hitmapLocalTimecuts =
            new TH2F("hitmapLocalTimecuts",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);
        hitmapLocalAssoc =
            new TH2F("hitmapLocalAssoc",
                     "hitmap;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);

        clusterSizeMap =
            new TProfile2D("clusterSizeMap",
                     "Mean cluster size map;x_{track} [#mum];y_{track} [#mum];<pixels/cluster>",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);

        clusterSizeMap_intercept =
            new TProfile2D("clusterSizeMap_intercept",
                     "Mean cluster size map;x_{track} [#mum];y_{track} [#mum];<pixels/cluster>",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);


        std::string title;
        title = "Mean cluster size map;" + mod_axes + "<pixels/cluster>";

        clusterSizeMap_inpix =
            new TProfile2D("clusterSizeMap_inpix",
                     title.c_str(),
                     40,
                     -pitch/2.0,
                     pitch/2.0,
                     40,
                     -height/2.0,
                     height/2.0);

        seedChargeMap =
            new TProfile2D("seedChargeMap",
                     "seed charge;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);

        seedChargeMap_inpix =
            new TProfile2D("seedChargeMap_inpix",
                     "seed charge;x_{track} [#mum];y_{track} [#mum]",
                     40,
                     -pitch/2.0,
                     pitch/2.0,
                     40,
                     -height/2.0,
                     height/2.0);

        clusterChargeMap =
            new TProfile2D("clusterChargeMap",
                     "cluster charge;x_{track} [#mum];y_{track} [#mum]",
                     200,
                     -250.5,
                     249.5,
                     200,
                     -250.5,
                     249.5);

        clusterChargeMap_inpix =
            new TProfile2D("clusterChargeMap_inpix",
                     "cluster charge;x_{track} [#mum];y_{track} [#mum]",
                     40,
                     -pitch/2.0,
                     pitch/2.0,
                     40,
                     -height/2.0,
                     height/2.0);

        binningIneff_inpix =
            new TH2F("binningIneff_inpix",
                     "binning;x_{track} [#mum];y_{track} [#mum]",
                     400,
                     -pitch/2.0,
                     pitch/2.0,
                     400,
                     -height/2.0,
                     height/2.0);

        title = "Mean cluster size map;" + mod_axes + "<pixels/cluster>";
        clusterSizeMap_inpix3 = new TProfile2Poly();
        clusterSizeMap_inpix3->SetName("clusterSizeMap_inpix3");
        clusterSizeMap_inpix3->SetTitle(title.c_str());
        hexagon(pitch, clusterSizeMap_inpix3, 15);

        clusterChargeMap_inpix3 = new TProfile2Poly();
        clusterChargeMap_inpix3->SetName("clusterChargeMap_inpix3");
        clusterChargeMap_inpix3->SetTitle("cluster charge");
        hexagon(pitch, clusterChargeMap_inpix3, 15);

        seedChargeMap_inpix3 = new TProfile2Poly();
        seedChargeMap_inpix3->SetName("seedChargeMap_inpix3");
        seedChargeMap_inpix3->SetTitle("seed charge");
        hexagon(pitch, seedChargeMap_inpix3, 15);

        hitmapTrigger_inpix3 = new TH2Poly();
        hitmapTrigger_inpix3->SetName("hitmapTrigger_inpix3");
        hitmapTrigger_inpix3->SetTitle("hitmap");
        hexagon(pitch, hitmapTrigger_inpix3, 15);

        hitmapTimecuts_inpix3 = new TH2Poly();
        hitmapTimecuts_inpix3->SetName("hitmapTimecuts_inpix3");
        hitmapTimecuts_inpix3->SetTitle("hitmap");
        hexagon(pitch, hitmapTimecuts_inpix3, 15);

        hitmapAssoc_inpix3 = new TH2Poly();
        hitmapAssoc_inpix3->SetName("hitmapAssoc_inpix3");
        hitmapAssoc_inpix3->SetTitle("hitmap");
        hexagon(pitch, hitmapAssoc_inpix3, 15);
}

StatusCode AnalysisFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    // get the TDC trigger
    std::shared_ptr<Detector> reference = get_reference();
    auto referenceSpidrSignals = clipboard->getData<SpidrSignal>(reference->getName());

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();
    auto event = clipboard->getEvent();
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());

    // Loop over all tracks
    for(auto& track : tracks) {
        auto globalIntercept = m_detector->getIntercept(track.get());
        auto localIntercept = m_detector->globalToLocal(globalIntercept);

        // Calculate in-pixel position of track in microns
        auto inpixel = m_detector->inPixel(localIntercept);
        auto xmod_um = inpixel.X() * 1000.; // convert nm -> um
        auto ymod_um = inpixel.Y() * 1000.; // convert nm -> um

        // Cut on the chi2/ndof
        if(track->getChi2ndof() > chi2_ndof_cut_) {
            continue;
        }

        hitmapLocal->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
        if(m_detector->hasIntercept(track.get())) {
            hitmapLocalIntercept->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
        } else {
            hitmapLocalNoIntercept->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
        }

        if(fabs(track->timestamp() - event->end()) < time_cut_frameedge_ &&
            fabs(track->timestamp() - event->start()) < time_cut_frameedge_ && 
            fabs(track->timestamp() - last_timestamp) < time_cut_deadtime_) {
            
            continue;
        }

        hitmapLocalTimecuts->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
        if(localIntercept.X() >= -0.150 && localIntercept.X() <= 0.140 && localIntercept.Y() >= -0.040 && localIntercept.Y() <= 0.020) {
            hitmapTimecuts_inpix3->Fill(xmod_um, ymod_um);
        }


        if(!referenceSpidrSignals.empty() || !pixels.empty()) {
            hitmapLocalTrigger->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);

            if(localIntercept.X() >= -0.150 && localIntercept.X() <= 0.140 && localIntercept.Y() >= -0.040 && localIntercept.Y() <= 0.020) {
                hitmapTrigger_inpix3->Fill(xmod_um, ymod_um);
            }
        }

        for(auto assoc_cluster : track->getAssociatedClusters(m_detector->getName())) {
            // if closest cluster should be used continue if current associated cluster is not the closest one
            if(use_closest_cluster_ && track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                continue;
            }

            //TODO: only fill once per track in any case
            hitmapLocalAssoc->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);

            clusterSizeMap->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0, static_cast<double>(assoc_cluster->size()));
            clusterChargeMap->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0, assoc_cluster->charge());
            seedChargeMap->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0, assoc_cluster->getSeedPixel()->charge());

            if(localIntercept.X() >= -0.150 && localIntercept.X() <= 0.140 && localIntercept.Y() >= -0.040 && localIntercept.Y() <= 0.020) {
                clusterSizeMap_intercept->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0, static_cast<double>(assoc_cluster->size()));

                clusterSizeMap_inpix->Fill(xmod_um, ymod_um, static_cast<double>(assoc_cluster->size()));
                clusterChargeMap_inpix->Fill(xmod_um, ymod_um, assoc_cluster->charge());
                seedChargeMap_inpix->Fill(xmod_um, ymod_um, assoc_cluster->getSeedPixel()->charge());

                clusterSizeMap_inpix3->Fill(xmod_um, ymod_um, static_cast<double>(assoc_cluster->size()));
                clusterChargeMap_inpix3->Fill(xmod_um, ymod_um, assoc_cluster->charge());
                seedChargeMap_inpix3->Fill(xmod_um, ymod_um, assoc_cluster->getSeedPixel()->charge());

                int i = hitmapAssoc_inpix3->Fill(xmod_um, ymod_um);

                if(i < 1) {
                    LOG(INFO) << "bin: " << i << " x: " << xmod_um << " y: " << ymod_um;
                    binningIneff_inpix->Fill(xmod_um, ymod_um);
                }
            }
        }
    }

    for(auto& refSpidrSignal : referenceSpidrSignals) { // FIXME?
        if(last_timestamp < refSpidrSignal->timestamp()) {
            last_timestamp = refSpidrSignal->timestamp();
        }
    }

    for(auto& pixel : pixels) {
        if(last_timestamp < pixel->timestamp()) {
            last_timestamp = pixel->timestamp();
        }     
    }

    return StatusCode::Success;
}

void AnalysisFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {
    clusterSizeMap_inpix3->Write();
    clusterChargeMap_inpix3->Write();
    seedChargeMap_inpix3->Write();

    hitmapTrigger_inpix3->Write();
    hitmapTimecuts_inpix3->Write();
    hitmapAssoc_inpix3->Write();

    LOG(INFO) << "clusterSizeMap_inpix3 unbinned: " << clusterSizeMap_inpix3->GetBinEntries(-5);
}
