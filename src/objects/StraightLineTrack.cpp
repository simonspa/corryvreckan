#include "StraightLineTrack.hpp"
#include "Track.hpp"
#include "exceptions.h"

using namespace corryvreckan;

StraightLineTrack::StraightLineTrack() : Track(), m_direction(0, 0, 1.), m_state(0, 0, 0.) {
    m_trackModel = "straightline";
}

StraightLineTrack::StraightLineTrack(const Track& track) : Track(track) {
    if(m_trackModel != "straightline")
        throw Exception("track model changed!");
    fit();
}

double StraightLineTrack::distance2(const Cluster* cluster) const {

    // Get the StraightLineTrack X and Y at the cluster z position
    double StraightLineTrackX = m_state.X() + m_direction.X() * cluster->global().z();
    double StraightLineTrackY = m_state.Y() + m_direction.Y() * cluster->global().z();

    // Calculate the 1D residuals
    double dx = (StraightLineTrackX - cluster->global().x());
    double dy = (StraightLineTrackY - cluster->global().y());

    // Return the distance^2
    return (dx * dx + dy * dy);
}

void StraightLineTrack::calculateChi2() {

    // Get the number of clusters
    m_ndof = static_cast<double>(m_trackClusters.size()) - 2.;
    m_chi2 = 0.;
    m_chi2ndof = 0.;

    // Loop over all clusters
    for(auto& cl : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        // Get the distance^2 and the error^2
        double error2 = cluster->error() * cluster->error();
        m_chi2 += (this->distance2(cluster) / error2);
    }

    // Store also the chi2/degrees of freedom
    m_chi2ndof = m_chi2 / m_ndof;
}

double StraightLineTrack::operator()(const double* parameters) {

    // Update the StraightLineTrack gradient and intercept
    this->m_direction.SetX(parameters[0]);
    this->m_state.SetX(parameters[1]);
    this->m_direction.SetY(parameters[2]);
    this->m_state.SetY(parameters[3]);

    // Calculate the chi2
    this->calculateChi2();

    // Return this to minuit
    return m_chi2;
}

void StraightLineTrack::fit() {

    double vecx[2] = {0., 0.};
    double vecy[2] = {0., 0.};
    double matx[2][2] = {{0., 0.}, {0., 0.}};
    double maty[2][2] = {{0., 0.}, {0., 0.}};

    // Loop over all clusters and fill the matrices
    for(auto& cl : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        // Get the global point details
        double x = cluster->global().x();
        double y = cluster->global().y();
        double z = cluster->global().z();
        double er2 = cluster->error() * cluster->error();
        // Fill the matrices
        vecx[0] += x / er2;
        vecx[1] += x * z / er2;
        vecy[0] += y / er2;
        vecy[1] += y * z / er2;
        matx[0][0] += 1. / er2;
        matx[1][0] += z / er2;
        matx[1][1] += z * z / er2;
        maty[0][0] += 1. / er2;
        maty[1][0] += z / er2;
        maty[1][1] += z * z / er2;
    }

    // Invert the matrices
    double detx = matx[0][0] * matx[1][1] - matx[1][0] * matx[1][0];
    double dety = maty[0][0] * maty[1][1] - maty[1][0] * maty[1][0];

    // Check for singularities.
    if(detx == 0. || dety == 0.)
        return;

    // Get the StraightLineTrack parameters
    double slopex = (vecx[1] * matx[0][0] - vecx[0] * matx[1][0]) / detx;
    double slopey = (vecy[1] * maty[0][0] - vecy[0] * maty[1][0]) / dety;

    double interceptx = (vecx[0] * matx[1][1] - vecx[1] * matx[1][0]) / detx;
    double intercepty = (vecy[0] * maty[1][1] - vecy[1] * maty[1][0]) / dety;

    // Set the StraightLineTrack parameters
    m_state.SetX(interceptx);
    m_state.SetY(intercepty);
    m_state.SetZ(0.);

    m_direction.SetX(slopex);
    m_direction.SetY(slopey);
    m_direction.SetZ(1.);
    // Calculate the chi2
    this->calculateChi2();
}

ROOT::Math::XYZPoint StraightLineTrack::intercept(double z) const {
    return m_state + m_direction * z;
}

void StraightLineTrack::print(std::ostream& out) const {
    out << "StraightLineTrack " << this->m_state.x() << ", " << this->m_state.y() << ", " << this->m_state.z() << ", "
        << this->m_direction.x() << ", " << this->m_direction.y() << ", " << this->m_direction.z() << ", " << this->m_chi2
        << ", " << this->m_ndof << ", " << this->m_chi2ndof << ", " << this->timestamp();
}
