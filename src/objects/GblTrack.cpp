/**
 * @file
 * @brief Implementation of GBL track object with rotations. The logic is inspired by the implementation in proteus (Kiehn,
 * Moritz et al., Proteus beam telescope reconstruction, doi:10.5281/zenodo.2579153)
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
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
using namespace Eigen;

GblTrack::GblTrack() : Track() {}

GblTrack::GblTrack(const GblTrack& track) : Track(track) {
    if(track.getType() != this->getType())
        throw TrackModelChanged(typeid(*this), track.getType(), this->getType());
    for(auto local : track.m_localTrackPoints) {
        m_localTrackPoints[local.first] = local.second;
    }
}

void GblTrack::fit() {

    m_isFitted = false;
    m_residual.clear();
    m_kink.clear();
    m_corrections.clear();
    m_localTrackPoints.clear();

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
    // create a list of planes and sort it, also calculate the material budget:
    double total_material = 0;
    std::sort(m_planes.begin(), m_planes.end());
    for(auto& l : m_planes) {
        total_material += l.materialbudget();
        if(clusters.count(l.name()) == 1) {
            l.setPosition(clusters.at(l.name())->global().z());
            l.setCluster(clusters.at(l.name()));
        }
    }
    // get the seedcluster for the fit - find the first plane with a cluster to use
    set_seed_cluster(
        std::find_if(m_planes.begin(), m_planes.end(), [](auto plane) { return plane.hasCluster(); })->cluster());

    // add volume scattering length - for now simply the distance between first and last plane
    if(m_use_volume_scatter) {
        total_material += (m_planes.back().position() - m_planes.front().position()) / m_scattering_length_volume;
    }

    std::vector<GblPoint> points;

    // lambda to calculate the scattering theta
    auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
        return (13.6 / m_momentum * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
    };
    // lambda to add a scatterer to a GBLPoint
    auto addScattertoGblPoint = [&total_material, &scatteringTheta](GblPoint& point, double material) {
        Vector2d scatterWidth;
        scatterWidth(0) = 1 / (scatteringTheta(material, total_material) * scatteringTheta(material, total_material));
        scatterWidth(1) = scatterWidth(0);
        point.addScatterer(Vector2d::Zero(), scatterWidth);
    };

    // extract the rotation from an ROOT::Math::Transfrom3D, sotred in 4x4 matrix to match proteus format
    auto getRotation = [](Transform3D in) {
        Matrix4d t = Matrix4d::Zero();
        in.Rotation().GetRotationMatrix(t);
        return t;
    };

    auto prevToGlobal = m_planes.front().toGlobal();
    auto prevToLocal = m_planes.front().toLocal();
    auto globalTrackPos = get_seed_cluster()->global();
    globalTrackPos.SetZ(0);
    auto globalTangent = Vector4d(0, 0, 1, 0);
    Vector4d localPosTrack;
    Vector4d localTangent;

    // lambda Jacobian
    auto jac = [](const Vector4d& tangent, const Matrix4d& toTarget, double distance) {
        Matrix<double, 4, 3> R;
        R.col(0) = toTarget.col(0);
        R.col(1) = toTarget.col(1);
        R.col(2) = toTarget.col(3);
        Vector4d S = toTarget * tangent * (1 / tangent[2]);

        Matrix<double, 3, 4> F = Matrix<double, 3, 4>::Zero();
        F(0, 0) = 1;
        F(1, 1) = 1;
        F(2, 3) = 1;
        F(0, 2) = -S[0] / S[2];
        F(1, 2) = -S[1] / S[2];
        F(2, 2) = -S[3] / S[2];
        Matrix<double, 6, 6> jaco;

        jaco << F * R, (-distance / S[2]) * F * R, Matrix3d::Zero(), (1 / S[2]) * F * R;
        jaco(5, 5) = 1; // set a future time component to 1
        return jaco;
    };
    auto addMeasurementtoGblPoint = [&localTangent, &localPosTrack, &globalTrackPos, this](GblPoint& point,
                                                                                           std::vector<Plane>::iterator& p) {
        auto cluster = p->cluster();
        Vector2d initialResidual;
        initialResidual(0) = cluster->local().x() - localPosTrack[0];
        initialResidual(1) = cluster->local().y() - localPosTrack[1];
        // Uncertainty of single hit in local coordinates
        Matrix2d covv = Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        point.addMeasurement(initialResidual, covv);
        m_inititalResidual[p->name()] = ROOT::Math::XYPoint(initialResidual(0), initialResidual(1));
        if(m_logging) {
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

    // lambda to add plane (not the first one) and air scatterers //FIXME: Where to put them?
    auto addPlane = [&](std::vector<Plane>::iterator& plane) {
        // Mapping of parameters in proteus - I would like to get rid of these converions once it works
        // For now they will stay here as changing this will cause the jacobian setup to be more messy right now
        Matrix<double, 5, 6> toGbl = Matrix<double, 5, 6>::Zero();
        Matrix<double, 6, 5> toProteus = Matrix<double, 6, 5>::Zero();
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
        auto tmp_local = plane->toLocal() * globalTrackPos;
        localPosTrack = Vector4d{tmp_local.x(), tmp_local.y(), tmp_local.z(), 1};
        localTangent = getRotation(plane->toLocal()) * globalTangent;
        double dist = localPosTrack[2];
        if(m_logging) {
            std::cout << getRotation(plane->toLocal()) << "\n local tan before normalization" << localTangent
                      << "\n distance: " << dist << std::endl;
        }
        localTangent /= localTangent.z();
        localPosTrack -= dist * localTangent;
        // add the local track pos for future reference - e.g. dut position:
        m_localTrackPoints[plane->name()] = ROOT::Math::XYPoint(localPosTrack(0), localPosTrack(1));

        Vector4d prevTan = getRotation(prevToLocal) * globalTangent;
        Matrix4d toTarget = getRotation(plane->toLocal()) * getRotation(prevToGlobal);

        auto myjac = jac(prevTan, toTarget, dist);
        double frac1 = 0.21, frac2 = 0.58;
        // Current layout
        // |        |        |       |
        // |  frac1 | frac2  | frac1 |
        // |        |        |       |

        // special treatment of first point on trajectory
        if(points.size() == 0) {
            myjac = Matrix<double, 6, 6>::Identity();
            myjac(0, 0) = 1;
            // Adding volume scattering if requested
        } else if(m_use_volume_scatter) {
            myjac = jac(prevTan, toTarget, frac1 * dist);
            GblPoint pVolume(toGbl * myjac * toProteus);
            addScattertoGblPoint(pVolume, fabs(dist) / 2. / m_scattering_length_volume);
            points.push_back(pVolume);
            // We have already rotated to the next local coordinate system
            myjac = jac(prevTan, Matrix4d::Identity(), frac2 * dist);
            GblPoint pVolume2(toGbl * myjac * toProteus);
            addScattertoGblPoint(pVolume2, fabs(dist) / 2. / m_scattering_length_volume);
            points.push_back(pVolume2);
            myjac = jac(prevTan, Matrix4d::Identity(), frac1 * dist);
        }
        auto transformedJac = toGbl * myjac * toProteus;
        GblPoint point(transformedJac);
        addScattertoGblPoint(point, plane->materialbudget());
        if(plane->hasCluster()) {
            addMeasurementtoGblPoint(point, plane);
        }
        prevToGlobal = plane->toGlobal();
        prevToLocal = plane->toLocal();
        points.push_back(point);
        plane->setGblPos(unsigned(points.size())); // gbl starts counting at 1
        globalTrackPos =                           // Constant switching between ROOT and EIGEN is really a pain...
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
        return;
    }

    // copy the results
    m_ndof = double(ndf);
    m_chi2ndof = (m_ndof < 0.0) ? -1 : (m_chi2 / m_ndof);
    VectorXd localPar(5);
    MatrixXd localCov(5, 5);
    VectorXd gblCorrection(5);
    MatrixXd gblCovariance(5, 5);
    VectorXd gblResiduals(2);
    VectorXd gblErrorsMeasurements(2);
    VectorXd gblErrorsResiduals(2);
    VectorXd gblDownWeights(2);
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
            // to be consistent with previous residuals:
            ROOT::Math::XYZPoint corPos =
                plane.toGlobal() *
                (ROOT::Math::XYZPoint(m_localTrackPoints.at(plane.name()).x() + m_corrections.at(plane.name()).x(),
                                      m_localTrackPoints.at(plane.name()).y() + m_corrections.at(plane.name()).y(),
                                      0));
            ROOT::Math::XYZPoint clusterPos = plane.cluster()->global();
            m_residual[plane.name()] = ROOT::Math::XYPoint(clusterPos.x() - corPos.x(), clusterPos.y() - corPos.y());
            // m_residual[plane.name()] = ROOT::Math::XYPoint(gblResiduals(0),gblResiduals(1));
            if(m_logging) {
                std::cout << "********* " << plane.name() << "\n Fitted res local:\t" << m_residual.at(plane.name())
                          << "\n seed res:\t" << m_inititalResidual.at(plane.name()) << " \n fitted res global:\t"
                          << ROOT::Math::XYPoint(clusterPos.x() - corPos.x(), clusterPos.y() - corPos.y()) << std::endl
                          << "*********\n";
            }
        }
        if(m_logging) {
            std::cout << "Plane: " << plane.name() << ": res" << m_residual[plane.name()]
                      << "\t kink: " << m_kink[plane.name()] << std::endl;
        }
    }
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
    // The track state is given in global coordinates and represents intersect of track and detetcor plane.
    // Let's check first if the data is fitted and all components are there
    if(!m_isFitted)
        throw TrackError(typeid(GblTrack), " detector " + detectorID + " state is not defined before fitting");
    if(m_localTrackPoints.count(detectorID) != 1) {
        throw TrackError(typeid(GblTrack), "Detector " + detectorID + " is not part of the GBL");
    }
    auto p =
        std::find_if(m_planes.begin(), m_planes.end(), [detectorID](auto plane) { return (plane.name() == detectorID); });
    return (p->toGlobal() * ROOT::Math::XYZPoint(m_localTrackPoints.at(detectorID).x() + correction(detectorID).x(),
                                                 m_localTrackPoints.at(detectorID).y() + correction(detectorID).y(),
                                                 0));
}

void GblTrack::set_seed_cluster(const Cluster* cluster) {
    m_seedCluster = const_cast<Cluster*>(cluster);
}

Cluster* GblTrack::get_seed_cluster() const {
    if(!m_seedCluster.IsValid() || m_seedCluster.GetObject() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return dynamic_cast<Cluster*>(m_seedCluster.GetObject());
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
