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
    auto seedcluster = dynamic_cast<Cluster*>(m_trackClusters.at(0).GetObject());
    vector<GblPoint> points;
    Matrix5d Jac;
    Jac = Jac.Identity();
    double total_material = 0, prev_z = 0;
    std::vector<std::string> detectorNames;
    for(auto& c : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        detectorNames.push_back(cluster->detectorID());
        double mb = m_materialBudget.at(cluster->detectorID());
        double delta_z = cluster->global().z() - prev_z;
        if(points.size() != 0) {
            Jac(3, 1) = delta_z;
            Jac(4, 2) = delta_z;
        }
        Eigen::Vector2d budget;
        budget(0) = scatteringTheta(mb, total_material);
        budget(1) = budget(0);
        total_material += mb;
        prev_z = cluster->global().z();

        // Create the point and add it
        GblPoint point(Jac);
        point.addScatterer(Eigen::Vector2d::Zero(), budget);
        Eigen::Vector2d initialResidual;
        // this assumes only z rotations
        initialResidual(0) = cluster->global().x() - seedcluster->global().x();
        initialResidual(1) = cluster->global().y() - seedcluster->global().y();

        // uncertainty of single hit
        Eigen::Matrix2d covv = Eigen::Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        point.addMeasurement(initialResidual, covv);

        points.push_back(point);
    }

    // fit it
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
    for(unsigned cID = 0; cID < traj.getNumPoints(); ++cID) {
        traj.getScatResults(cID + 1, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        m_kink[detectorNames.at(cID)] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        // only work if plane is in fit
        traj.getMeasResults(cID + 1, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        m_residual[detectorNames.at(cID)] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
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
    out << "GblTrack ";
}
