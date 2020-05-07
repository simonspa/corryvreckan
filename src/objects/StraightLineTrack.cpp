/**
 * @file
 * @brief Implementation of StraightLine track object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

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

ROOT::Math::XYPoint StraightLineTrack::getKinkAt(std::string) const {
    return ROOT::Math::XYPoint(0, 0);
}

void StraightLineTrack::calculateChi2() {

    // Get the number of clusters
    ndof_ = static_cast<double>(track_clusters_.size()) - 2.;
    chi2_ = 0.;
    chi2ndof_ = 0.;

    // Loop over all clusters
    for(auto& cl : track_clusters_) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        // Get the distance and the error
        ROOT::Math::XYPoint dist = this->distance(cluster);
        double ex2 = cluster->errorX() * cluster->errorX();
        double ey2 = cluster->errorY() * cluster->errorY();
        chi2_ += ((dist.x() * dist.x() / ex2) + (dist.y() * dist.y() / ey2));
    }

    // Store also the chi2/degrees of freedom
    chi2ndof_ = chi2_ / ndof_;
}

void StraightLineTrack::calculateResiduals() {
    for(auto c : track_clusters_) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        residual_[cluster->detectorID()] = cluster->global() - getIntercept(cluster->global().z());
    }
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
    return chi2_;
}

void StraightLineTrack::fit() {

    Eigen::Matrix4d mat(Eigen::Matrix4d::Zero());
    Eigen::Vector4d vec(Eigen::Vector4d::Zero());

    // Loop over all clusters and fill the matrices
    for(auto& cl : track_clusters_) {
        auto cluster = dynamic_cast<Cluster*>(cl.GetObject());
        if(cluster == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }

        // Get the measurement and its errors
        double x = cluster->global().x();
        double y = cluster->global().y();
        double z = cluster->global().z();
        double ex2 = cluster->errorX() * cluster->errorX();
        double ey2 = cluster->errorY() * cluster->errorY();

        // Fill the matrices
        vec += Eigen::Vector4d((x / ex2), (x * z / ex2), (y / ey2), (y * z / ey2));
        Eigen::Vector4d pos(x, x, y, y);
        Eigen::Matrix2d err;
        err << 1, z, z, z * z;
        mat.topLeftCorner(2, 2) += err / ex2;
        mat.bottomRightCorner(2, 2) += err / ey2;
    }

    // Check for singularities.
    if(fabs(mat.determinant()) < std::numeric_limits<double>::epsilon()) {
        throw TrackFitError(typeid(this), "Martix inversion in straight line fit failed");
    }
    // Get the StraightLineTrack parameters
    Eigen::Vector4d res = mat.inverse() * vec;

    // Set the StraightLineTrack parameters
    m_state.SetX(res(0));
    m_state.SetY(res(2));
    m_state.SetZ(0.);

    m_direction.SetX(res(1));
    m_direction.SetY(res(3));
    m_direction.SetZ(1.);

    // Calculate the chi2
    this->calculateChi2();
    this->calculateResiduals();
    isFitted_ = true;
}

ROOT::Math::XYZPoint StraightLineTrack::getIntercept(double z) const {
    return m_state + m_direction * z;
}

void StraightLineTrack::print(std::ostream& out) const {
    out << "StraightLineTrack " << this->m_state << ", " << this->m_direction << ", " << this->chi2_ << ", " << this->ndof_
        << ", " << this->chi2ndof_ << ", " << this->timestamp();
}
