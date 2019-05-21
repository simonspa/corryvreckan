#include "Track.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Track::Track() : m_direction(0, 0, 1.), m_state(0, 0, 0.) {}
Track::Track(Track* track) {
    auto trackClusters = track->clusters();
    for(auto& track_cluster : trackClusters) {
        Cluster* cluster = new Cluster(track_cluster);
        addCluster(cluster);
    }

    auto associatedClusters = track->associatedClusters();
    for(auto& assoc_cluster : associatedClusters) {
        Cluster* cluster = new Cluster(assoc_cluster);
        addAssociatedCluster(cluster);
    }
    m_state = track->m_state;
    m_direction = track->m_direction;
}

void Track::addCluster(const Cluster* cluster) {
    m_trackClusters.push_back(const_cast<Cluster*>(cluster));
}
void Track::addAssociatedCluster(const Cluster* cluster) {
    m_associatedClusters.push_back(const_cast<Cluster*>(cluster));
}

std::vector<Cluster*> Track::clusters() const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : m_trackClusters) {
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(dynamic_cast<Cluster*>(cluster.GetObject()));
    }

    // Return as a vector of pixels
    return clustervec;
}

std::vector<Cluster*> Track::associatedClusters() const {
    std::vector<Cluster*> clustervec;
    for(auto& cluster : m_associatedClusters) {
        if(!cluster.IsValid() || cluster.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Cluster));
        }
        clustervec.emplace_back(dynamic_cast<Cluster*>(cluster.GetObject()));
    }

    // Return as a vector of pixels
    return clustervec;
}

double Track::distance2(const Cluster* cluster) const {

    // Get the track X and Y at the cluster z position
    double trackX = m_state.X() + m_direction.X() * cluster->global().z();
    double trackY = m_state.Y() + m_direction.Y() * cluster->global().z();

    // Calculate the 1D residuals
    double dx = (trackX - cluster->global().x());
    double dy = (trackY - cluster->global().y());

    // Return the distance^2
    return (dx * dx + dy * dy);
}

void Track::calculateChi2() {

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

double Track::operator()(const double* parameters) {

    // Update the track gradient and intercept
    this->m_direction.SetX(parameters[0]);
    this->m_state.SetX(parameters[1]);
    this->m_direction.SetY(parameters[2]);
    this->m_state.SetY(parameters[3]);

    // Calculate the chi2
    this->calculateChi2();

    // Return this to minuit
    return m_chi2;
}

void Track::fit() {

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

    // Get the track parameters
    double slopex = (vecx[1] * matx[0][0] - vecx[0] * matx[1][0]) / detx;
    double slopey = (vecy[1] * maty[0][0] - vecy[0] * maty[1][0]) / dety;

    double interceptx = (vecx[0] * matx[1][1] - vecx[1] * matx[1][0]) / detx;
    double intercepty = (vecy[0] * maty[1][1] - vecy[1] * maty[1][0]) / dety;

    // Set the track parameters
    m_state.SetX(interceptx);
    m_state.SetY(intercepty);
    m_state.SetZ(0.);

    m_direction.SetX(slopex);
    m_direction.SetY(slopey);
    m_direction.SetZ(1.);

    // Calculate the chi2
    this->calculateChi2();
}

bool Track::isAssociated(Cluster* cluster) const {
    if(std::find(m_associatedClusters.begin(), m_associatedClusters.end(), cluster) != m_associatedClusters.end()) {
        return true;
    }
    return false;
}

ROOT::Math::XYZPoint Track::intercept(double z) const {
    return m_state + m_direction * z;
}
