/**
 * @file
 * @brief Implementation of module AnalysisTimingFASTPIX
 *
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "AnalysisTimingFASTPIX.h"
#include "objects/Waveform.hpp"
#include "objects/Timestamp.hpp"

using namespace corryvreckan;

double mean(const std::vector<double> &v) {
  double m = 0;

  for(double i : v) {
    m += i;
  }

  return m / v.size();
}

double rms(const std::vector<double> &v) {
  //double mean = std::accumulate(v.begin(), v.end(), 0)/v.size();

  double m = mean(v);

  /*double out = std::accumulate(v.begin(), v.end(), 0, [mean] (double a, double b) {
    return a+(mean-b)*(mean-b);
  });*/

  double out = 0;

  for(double i : v) {
    out += (m-i)*(m-i);
  }

  return std::sqrt(out/v.size());
}

double rms997(const std::vector<double> &v) {
  std::vector<double> sorted = v;
  std::sort(sorted.begin(), sorted.end());

  size_t idx_low = 0;
  size_t idx_high = sorted.size()-1;
  size_t points = sorted.size() *( 1.0 - 0.997);

  double m = mean(sorted);

  for(size_t i = 0; i < points; i++) {
    if(std::abs(sorted[idx_low]-m) > std::abs(sorted[idx_high]-m)) {
      idx_low++;
    } else {
      idx_high--;
    }
  }

  std::vector<double> out(sorted.begin()+idx_low, sorted.begin()+idx_high);

  return rms(out);
}

struct Cfd {
  double e1, e2, min, cfd_pos;
};

std::vector<Cfd> find_npeaks(const Waveform::waveform_t &w, double th, double frac) {
  std::vector<size_t> redge;
  std::vector<size_t> fedge;

  for(size_t i = 0; i<w.waveform.size()-1; i++) {
    if(w.waveform[i]<=th && w.waveform[i+1]>th) {
      fedge.push_back(i);
    }

    if(w.waveform[i]>=th && w.waveform[i+1]<th) {
      redge.push_back(i);
    }
  }

  std::vector<std::pair<size_t, size_t>> edges;

  if(redge.size() != fedge.size()) { // FIXME
    size_t size = std::min(redge.size(), fedge.size());
    redge.resize(size);
    fedge.resize(size);
  }

  std::transform(redge.begin(),redge.end(), fedge.begin(), std::back_inserter(edges),
    [](const auto a, const auto b) {
        return std::make_pair(a, b);
    });

  std::vector<Cfd> out;
  
  for(auto &i : edges) {
    double min = w.waveform[i.first];
    size_t min_pos = i.first;

    for(size_t j=i.first;j<=i.second;j++) {
      if(w.waveform[j]<min) {
        min = w.waveform[j];
        min_pos = j;
      }
    }

    size_t cfd_pos = i.first;

    for(size_t j=i.first;j<=min_pos-1;j++) {
      if(w.waveform[j]>=min*frac && w.waveform[j+1]<min*frac) {
        cfd_pos = j;
        break;
      }
    }
    
    double e1 = w.x0 + w.dx * (i.first + (th - w.waveform[i.first]) / ( w.waveform[i.first+1] - w.waveform[i.first]));
    double e2 = w.x0 + w.dx * (i.second + (th - w.waveform[i.second]) / ( w.waveform[i.second+1] - w.waveform[i.second]));
    double cfd = w.x0 + w.dx * (cfd_pos + (min*frac - w.waveform[cfd_pos]) / ( w.waveform[cfd_pos+1] - w.waveform[cfd_pos]));

    out.emplace_back(Cfd{e1, e2, min, cfd});
  }

  return out;
}

AnalysisTimingFASTPIX::AnalysisTimingFASTPIX(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {}

void AnalysisTimingFASTPIX::initialize() {

    config_.setDefault<double>("chi2ndof_cut", 3.);
    chi2_ndof_cut_ = config_.get<double>("chi2ndof_cut");

    // Initialise member variables
    tree = new TTree("tree", "");
    tree->Branch("pixel", &pixel_);
    tree->Branch("tot", &tot_);
    tree->Branch("dt", &dt_);

    //df = new ROOT::RDataFrame(*tree);

    timewalk2d = new TH2F("timewalk2d", "timewalk", 700, -0.5, 350.5,  200, -20, 0);
    timewalk2d_inner = new TH2F("timewalk2d_inner", "timewalk", 700, -0.5, 350.5,  200, -20, 0);
    timewalk2d_outer = new TH2F("timewalk2d_outer", "timewalk", 700, -0.5, 350.5,  200, -20, 0);

    timewalk = new TH1F("timewalk", "timewalk", 1000, -50, 50);
    timewalk_inner = new TH1F("timewalk_inner", "timewalk", 1000, -50, 50);
    timewalk_outer = new TH1F("timewalk_outer", "timewalk", 1000, -50, 50);

    hitmapLocal =
        new TH2F("hitmapLocal",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);

    hitmapLocalInner =
        new TH2F("hitmapLocalInner",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);

    hitmapLocalOuter =
        new TH2F("hitmapLocalOuter",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);

    hitmapLocalInnerCut =
        new TH2F("hitmapLocalInnerCut",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);

    hitmapLocalOuterCut =
        new TH2F("hitmapLocalOuterCut",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);

    hitmapLocalCut =
        new TH2F("hitmapLocalCut",
                    "hitmap;x_{track} [#mum];y_{track} [#mum]",
                    200,
                    -250.5,
                    249.5,
                    200,
                    -250.5,
                    249.5);
}

StatusCode AnalysisTimingFASTPIX::run(const std::shared_ptr<Clipboard>& clipboard) {

    auto waveforms = clipboard->getData<Waveform>("MCP_0");
    auto timestamps = clipboard->getData<Timestamp>(m_detector->getName());
    auto tracks = clipboard->getData<Track>();

    //LOG(INFO) << waveforms.size() << ' ' << timestamps.size() << ' ' << tracks.size();

    if(waveforms.size() == 1 && timestamps.size() == 1) {
        auto cfd = find_npeaks(waveforms[0]->waveform(), -0.15, 0.2);

        if(cfd.size() == 1) {
            for(const auto& track : tracks) {
                if(track->getChi2ndof() > chi2_ndof_cut_) {
                    LOG(DEBUG) << " - track discarded due to Chi2/ndof";
                    continue;
                }

                // Loop over all associated DUT clusters:
                for(auto assoc_cluster : track->getAssociatedClusters(m_detector->getName())) {
                    // if closest cluster should be used continue if current associated cluster is not the closest one
                    if(/*use_closest_cluster_ &&*/ track->getClosestCluster(m_detector->getName()) != assoc_cluster) {
                        continue;
                    }

                    auto seed = assoc_cluster->getSeedPixel();
                    int col = seed->column();
                    int row = seed->row();

                    col = col + (row - (row&1)) / 2;
                    int idx = col + row * 16;

                    double tot = seed->charge();
                    double dt = cfd[0].cfd_pos - timestamps[0]->timestamp();
                    timewalk2d->Fill(tot, dt);
                    timewalk->Fill(dt);

                    pixel_ = idx;
                    tot_ = tot;
                    dt_ = dt;
                    tree->Fill();

                    auto globalIntercept = m_detector->getIntercept(track.get());
                    auto localIntercept = m_detector->globalToLocal(globalIntercept);

                    hitmapLocal->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                    if(tot>25 && tot < 40) {
                      hitmapLocalCut->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                    }

                    dt_hist.emplace_back(dt);

                    if(col == 0 || col == 15 || row == 0 || row == 3) {
                        timewalk2d_outer->Fill(tot, dt);
                        timewalk_outer->Fill(dt);
                        hitmapLocalOuter->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                        if(tot>25 && tot < 40) {
                          hitmapLocalOuterCut->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                        }
                        dt_outer_hist.emplace_back(dt);
                    } else {
                        timewalk2d_inner->Fill(tot, dt);
                        timewalk_inner->Fill(dt);
                        hitmapLocalInner->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                        if(tot>25 && tot < 40) {
                          hitmapLocalInnerCut->Fill(localIntercept.X() * 1000.0, localIntercept.Y() * 1000.0);
                        }
                        dt_inner_hist.emplace_back(dt);
                    }
                }
            }
        }
    }



    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisTimingFASTPIX::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    /*for(size_t i = 0; i < 64; i++) {
        //std::string s1 = std::string("pixel == ") + std::to_string(i);
        std::string s2 = std::string("pix_") + std::to_string(i);
        auto f = [=](int idx) { return idx == i; };
        auto h = df->Filter(f, {"pixel"}).Histo2D({s2.c_str(), "",  700, -0.5, 350.5,  200, -50, 50}, "tot", "dt");
        h->Write();
    }*/

    LOG(INFO) << "RMS inner " << rms(dt_inner_hist);
    LOG(INFO) << "RMS outer " << rms(dt_outer_hist);
    LOG(INFO) << "RMS all " << rms(dt_hist);

    LOG(INFO) << "RMS997 inner " << rms997(dt_inner_hist);
    LOG(INFO) << "RMS997 outer " << rms997(dt_outer_hist);
    LOG(INFO) << "RMS997 all " << rms997(dt_hist);

}
