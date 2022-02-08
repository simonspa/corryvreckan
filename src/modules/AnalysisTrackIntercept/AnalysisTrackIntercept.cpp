#include "AnalysisTrackIntercept.h"

#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

using namespace corryvreckan;

AnalysisTrackIntercept::AnalysisTrackIntercept(Configuration& config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {

    m_planes_z = config.getArray<double>("plane_z");
    n_bins_x = config.get<int>("n_bins_x");
    n_bins_y = config.get<int>("n_bins_y");
    xmin = config.get<double>("xmin");
    xmax = config.get<double>("xmax");
    ymin = config.get<double>("ymin");
    ymax = config.get<double>("ymax");
}

void AnalysisTrackIntercept::initialize() {

    std::cout << "AnalysisTrackIntercept:initialize" << std::endl;

    TDirectory* directory = getROOTDirectory();
    directory->cd();

    for(auto cur_plane_z : m_planes_z) {
        m_intercepts.push_back(new TH2F(("track_intercepts_plane_" + std::to_string(cur_plane_z)).c_str(),
                                        ("track_intercepts_plane_" + std::to_string(cur_plane_z)).c_str(),
                                        n_bins_x,
                                        xmin,
                                        xmax,
                                        n_bins_y,
                                        ymin,
                                        ymax));
    }
}

StatusCode AnalysisTrackIntercept::run(const std::shared_ptr<Clipboard>& clipboard) {

    // Get the telescope tracks from the clipboard
    auto tracks = clipboard->getData<Track>();

    if(!tracks.size()) {
        return StatusCode::Success;
    }

    // Loop over all tracks
    for(auto& track : tracks) {
        for(unsigned int plane_ind = 0; plane_ind < m_planes_z.size(); plane_ind++) {
            ROOT::Math::XYZPoint intercept = track->getIntercept(m_planes_z[plane_ind]);
            m_intercepts[plane_ind]->Fill(intercept.x(), intercept.y());
        }
    }

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void AnalysisTrackIntercept::finalize(const std::shared_ptr<ReadonlyClipboard>&) {}
