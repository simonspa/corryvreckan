#include "StraightLineTrack.hpp"
#include "Eigen/Dense"
#include "Track.hpp"
#include "exceptions.h"

using namespace corryvreckan;

StraightLineTrack::StraightLineTrack() : Track(), m_direction(0, 0, 1.), m_state(0, 0, 0.) {}

StraightLineTrack::StraightLineTrack(const StraightLineTrack& track) : Track(track) {
    if(track.getType() != this->getType())
        throw TrackModelChanged(typeid(*this), track.getType(), this->getType());
    m_direction = track.m_direction;
    m_state = track.m_state;
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
ROOT::Math::XYPoint StraightLineTrack::distance(const Cluster* cluster) const {

    // Get the StraightLineTrack X and Y at the cluster z position
    double StraightLineTrackX = m_state.X() + m_direction.X() * cluster->global().z();
    double StraightLineTrackY = m_state.Y() + m_direction.Y() * cluster->global().z();

    // Calculate the 1D residuals
    double dx = (StraightLineTrackX - cluster->global().x());
    double dy = (StraightLineTrackY - cluster->global().y());

    // Return the distance^2
    return ROOT::Math::XYPoint(dx, dy);
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

        // Get the distance and the error
        ROOT::Math::XYPoint dist = this->distance(cluster);
        double ex2 = cluster->errorX() * cluster->errorX();
        double ey2 = cluster->errorY() * cluster->errorY();
        m_chi2 += (dist.x() * dist.x() / ex2 + dist.y() * dist.y() / ey2);
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

    Eigen::Vector2d vecx(0, 0);
    Eigen::Vector2d vecy(0, 0);
    Eigen::Matrix2d matx;
    matx << 0, 0, 0, 0;
    Eigen::Matrix2d maty;
    maty << 0, 0, 0, 0;

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
        // cluster has an x/y error
        double ex2 = cluster->errorX() * cluster->errorX();
        double ey2 = cluster->errorY() * cluster->errorY();
        // Fill the matrices
        vecx(0) += x / ex2;
        vecx(1) += x * z / ex2;
        vecy(0) += y / ey2;
        vecy(1) += y * z / ey2;

        matx(0, 0) += 1. / ex2;
        matx(1, 0) += z / ex2;
        matx(0, 1) = matx(1, 0);
        matx(1, 1) += z * z / ex2;
        maty(0, 0) += 1. / ey2;
        maty(1, 0) += z / ey2;
        maty(0, 1) = maty(1, 0);
        maty(1, 1) += z * z / ey2;
    }

    // Check for singularities.
    if(matx.determinant() == 0. || maty.determinant() == 0.)
        return;

    // Get the StraightLineTrack parameters
    Eigen::Vector2d resX = matx.inverse() * vecx;
    Eigen::Vector2d resY = maty.inverse() * vecy;

    // Set the StraightLineTrack parameters
    m_state.SetX(resX(0));
    m_state.SetY(resY(0));
    m_state.SetZ(0.);

    m_direction.SetX(resX(1));
    m_direction.SetY(resY(1));
    m_direction.SetZ(1.);

    // Calculate the chi2
    this->calculateChi2();
    m_isFitted = true;
}

ROOT::Math::XYZPoint StraightLineTrack::intercept(double z) const {
    return m_state + m_direction * z;
}

void StraightLineTrack::print(std::ostream& out) const {
    out << "StraightLineTrack " << this->m_state << ", " << this->m_direction << ", " << this->m_chi2 << ", " << this->m_ndof
        << ", " << this->m_chi2ndof << ", " << this->timestamp();
}
