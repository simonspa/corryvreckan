/**
 * @file
 * @brief Implementation of GBL track object with rotations. The logic is inspired by the implementation in proteus (Kiehn,
 * Moritz et al., Proteus beam telescope reconstruction, doi:10.5281/zenodo.2579153)
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <GblPoint.h>
#include <GblTrajectory.h>
#include <Math/Point3D.h>
#include <Math/Vector3D.h>

#include "GblTrack.hpp"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace corryvreckan;
using namespace gbl;
using namespace Eigen;

ROOT::Math::XYPoint GblTrack::getKinkAt(const std::string& detectorID) const {
    if(kink_.count(detectorID) == 1) {
        return kink_.at(detectorID);
    } else {
        return ROOT::Math::XYPoint(0, 0);
    }
}

void GblTrack::setVolumeScatter(double length) {
    scattering_length_volume_ = length;
    use_volume_scatter_ = true;
}

void GblTrack::fit() {
    LOG(DEBUG) << "Starting GBL fit";
    isFitted_ = false;
    residual_.clear();
    kink_.clear();
    corrections_.clear();
    local_track_points_.clear();

    // Fitting with less than 2 clusters is pointless
    if(track_clusters_.size() < 2) {
        throw TrackError(typeid(GblTrack), " attempting to fit a track with less than 2 clusters");
    }
    // store the used clusters in a map for easy access:
    std::map<std::string, Cluster*> clusters;
    for(auto& c : track_clusters_) {
        auto cluster = dynamic_cast<Cluster*>(c.GetObject());
        clusters[cluster->detectorID()] = cluster;
    }
    // create a list of planes and sort it, also calculate the material budget:
    double total_material = 0;
    std::sort(planes_.begin(), planes_.end());
    for(auto& l : planes_) {
        total_material += l.getMaterialBudget();
        if(clusters.count(l.getName()) == 1) {
            l.setPosition(clusters.at(l.getName())->global().z());
            l.setCluster(clusters.at(l.getName()));
        }
    }
    // get the seedcluster for the fit - find the first plane with a cluster to use
    set_seed_cluster(
        std::find_if(planes_.begin(), planes_.end(), [](auto plane) { return plane.hasCluster(); })->getCluster());

    // add volume scattering length - for now simply the distance between first and last plane
    if(use_volume_scatter_) {
        total_material +=
            (planes_.back().getPlanePosition() - planes_.front().getPlanePosition()) / scattering_length_volume_;
    }

    std::vector<GblPoint> points;

    // lambda to calculate the scattering theta, beta2 assumed to be one and the momentum in MeV
    auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
        return (13.6 / momentum_ * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
    };

    // extract the rotation from an ROOT::Math::Transfrom3D, store it  in 4x4 matrix to match proteus format
    auto getRotation = [](Transform3D in) {
        Matrix4d t = Matrix4d::Zero();
        in.Rotation().GetRotationMatrix(t);
        return t;
    };

    auto prevToGlobal = planes_.front().getToGlobal();
    auto prevToLocal = planes_.front().getToLocal();
    auto globalTrackPos = get_seed_cluster()->global();
    globalTrackPos.SetZ(0);
    auto globalTangent = Vector4d(0, 0, 1, 0);
    Vector4d localPosTrack;
    Vector4d localTangent;

    // lambda to add a scatterer to a GBLPoint
    auto addScattertoGblPoint = [&total_material, &scatteringTheta, &localTangent](GblPoint& point, double material) {
        Matrix<double, 2, 2> scatter;
        // This can only happen if someone messes up the tracking code. Simply renormalizing would shadow the mistake made at
        // a different position and therefore the used plane distances would be wrong.
        if(localTangent(2) != 1)
            throw TrackError(typeid(GblTrack),
                             "wrong normalization of local slope, should be 1 but is " + std::to_string(localTangent(2)));
        Vector2d localSlope(localTangent(0), localTangent(1));
        auto scale =
            1 / scatteringTheta(material * (1 + localSlope.squaredNorm()), total_material) / (1 + localSlope.squaredNorm());
        scatter(0, 0) = 1 + localSlope(1) * localSlope(1);
        scatter(1, 1) = 1 + localSlope(0) * localSlope(0);
        scatter(0, 1) = scatter(1, 0) = -(localSlope(0) * localSlope(1));
        scatter *= (scale * scale);
        point.addScatterer(Vector2d::Zero(), scatter);
    };
    // lambda Jacobian from one scatter to the next
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
        jaco(5, 5) = 1; // a future time component
        return jaco;
    };
    auto addMeasurementtoGblPoint = [&localTangent, &localPosTrack, &globalTrackPos, this](GblPoint& point,
                                                                                           std::vector<Plane>::iterator& p) {
        auto cluster = p->getCluster();
        Vector2d initialResidual;
        initialResidual(0) = cluster->local().x() - localPosTrack[0];
        initialResidual(1) = cluster->local().y() - localPosTrack[1];
        // Uncertainty of single hit in local coordinates
        Matrix2d covv = Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        point.addMeasurement(initialResidual, covv);
        initital_residual_[p->getName()] = ROOT::Math::XYPoint(initialResidual(0), initialResidual(1));
        LOG(TRACE) << "Plane:  " << p->getName() << std::endl
                   << "Global Res to fit: \t(" << (cluster->global() - planes_.begin()->getCluster()->global()) << ")"
                   << std::endl
                   << "Local Res to fit: \t(" << (cluster->local().x() - localPosTrack[0]) << ", "
                   << (cluster->local().y() - localPosTrack[1]) << ")" << std::endl
                   << "Local  track pos: \t(" << localPosTrack[0] << ", " << localPosTrack[1] << ", " << localPosTrack[2]
                   << ", " << localPosTrack[3] << ")" << std::endl
                   << "global track Pos:\t" << globalTrackPos << std::endl
                   << "local tangent:\t\t(" << localTangent[0] << ", " << localTangent[1] << ", " << localTangent[2] << ", "
                   << localTangent[3] << ")" << std::endl
                   << "cluster global:\t " << p->getCluster()->global() << std::endl
                   << "cluster local:\t" << p->getCluster()->local();
    };

    // lambda to add plane (not the first one) and air scatterers
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
        auto tmp_local = plane->getToLocal() * globalTrackPos;
        localPosTrack = Vector4d{tmp_local.x(), tmp_local.y(), tmp_local.z(), 1};
        localTangent = getRotation(plane->getToLocal()) * globalTangent;
        double dist = localPosTrack[2];
        LOG(TRACE) << "Rotation: " << getRotation(plane->getToLocal());
        LOG(TRACE) << "Local tan before normalization: " << localTangent;
        LOG(TRACE) << "Distance: " << dist;

        localTangent /= localTangent.z();
        localPosTrack -= dist * localTangent;
        // add the local track pos for future reference - e.g. dut position:
        local_track_points_[plane->getName()] = ROOT::Math::XYPoint(localPosTrack(0), localPosTrack(1));

        Vector4d prevTan = getRotation(prevToLocal) * globalTangent;
        Matrix4d toTarget = getRotation(plane->getToLocal()) * getRotation(prevToGlobal);

        auto myjac = jac(prevTan, toTarget, dist);

        // Layout if volume scattering active
        // |        |        |       |
        // |  frac1 | frac2  | frac1 |
        // |        |        |       |

        double frac1 = 0.21, frac2 = 0.58;
        // special treatment of first point on trajectory
        if(points.size() == 0) {
            myjac = Matrix<double, 6, 6>::Identity();
            myjac(0, 0) = 1;
            // Adding volume scattering if requested
        } else if(use_volume_scatter_) {
            myjac = jac(prevTan, toTarget, frac1 * dist);
            GblPoint pVolume(toGbl * myjac * toProteus);
            addScattertoGblPoint(pVolume, fabs(dist) / 2. / scattering_length_volume_);
            points.push_back(pVolume);
            // We have already rotated to the next local coordinate system
            myjac = jac(prevTan, Matrix4d::Identity(), frac2 * dist);
            GblPoint pVolume2(toGbl * myjac * toProteus);
            addScattertoGblPoint(pVolume2, fabs(dist) / 2. / scattering_length_volume_);
            points.push_back(pVolume2);
            myjac = jac(prevTan, Matrix4d::Identity(), frac1 * dist);
        }
        auto transformedJac = toGbl * myjac * toProteus;
        GblPoint point(transformedJac);
        addScattertoGblPoint(point, plane->getMaterialBudget());
        if(plane->hasCluster()) {
            addMeasurementtoGblPoint(point, plane);
        }
        prevToGlobal = plane->getToGlobal();
        prevToLocal = plane->getToLocal();
        points.push_back(point);
        plane->setGblPointPosition(unsigned(points.size())); // gbl starts counting at 1
        globalTrackPos =                                     // Constant switching between ROOT and EIGEN is really a pain...
            plane->getToGlobal() *
            ROOT::Math::XYZPoint(localPosTrack(0), localPosTrack(1), localPosTrack(2)); // reference slope stays unchanged
    };

    // First GblPoint
    std::vector<Plane>::iterator pl = planes_.begin();
    // add all other points
    for(; pl != planes_.end(); ++pl) {
        addPlane(pl);
    }

    // Make sure we missed nothing
    // Case 1: We take the material into account which adds a scatterer left and right of each detetcor. The very first
    // and very last one are not relevant and therefore ignored
    // Case 2: Each plane is a scatter,volume ignored
    if((points.size() != ((planes_.size() * 3) - 2) && use_volume_scatter_) ||
       (points.size() != planes_.size() && !use_volume_scatter_)) {
        throw TrackError(typeid(GblTrack),
                         "Number of planes " + std::to_string(planes_.size()) +
                             " doesn't match number of GBL points on trajectory " + std::to_string(points.size()));
    }

    // perform fit
    GblTrajectory traj(points, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;
    auto fitReturnValue = traj.fit(chi2_, ndf, lostWeight);
    if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
        LOG(DEBUG) << "GBL failed with return value " << fitReturnValue;
        return;
    }

    // copy the results
    ndof_ = double(ndf);
    chi2ndof_ = (ndof_ < 0.0) ? -1 : (chi2_ / ndof_);
    VectorXd localPar(5);
    MatrixXd localCov(5, 5);
    VectorXd gblCorrection(5);
    MatrixXd gblCovariance(5, 5);
    VectorXd gblResiduals(2);
    VectorXd gblErrorsMeasurements(2);
    VectorXd gblErrorsResiduals(2);
    VectorXd gblDownWeights(2);
    unsigned int numData = 2;

    for(auto plane : planes_) {
        auto name = plane.getName();
        traj.getScatResults(
            plane.getGblPointPosition(), numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        // fixme: Kinks are in local coordinates and would be more reasonably in global
        kink_[name] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        traj.getResults(int(plane.getGblPointPosition()), localPar, localCov);
        corrections_[name] = ROOT::Math::XYZPoint(localPar(3), localPar(4), 0);
        if(plane.hasCluster()) {
            traj.getMeasResults(plane.getGblPointPosition(),
                                numData,
                                gblResiduals,
                                gblErrorsMeasurements,
                                gblErrorsResiduals,
                                gblDownWeights);
            // to be consistent with previous residuals global ones here:
            ROOT::Math::XYZPoint corPos =
                plane.getToGlobal() * (ROOT::Math::XYZPoint(local_track_points_.at(name).x() + corrections_.at(name).x(),
                                                            local_track_points_.at(name).y() + corrections_.at(name).y(),
                                                            0));
            ROOT::Math::XYZPoint clusterPos = plane.getCluster()->global();
            residual_[name] = ROOT::Math::XYPoint(clusterPos.x() - corPos.x(), clusterPos.y() - corPos.y());
            // m_residual[plane.name()] = ROOT::Math::XYPoint(gblResiduals(0),gblResiduals(1));
            LOG(TRACE) << "Results for detector  " << name << std::endl
                       << "Fitted residual local:\t" << residual_.at(name) << std::endl
                       << "Seed residual:\t" << initital_residual_.at(name) << std::endl
                       << "Ditted residual global:\t" << ROOT::Math::XYPoint(clusterPos - corPos);
        }
        LOG(DEBUG) << "Plane: " << name << ": residual " << residual_[name] << ", kink: " << kink_[name];
    }
    isFitted_ = true;
}

ROOT::Math::XYZPoint GblTrack::getIntercept(double z) const {
    // find the detector with largest z-positon <= z, assumes detectors sorted by z position
    std::string layer = "";
    LOG(DEBUG) << "Requesting intercept at: " << z;
    bool found = false;

    if(!isFitted_) {
        throw TrackError(typeid(GblTrack), "An interception is requested befor the track is fitted");
    }

    for(auto l : planes_) {
        if(l.getPlanePosition() >= z) {
            found = true;
            break;
        }
        layer = l.getName();
    }
    // Two cases to not return an intercept and throw an error
    // Most upstream plane has larger z (layer == "") -> asked for intercept in front of telescope
    // We do not find a plane with larger z (found == false) -> ased for intercept behind telescope
    if(!found || layer == "") {
        throw TrackError(typeid(GblTrack), "Z-Position of " + std::to_string(z) + " is outside the telescopes z-coverage");
    }
    return (getState(layer) + getDirection(layer) * (z - getState(layer).z()));
}

ROOT::Math::XYZPoint GblTrack::getState(const std::string& detectorID) const {
    // The track state is given in global coordinates and represents intersect of track and detetcor plane.
    // Let's check first if the data is fitted and all components are there
    LOG(DEBUG) << "Requesting state at: " << detectorID;
    if(!isFitted_)
        throw TrackError(typeid(GblTrack), " has no difned state for" + detectorID + " before fitting");
    if(local_track_points_.count(detectorID) != 1) {
        throw TrackError(typeid(GblTrack), "  does not have any entry for detector " + detectorID);
    }
    // The local track position can simply be transformed to global coordinates
    auto p =
        std::find_if(planes_.begin(), planes_.end(), [detectorID](auto plane) { return (plane.getName() == detectorID); });

    return (p->getToGlobal() * ROOT::Math::XYZPoint(local_track_points_.at(detectorID).x() + getCorrection(detectorID).x(),
                                                    local_track_points_.at(detectorID).y() + getCorrection(detectorID).y(),
                                                    0));
}

void GblTrack::set_seed_cluster(const Cluster* cluster) {
    seed_cluster_ = const_cast<Cluster*>(cluster);
}

Cluster* GblTrack::get_seed_cluster() const {
    if(!seed_cluster_.IsValid() || seed_cluster_.GetObject() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return dynamic_cast<Cluster*>(seed_cluster_.GetObject());
}

ROOT::Math::XYZVector GblTrack::getDirection(const std::string& detectorID) const {

    // Defining the direction following the particle results in the direction
    // beeing definded from the requested plane onwards to the next one
    ROOT::Math::XYZPoint point = getState(detectorID);
    LOG(DEBUG) << "Requesting direction at: " << detectorID;

    // searching for the next detector layer - fixme: can this be done nicer?
    bool found = false;
    std::string nextLayer = "";
    for(auto& layer : planes_) {
        if(found) {
            nextLayer = layer.getName();
            break;
        } else if(layer.getName() == detectorID) {
            found = true;
        }
    }
    if(nextLayer == "")
        throw TrackError(typeid(GblTrack), " does not define a direction after the last telescope plane");
    ROOT::Math::XYZPoint pointAfter = getState(nextLayer);
    return ((pointAfter - point) / (pointAfter.z() - point.z()));
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << track_clusters_.size() << " and nscatterers = " << planes_.size()
        << ", chi2 = " << chi2_ << ", ndf = " << ndof_;
}
