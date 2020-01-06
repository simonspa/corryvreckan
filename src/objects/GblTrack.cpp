#include "GblTrack.hpp"
#include "Track.hpp"
#include "exceptions.h"

#include "GblPoint.h"
#include "GblTrajectory.h"

using namespace corryvreckan;
using namespace gbl;

GblTrack::GblTrack() : Track() {}

GblTrack::GblTrack(const GblTrack& track) : Track(track) {
    if(track.getType() != this->getType())
        throw TrackModelChanged(typeid(*this), track.getType(), this->getType());
}

double GblTrack::distance2(const Cluster* cluster) const {

    return cluster->global().x();
}

void GblTrack::fit() {

    // create a list of gbl points:
    std::vector<GblPoint> points;

    auto seedcluster = dynamic_cast<Cluster*>(m_trackClusters.at(0).GetObject());

    // we need to store the previous z position as well as the  material off all layers passed
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

    // loop over all layers and add scatter/measurement
    // gbl needs the entries to be sorted by z
    std::vector<std::pair<double, std::string>> sorted_budgets;
    for(auto layer : m_materialBudget) {
        sorted_budgets.push_back(std::pair<double, std::string>(layer.second.second, layer.first));
    }
    std::sort(sorted_budgets.begin(), sorted_budgets.end());

    for(auto layer : sorted_budgets) {
        double mb = m_materialBudget.at(layer.second).second;
        double current_z = layer.first;
        if(clusters.count(layer.second) == 1) {
            current_z = clusters.at(layer.second)->global().z();
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

        if(clusters.count(layer.second) == 1) {
            auto cluster = clusters.at(layer.second);
            Eigen::Vector2d initialResidual;
            initialResidual(0) = cluster->global().x() - seedcluster->global().x();
            initialResidual(1) = cluster->global().y() - seedcluster->global().y();
            // uncertainty of single hit - rotations ignored
            Eigen::Matrix2d covv = Eigen::Matrix2d::Identity();
            covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
            covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
            point.addMeasurement(initialResidual, covv);
            detectors.push_back(std::pair<std::string, bool>(layer.second, true));
        } else {
            detectors.push_back(std::pair<std::string, bool>(layer.second, false));
        }
        prev_z = current_z;
        points.push_back(point);
    }

    if(points.size() != m_materialBudget.size()) {
        throw TrackError(typeid(GblTrack), "wrong number of measuremtns");
    }
    GblTrajectory traj(points, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;
    // fit it
    unsigned success = traj.fit(m_chi2, ndf, lostWeight);
    if(success != 0) { // Is this a good ieda? should we discard track candidates that fail?
        throw TrackFitError(typeid(GblTrack), "internal GBL Error " + std::to_string(success));
    }

    // copy the results
    m_ndof = double(ndf);
    m_chi2ndof = (m_ndof < 0.0) ? -1 : (m_chi2 / m_ndof);
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

        traj.getResults(int(gblcounter), localPar, localCov);
        m_corrections[plane.first] = ROOT::Math::XYZPoint(localPar(3), localPar(4), 0);
        if(plane.second) {
            traj.getMeasResults(
                gblcounter, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
            m_residual[plane.first] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        }
        gblcounter++;
    }
    m_isFitted = true;
}

ROOT::Math::XYZPoint GblTrack::intercept(double z) const {
    // find the detector with largest z-positon <= z, assumes detectors sorted by z position
    std::string layer = "";
    bool found = false;
    for(auto l : m_materialBudget) {
        if(l.second.second >= z) {
            found = true;
            break;
        }
        layer = l.first;
    }
    if(!found) {
        throw TrackError(typeid(GblTrack), "Z-Position of " + std::to_string(z) + " is outside the telescopes z-coverage");
    }
    return (state(layer) + direction(layer) * (z - state(layer).z()));
}

ROOT::Math::XYZPoint GblTrack::state(std::string detectorID) const {
    // The track state at any plane is the seed (always first cluster for now) plus the correction for the plane
    // And as rotations are ignored, the z position is simply the detectors z postion
    // Let's check first if the data is fitted and all components are there

    if(!m_isFitted)
        throw TrackError(typeid(GblTrack), " detector " + detectorID + " state is not defined before fitting");
    if(m_materialBudget.count(detectorID) != 1) {
        std::string list;
        for(auto l : m_materialBudget)
            list.append("\n " + l.first + " with <materialBudget, z position>(" + std::to_string(l.second.first) + ", " +
                        std::to_string(l.second.second) + ")");
        throw TrackError(typeid(GblTrack),
                         " detector " + detectorID + " is not appearing in the material budget map" + list);
    }
    if(m_corrections.count(detectorID) != 1)
        throw TrackError(typeid(GblTrack), " detector " + detectorID + " is not appearing in the corrections map");

    // Using the global detector position here is of course not correct, it works for small/no rotations
    // For larger rotations is it an issue
    return ROOT::Math::XYZPoint(clusters().at(0)->global().x() + correction(detectorID).x(),
                                clusters().at(0)->global().y() + correction(detectorID).y(),
                                m_materialBudget.at(detectorID).second);
}

ROOT::Math::XYZVector GblTrack::direction(std::string detectorID) const {

    // Defining the direction following the particle results in the direction
    // beeing definded from the requested plane onwards to the next one
    ROOT::Math::XYZPoint point = state(detectorID);

    // searching for the next detector layer
    bool found = false;
    std::string nextLayer = "";
    for(auto& layer : m_materialBudget) {
        if(found) {
            nextLayer = layer.first;
            break;
        } else if(layer.first == detectorID) {
            found = true;
        }
    }
    if(nextLayer == "")
        throw TrackError(typeid(GblTrack), "Direction after the last telescope plane not defined");
    ROOT::Math::XYZPoint pointAfter = state(nextLayer);
    return ((pointAfter - point) / (pointAfter.z() - point.z()));
}

double GblTrack::scatteringTheta(double mbCurrent, double mbSum) {

    return (13.6 / m_momentum * sqrt(mbCurrent) * (1 + 0.038 * log(mbSum + mbCurrent)));
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << m_trackClusters.size() << " and nscatterers = " << m_materialBudget.size()
        << ", chi2 = " << m_chi2 << ", ndf = " << m_ndof;
}
