#include "GblTrack.hpp"
#include "Track.hpp"
#include "exceptions.h"

#include "GeneralBrokenLines/include/GblPoint.h"
#include "GeneralBrokenLines/include/GblTrajectory.h"

using namespace corryvreckan;
using namespace gbl;
GblTrack::GblTrack() : Track() {
    m_trackModel = "gbl";
}

GblTrack::GblTrack(const Track& track) : Track(track) {
    if(m_trackModel != "gbl")
        throw Exception("track model changed!");
    fit();
}

double GblTrack::distance2(const Cluster* cluster) const {

    return cluster->global().x();
}

void GblTrack::fit() {}

ROOT::Math::XYZPoint GblTrack::intercept(double z) const {
    return ROOT::Math::XYZPoint(0, 0, z);
}

ROOT::Math::XYZPoint GblTrack::state(std::string) const {
    return ROOT::Math::XYZPoint(0, 0, 0);
}

ROOT::Math::XYZVector GblTrack::direction(std::string) const {
    return ROOT::Math::XYZVector(0, 0, 1);
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack ";
}
