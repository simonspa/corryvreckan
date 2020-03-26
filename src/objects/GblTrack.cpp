/**
 * @file
 * @brief Implementation of GBL track object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GblTrack.hpp"
#include "Track.hpp"
#include "exceptions.h"

#include <Math/Point3D.h>
#include "GblPoint.h"
#include "GblTrajectory.h"
#include "Math/Vector3D.h"
using namespace corryvreckan;
using namespace gbl;

GblTrack::GblTrack() : Track() {}

GblTrack::GblTrack(const GblTrack& track) : Track(track) {
    if(track.getType() != this->getType())
        throw TrackModelChanged(typeid(*this), track.getType(), this->getType());
}

void GblTrack::fit() {

    // Fitting with less than 2 clusters is pointless
    if(m_trackClusters.size() < 2) {
        throw TrackError(typeid(GblTrack), " attempting to fit a track with less than 2 clusters");
    }
    // store the used clusters in a map for easy access:
    std::map<std::string, Cluster*> clusters;
    for(auto& c : m_trackClusters) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        clusters[cluster->detectorID()] = cluster;
    }
    // gbl needs a seed cluster - at this stage it might be a nullptr
    auto seedcluster = m_planes.front().cluster();
    // create a list of planes and sort it, also calculate the material budget:
    double total_material = 0;
    std::sort(m_planes.begin(), m_planes.end());
    for(auto& l : m_planes) {
        total_material += l.materialbudget();
        if(clusters.count(l.name()) == 1) {
            if(seedcluster == nullptr) {
                seedcluster = clusters.at(l.name());
            }
            l.setPosition(clusters.at(l.name())->global().z());
            l.setCluster(clusters.at(l.name()));
        }
    }

    // add volume scattering length - for now simply the distance between first and last plane
    if(m_use_volume_scatter) {
        total_material += (m_planes.back().position() - m_planes.front().position()) / m_scattering_length_volume;
    }
    // lambda to add measurement (if existing) and scattering from single plane
    // lambda to calculate the scattering theta
    auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
        return (13.6 / m_momentum * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
    };
    auto addScattertoGblPoint = [&total_material, &scatteringTheta](GblPoint& point, double material) {
        Eigen::Vector2d scatterWidth;
        scatterWidth(0) = 1 / (scatteringTheta(material, total_material) * scatteringTheta(material, total_material));
        scatterWidth(1) = scatterWidth(0);
        point.addScatterer(Eigen::Vector2d::Zero(), scatterWidth);
    };
    std::vector<GblPoint> points;
    // get the seed  for the fit - simply global x/y of the first cluster in the list and z = 0
    auto prevToGlobal = m_planes.front().toGlobal();
    auto prevToLocal = m_planes.front().toLocal();
    auto globalTrackPos = seedcluster->global();
    globalTrackPos.SetZ(0);
    auto globalTangent = Eigen::Vector4d(0, 0, 1, 0);
    Eigen::Vector4d localPosTrack;
    Eigen::Vector4d localTangent;

    auto fromTransform3DtoEigenMatrix = [](Transform3D in) {
        Eigen::Matrix4d t = Eigen::Matrix4d::Zero();
        std::vector<double> c;
        c.resize(12, 0);
        in.GetComponents(c.begin());
        // t << c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11];
        t << c[0], c[1], c[2], 0, c[4], c[5], c[6], 0, c[8], c[9], c[10], 1;
        return t;
    };

    // lambda Jacobian
    auto jac = [](const Eigen::Vector4d& tangent, const Eigen::Matrix4d& toTarget, double distance) {
        // lmbda to transform ROOT::Transform3D to an Eigen3 Matrix - we only need the rotations to get the correct
        // transformation between the two local systems. Shift is handeld by distance

        Eigen::Matrix<double, 4, 3> R;
        R.col(0) = toTarget.col(0);
        R.col(1) = toTarget.col(1);
        R.col(2) = toTarget.col(3);
        Eigen::Vector4d S = toTarget * tangent * (1 / tangent[2]);

        Eigen::Matrix<double, 3, 4> F = Eigen::Matrix<double, 3, 4>::Zero();
        F(0, 0) = 1;
        F(1, 1) = 1;
        F(2, 3) = 1;
        F(0, 2) = -S[0] / S[2];
        F(1, 2) = -S[1] / S[2];
        F(2, 2) = -S[3] / S[2];
        Eigen::Matrix<double, 6, 6> jaco;

        jaco << F * R, (-distance / S[2]) * F * R, Eigen::Matrix3d::Zero(), (1 / S[2]) * F * R;
        jaco(5, 5) = 1; // set a future time component to 1
        return jaco;
    };

    auto addMeasurementtoGblPoint = [&localTangent, &localPosTrack, &globalTrackPos, this](GblPoint& point,
                                                                                           std::vector<Plane>::iterator& p) {
        auto cluster = p->cluster();
        Eigen::Vector2d initialResidual;

        // FIXME: We need the correct initial seed and then do it all in local coordinates
        //                initialResidual(0) = cluster->global().x() - m_planes.begin()->cluster()->global().x();
        //                initialResidual(1) = cluster->global().y() - m_planes.begin()->cluster()->global().y();
        initialResidual(0) = cluster->local().x() - localPosTrack[0];
        initialResidual(1) = cluster->local().y() - localPosTrack[1];
        // Uncertainty of single hit in local coordinates
        Eigen::Matrix2d covv = Eigen::Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        point.addMeasurement(initialResidual, covv);
        if(m_logging /*|| true*/) {
            std::cout << "*********** Plane:  " << p->name() << " \n Global Res to fit: \t("
                      << (cluster->global().x() - m_planes.begin()->cluster()->global().x()) << ", "
                      << (cluster->global().y() - m_planes.begin()->cluster()->global().y()) << ")\n Local Res to fit: \t("
                      << (cluster->local().x() - localPosTrack[0]) << ", " << (cluster->local().y() - localPosTrack[1])
                      << ")\n Local  track pos: \t(" << localPosTrack[0] << ", " << localPosTrack[1] << ", "
                      << localPosTrack[2] << ", " << localPosTrack[3] << ")\n global track Pos:\t" << globalTrackPos
                      << "\n local tangent:\t\t(" << localTangent[0] << ", " << localTangent[1] << ", " << localTangent[2]
                      << ", " << localTangent[3] << ")\n cluster global:\t " << p->cluster()->global()
                      << " \n cluster local:\t" << p->cluster()->local() << std::endl;
        }
    };

    // Mapping of parameters in proteus - I would like to get rid of these converions once it works
    Eigen::Matrix<double, 5, 6> toGbl = Eigen::Matrix<double, 5, 6>::Zero();
    Eigen::Matrix<double, 6, 5> toProteus = Eigen::Matrix<double, 6, 5>::Zero();
    toGbl(0, 5) = 1;
    toGbl(1, 3) = 1;
    toGbl(2, 4) = 1;
    toGbl(3, 0) = 1;
    toGbl(4, 1) = 1;
    toProteus(0, 3) = 1;
    toProteus(1, 4) = 1;
    toProteus(3, 1) = 1;
    toProteus(4, 2) = 1;
    toProteus(5, 0) = 1;

    // lambda to add plane (not the first one) and air scatterers //FIXME: Where to put them?
    auto addPlane = [&](std::vector<Plane>::iterator& plane) {
        auto tmp_local = plane->toLocal() * globalTrackPos;
        localPosTrack = Eigen::Vector4d{tmp_local.x(), tmp_local.y(), tmp_local.z(), 1};
        localTangent = fromTransform3DtoEigenMatrix(plane->toLocal()) * globalTangent;
        double dist = localPosTrack[2];
        if(m_logging) {
            std::cout << fromTransform3DtoEigenMatrix(plane->toLocal()) << "\n local tan before normalization"
                      << localTangent << "\n distance: " << dist << std::endl;
        }
        localTangent /= localTangent.z();
        localPosTrack -= dist * localTangent;

        Eigen::Vector4d prevTan = fromTransform3DtoEigenMatrix(prevToLocal) * globalTangent;
        Eigen::Matrix4d toTarget =
            fromTransform3DtoEigenMatrix(plane->toLocal()) * fromTransform3DtoEigenMatrix(prevToGlobal);
        auto myjac = jac(prevTan, toTarget, dist);

        if(points.size() == 0) {
            myjac = Eigen::Matrix<double, 6, 6>::Identity();
            myjac(0, 0) = 0;
        }
        auto transformedJac = toGbl * myjac * toProteus;

        if(m_logging /*|| true*/) {
            std::cout << "Plane " << plane->name() << " jac\n"
                      << transformedJac << std::endl
                      << "\n to local \n"
                      << plane->toLocal() << "\n to global \n"
                      << plane->toGlobal();
        }
        auto point = GblPoint(transformedJac);
        addScattertoGblPoint(point, plane->materialbudget());
        if(plane->hasCluster()) {
            addMeasurementtoGblPoint(point, plane);
        }
        prevToGlobal = plane->toGlobal();
        prevToLocal = plane->toLocal();
        points.push_back(point);
        plane->setGblPos(unsigned(points.size())); // gbl starts counting at 1
        globalTrackPos =
            plane->toGlobal() *
            ROOT::Math::XYZPoint(localPosTrack(0), localPosTrack(1), localPosTrack(2)); // reference slope stays unchanged
    };

    // First GblPoint
    std::vector<Plane>::iterator pl = m_planes.begin();
    // add all other points
    for(; pl != m_planes.end(); ++pl) {
        addPlane(pl);
    }

    // Make sure we missed nothing
    if((points.size() != ((m_materialBudget.size() * 3) - 2) && m_use_volume_scatter) ||
       (points.size() != m_materialBudget.size() && !m_use_volume_scatter)) {
        throw TrackError(typeid(GblTrack),
                         "Number of planes " + std::to_string(m_materialBudget.size()) +
                             " doesn't match number of GBL points on trajectory " + std::to_string(points.size()));
    }
    // perform fit
    GblTrajectory traj(points, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;
    // fit it
    auto fitReturnValue = traj.fit(m_chi2, ndf, lostWeight);
    if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
        fitReturnValue = traj.fit(m_chi2, ndf, lostWeight);
        if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
            return;
            //            throw TrackFitError(typeid(GblTrack), "internal GBL Error " + std::to_string(fitReturnValue));
        }
    }
    if(m_logging /*|| true*/) {
        std::cout << "Fit Results: chi2 = " << m_chi2 << "\t wth npoints: " << points.size() << "and fit return value "
                  << fitReturnValue << std::endl
                  << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
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
    for(auto plane : m_planes) {
        traj.getScatResults(
            plane.gblPos(), numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        m_kink[plane.name()] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        traj.getResults(int(plane.gblPos()), localPar, localCov);
        m_corrections[plane.name()] = ROOT::Math::XYZPoint(localPar(3), localPar(4), 0);
        if(plane.hasCluster()) {
            traj.getMeasResults(
                plane.gblPos(), numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
            m_residual[plane.name()] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        }
        if(m_logging) {
            std::cout << m_residual[plane.name()] << "\t" << m_kink[plane.name()] << std::endl;
        }
    }
    //    throw TrackFitError(typeid(GblTrack), "internal GBL Error ");

    m_isFitted = true;
}

ROOT::Math::XYZPoint GblTrack::intercept(double z) const {
    // find the detector with largest z-positon <= z, assumes detectors sorted by z position
    std::string layer = "";
    bool found = false;

    if(!m_isFitted) {
        throw TrackError(typeid(GblTrack), "An interception is requested befor the track is fitted");
    }
    for(auto l : m_planes) {
        layer = l.name();
        if(l.position() >= z) {
            found = true;
            break;
        }
    }
    if(!found) {
        throw TrackError(typeid(GblTrack), "Z-Position of " + std::to_string(z) + " is outside the telescopes z-coverage");
    }
    return (state(layer) + direction(layer) * (z - state(layer).z()));
}

ROOT::Math::XYZPoint GblTrack::state(std::string detectorID) const {
    // The track state at any plane is the seed (always first cluster for now) plus the correction for the plane
    // And as rotations are ignored, the z position is simply the detectors z position
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
    // For larger rotations it is an issue
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

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << m_trackClusters.size() << " and nscatterers = " << m_materialBudget.size()
        << ", chi2 = " << m_chi2 << ", ndf = " << m_ndof;
}
