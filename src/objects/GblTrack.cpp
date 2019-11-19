#include "GblTrack.hpp"
#include "Track.hpp"
#include "exceptions.h"

#include "GblPoint.h"
#include "GblTrajectory.h"

using namespace corryvreckan;
using namespace gbl;
using namespace std;

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

void GblTrack::fit() {

    // create a list of gbl points:
    vector<GblPoint> points;

    auto seedcluster = dynamic_cast<Cluster*>(m_trackClusters.at(0).GetObject());

    // we need to store the previous z position as well as the up to know material
    double total_material = 0, prev_z = 0;
    // keep track of the detectors to ensure we know which gblpoint later is from which plane - also remember if there was a
    // measurement
    std::vector<std::pair<std::string, bool>> detectors;
    // store the used clusters in a map for easy access:
    std::map<std::string, Cluster*> clusters;
    for(auto& c : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        clusters[cluster->detectorID()] = cluster;
    }

    for(auto layer : m_materialBudget) {
        double mb = layer.second.first;
        double current_z = layer.second.second;
        if(clusters.count(layer.first) == 1) {
            current_z = clusters.at(layer.first)->global().z();
        }
        Matrix5d Jac;
        Jac = Matrix5d::Identity();
        if(points.size() != 0) {
            Jac(3, 1) = current_z - prev_z;
            Jac(4, 2) = Jac(3, 1);
        }
        Eigen::Vector2d budget;
        budget(0) = 1 / (scatteringTheta(mb, total_material) * scatteringTheta(mb, total_material));
        budget(1) = budget(0);
        total_material += mb;
        auto point = GblPoint(Jac);
        point.addScatterer(Eigen::Vector2d::Zero(), budget);
        if(clusters.count(layer.first) == 1) {
            auto cluster = clusters.at(layer.first);
            Eigen::Vector2d initialResidual;
            initialResidual(0) = cluster->global().x() - seedcluster->global().x();
            initialResidual(1) = cluster->global().y() - seedcluster->global().y();
            // uncertainty of single hit
            Eigen::Matrix2d covv = Eigen::Matrix2d::Identity();
            covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
            covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
            point.addMeasurement(initialResidual, covv);
            detectors.push_back(std::pair<std::string, bool>(layer.first, true));
        } else {
            detectors.push_back(std::pair<std::string, bool>(layer.first, false));
        }
        prev_z = current_z;
        points.push_back(point);
    }
    // fit it
    if(points.size() != m_materialBudget.size())
        throw GblException(typeid(GblTrack), "wrong number of measuremtns");

    GblTrajectory traj(points, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;
    unsigned success = traj.fit(m_chi2, ndf, lostWeight);
    if(success != 0) {
        throw GblException(typeid(GblTrack), "fitting failed");
    }
    m_ndof = double(ndf);
    m_chi2ndof = (m_ndof < 0.0) ? -1 : (m_chi2 / m_ndof);

    // copy the results
    Eigen::VectorXd localPar(5);
    Eigen::MatrixXd localCov(5, 5);
    Eigen::VectorXd gblCorrection(5);
    Eigen::MatrixXd gblCovariance(5, 5);
    Eigen::VectorXd gblResiduals(2);
    Eigen::VectorXd gblErrorsMeasurements(2);
    Eigen::VectorXd gblErrorsResiduals(2);
    Eigen::VectorXd gblDownWeights(2);
    unsigned int numData = 2;
    unsigned gblcounter = 1;
    for(auto plane : detectors) {
        traj.getScatResults(gblcounter, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        m_kink[plane.first] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        if(plane.second) {
            traj.getMeasResults(
                gblcounter, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
            m_residual[plane.first] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        }
    }
}

ROOT::Math::XYZPoint GblTrack::intercept(double z) const {
    return ROOT::Math::XYZPoint(0, 0, z);
}

ROOT::Math::XYZPoint GblTrack::state(std::string) const {
    return ROOT::Math::XYZPoint(0, 0, 0);
}

ROOT::Math::XYZVector GblTrack::direction(std::string) const {
    return ROOT::Math::XYZVector(0, 0, 1);
}

double GblTrack::scatteringTheta(double mbCurrent, double mbSum) {

    return (13.6 / m_momentum * sqrt(mbCurrent) * (1 + 0.038 * log(mbSum + mbCurrent)));
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << m_trackClusters.size() << " and nscatterers = " << m_materialBudget.size()
        << ", chi2 = " << m_chi2 << ", ndf = " << m_ndof;
}
