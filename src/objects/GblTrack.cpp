/**
 * @file
 * @brief Implementation of GBL track object with rotations. The logic is inspired by the implementation in proteus (Kiehn,
 * Moritz et al., Proteus beam telescope reconstruction, doi:10.5281/zenodo.2579153)
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
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

// clang-format off
Eigen::Matrix<double, 5, 6> GblTrack::toGbl = (Eigen::Matrix<double, 5, 6>() << 0., 0., 0., 0., 0., 1.,
                                                                                0., 0., 0., 1., 0., 0.,
                                                                                0., 0., 0., 0., 1., 0.,
                                                                                1., 0., 0., 0., 0., 0.,
                                                                                0., 1., 0., 0., 0., 0.).finished();
Eigen::Matrix<double, 6, 5> GblTrack::toProt =(Eigen::Matrix<double, 6, 5>() << 0., 0., 0., 1., 0.,
                                                                                0., 0., 0., 0., 1.,
                                                                                0., 0., 0., 0., 0.,
                                                                                0., 1., 0., 0., 0.,
                                                                                0., 0., 1., 0., 0.,
                                                                                1., 0., 0., 0., 0.).finished();
// clang-format on

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

void GblTrack::add_plane(std::vector<Plane>::iterator& plane,
                         Transform3D& prevToGlobal,
                         Transform3D& prevToLocal,
                         ROOT::Math::XYZPoint& globalTrackPos,
                         double total_material) {
    // lambda to add plane (not the first one) and air scatterers
    auto globalTangent = Vector4d(0, 0, 1, 0);

    // extract the rotation from an ROOT::Math::Transfrom3D, store it  in 4x4 matrix to match proteus format
    auto getRotation = [](Transform3D in) {
        Matrix4d t = Matrix4d::Zero();
        in.Rotation().GetRotationMatrix(t);
        return t;
    };

    // Mapping of parameters in proteus - I would like to get rid of these conversions once it works
    // For now they will stay here as changing this will cause the jacobian setup to be more messy right now
    auto tmp_local = plane->getToLocal() * globalTrackPos;
    Vector4d localPosTrack = Vector4d{tmp_local.x(), tmp_local.y(), tmp_local.z(), 1};
    Vector4d localTangent = getRotation(plane->getToLocal()) * globalTangent;
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

    // lambda Jacobian from one scatter to the next
    auto jac = [](const Vector4d& tangent, const Matrix4d& target, double distance) {
        Matrix<double, 4, 3> R;
        R.col(0) = target.col(0);
        R.col(1) = target.col(1);
        R.col(2) = target.col(3);
        Vector4d S = target * tangent * (1 / tangent[2]);

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
    auto myjac = jac(prevTan, toTarget, dist);

    // Layout if volume scattering active
    // |        |        |       |
    // |  frac1 | frac2  | frac1 |
    // |        |        |       |
    double frac1 = 0.21, frac2 = 0.58;

    // lambda to add a scatterer to a GBLPoint
    auto addScattertoGblPoint = [this, &total_material, &localTangent](GblPoint& point, double material) {
        Matrix<double, 2, 2> scatter;

        // lambda to calculate the scattering theta, beta2 assumed to be one and the momentum in MeV
        auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
            return (13.6 / momentum_ * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
        };

        // This can only happen if someone messes up the tracking code. Simply renormalizing would shadow the mistake made at
        // a different position and therefore the used plane distances would be wrong.
        if(localTangent(2) != 1) {
            throw TrackError(typeid(GblTrack),
                             "wrong normalization of local slope, should be 1 but is " + std::to_string(localTangent(2)));
        }
        Vector2d localSlope(localTangent(0), localTangent(1));
        auto scale =
            1 / scatteringTheta(material * (1 + localSlope.squaredNorm()), total_material) / (1 + localSlope.squaredNorm());
        scatter(0, 0) = 1 + localSlope(1) * localSlope(1);
        scatter(1, 1) = 1 + localSlope(0) * localSlope(0);
        scatter(0, 1) = scatter(1, 0) = -(localSlope(0) * localSlope(1));
        scatter *= (scale * scale);
        point.addScatterer(Vector2d::Zero(), scatter);
    };

    // special treatment of first point on trajectory
    if(gblpoints_.empty()) {
        myjac = Matrix<double, 6, 6>::Identity();
        myjac(0, 0) = 1;
        // Adding volume scattering if requested
    } else if(use_volume_scatter_) {
        myjac = jac(prevTan, toTarget, frac1 * dist);
        GblPoint pVolume(toGbl * myjac * toProt);
        addScattertoGblPoint(pVolume, fabs(dist) / 2. / scattering_length_volume_);
        gblpoints_.push_back(pVolume);
        // We have already rotated to the next local coordinate system
        myjac = jac(prevTan, Matrix4d::Identity(), frac2 * dist);
        GblPoint pVolume2(toGbl * myjac * toProt);
        addScattertoGblPoint(pVolume2, fabs(dist) / 2. / scattering_length_volume_);
        gblpoints_.push_back(pVolume2);
        myjac = jac(prevTan, Matrix4d::Identity(), frac1 * dist);
    }
    auto transformedJac = toGbl * myjac * toProt;
    GblPoint point(transformedJac);
    addScattertoGblPoint(point, plane->getMaterialBudget());

    auto addMeasurementtoGblPoint = [&localTangent, &localPosTrack, &globalTrackPos, this](GblPoint& pt,
                                                                                           std::vector<Plane>::iterator& p) {
        auto* cluster = p->getCluster();
        Vector2d initialResidual;
        initialResidual(0) = cluster->local().x() - localPosTrack[0];
        initialResidual(1) = cluster->local().y() - localPosTrack[1];
        // Uncertainty of single hit in local coordinates
        Matrix2d covv = Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        pt.addMeasurement(initialResidual, covv);
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

    if(plane->hasCluster()) {
        addMeasurementtoGblPoint(point, plane);
    }
    prevToGlobal = plane->getToGlobal();
    prevToLocal = plane->getToLocal();
    gblpoints_.push_back(point);
    plane_to_gblpoint_[plane->getName()] = unsigned(gblpoints_.size()); // gbl starts counting at 1
    globalTrackPos = // Constant switching between ROOT and EIGEN is really a pain...
        plane->getToGlobal() *
        ROOT::Math::XYZPoint(localPosTrack(0), localPosTrack(1), localPosTrack(2)); // reference slope stays unchanged
}

void GblTrack::prepare_gblpoints() {

    gblpoints_.clear();

    // store the used clusters in a map for easy access:
    std::map<std::string, Cluster*> clusters;
    for(auto& c : track_clusters_) {
        auto* cluster = c.get();
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
        std::find_if(planes_.begin(), planes_.end(), [](const auto& pl) { return pl.hasCluster(); })->getCluster());

    // add volume scattering length - for now simply the distance between first and last plane
    if(use_volume_scatter_) {
        total_material += (planes_.back().getPosition() - planes_.front().getPosition()) / scattering_length_volume_;
    }

    // Prepare transformations for first plane:
    auto prevToGlobal = planes_.front().getToGlobal();
    auto prevToLocal = planes_.front().getToLocal();
    auto globalTrackPos = get_seed_cluster()->global();
    globalTrackPos.SetZ(0);

    // First GblPoint
    auto pl = planes_.begin();
    // add all other points
    for(; pl != planes_.end(); ++pl) {
        add_plane(pl, prevToGlobal, prevToLocal, globalTrackPos, total_material);
    }

    // Make sure we missed nothing
    // Case 1: We take the material into account which adds a scatterer left and right of each detetcor. The very first
    // and very last one are not relevant and therefore ignored
    // Case 2: Each plane is a scatter,volume ignored
    if((gblpoints_.size() != ((planes_.size() * 3) - 2) && use_volume_scatter_) ||
       (gblpoints_.size() != planes_.size() && !use_volume_scatter_)) {
        throw TrackError(typeid(GblTrack),
                         "Number of planes " + std::to_string(planes_.size()) +
                             " doesn't match number of GBL points on trajectory " + std::to_string(gblpoints_.size()));
    }
}

void GblTrack::fit() {

    LOG(DEBUG) << "Starting GBL fit";
    isFitted_ = false;
    residual_local_.clear();
    residual_global_.clear();
    kink_.clear();
    local_track_points_.clear();
    plane_to_gblpoint_.clear();

    // Fitting with less than 2 clusters is pointless
    if(track_clusters_.size() < 2) {
        throw TrackError(typeid(GblTrack), " attempting to fit a track with less than 2 clusters");
    }

    prepare_gblpoints();

    // perform fit
    GblTrajectory traj(gblpoints_, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;

    // Let's not print GBL's internal messages:
    IFNOTLOG(DEBUG) { SUPPRESS_STREAM(std::cout); }
    auto fitReturnValue = traj.fit(chi2_, ndf, lostWeight);
    RELEASE_STREAM(std::cout);
    if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
        switch(fitReturnValue) {
        case 1:
            LOG(DEBUG) << "GBL fit failed: VMatrix is singular";
            break;
        case 2:
            LOG(DEBUG) << "GBL fit failed: Bordered band matrix is singular";
            break;
        case 3:
            LOG(DEBUG) << "GBL fit failed: Bordered band matrix is not positive definite";
            break;
        case 10:
            LOG(DEBUG) << "GBL fit failed: Inner transformation matrix with invalid number of rows";
            break;
        case 11:
            LOG(DEBUG) << "GBL fit failed: Inner transformation matrix with to few columns";
            break;
        case 12:
            LOG(DEBUG) << "GBL fit failed: Inner transformation matrices with varying sizes";
            break;
        default:
            LOG(DEBUG) << "GBL fit failed, unknown reason";
            break;
        }
        return;
    }

    // copy the results
    ndof_ = static_cast<size_t>(ndf);
    chi2ndof_ = (ndof_ <= 0) ? -1 : (chi2_ / static_cast<double>(ndof_));
    VectorXd localPar(5);
    MatrixXd localCov(5, 5);
    VectorXd gblCorrection(5);
    MatrixXd gblCovariance(5, 5);
    VectorXd gblResiduals(2);
    VectorXd gblErrorsMeasurements(2);
    VectorXd gblErrorsResiduals(2);
    VectorXd gblDownWeights(2);
    unsigned int numData = 2;

    for(const auto& plane : planes_) {
        const auto& name = plane.getName();
        auto gbl_id = plane_to_gblpoint_[plane.getName()];
        traj.getScatResults(gbl_id, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        // fixme: Kinks are in local coordinates and would be more reasonably in global
        kink_[name] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        traj.getResults(static_cast<int>(gbl_id), localPar, localCov);
        local_fitted_track_points_[name] = ROOT::Math::XYZPoint(
            local_track_points_.at(name).x() + localPar(3), local_track_points_.at(name).y() + localPar(4), 0);

        if(plane.hasCluster()) {
            traj.getMeasResults(gbl_id, numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
            // to be consistent with previous residuals global ones here:

            auto corPos = plane.getToGlobal() * local_fitted_track_points_.at(name);
            ROOT::Math::XYZPoint clusterPos = plane.getCluster()->global();
            residual_global_[name] = clusterPos - corPos;
            residual_local_[plane.getName()] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));

            LOG(TRACE) << "Results for detector  " << name << std::endl
                       << "Fitted residual local:\t" << residual_local_.at(name) << std::endl
                       << "Seed residual:\t" << initital_residual_.at(name) << std::endl
                       << "Ditted residual global:\t" << ROOT::Math::XYPoint(clusterPos - corPos);
        }
        LOG(DEBUG) << "Plane: " << name << ": residual " << residual_local_[name] << ", kink: " << kink_[name];
    }
    isFitted_ = true;
}

ROOT::Math::XYZPoint GblTrack::getIntercept(double z) const {
    // find the detector with largest z-positon <= z, assumes detectors sorted by z position
    std::string layer;
    LOG(TRACE) << "Requesting intercept at: " << z;
    bool found = false;

    if(!isFitted_) {
        throw TrackError(typeid(GblTrack), "An interception is requested before the track has been fitted");
    }

    for(const auto& l : planes_) {
        if(l.getPosition() >= z) {
            found = true;
            break;
        }
        layer = l.getName();
    }
    // Two cases to not return an intercept and throw an error
    // Most upstream plane has larger z (layer == "") -> asked for intercept in front of telescope
    // We do not find a plane with larger z (found == false) -> ased for intercept behind telescope
    // We have been asked to allow extrapolation outside the coverage
    if(!found || layer.empty()) {
        LOG_N(DEBUG, 10)
            << "Requesting extrapolation outside the telescope coverage. Scattering at first/last plane set to zero";
        return get_position_outside_telescope(z);
    }

    return (getState(layer) + getDirection(layer) * (z - getState(layer).z()));
}

ROOT::Math::XYZPoint GblTrack::getState(const std::string& detectorID) const {
    // The track state is given in global coordinates and represents intersect of track and detetcor plane.
    // Let's check first if the data is fitted and all components are there
    LOG(TRACE) << "Requesting state at: " << detectorID;
    if(!isFitted_) {
        throw TrackError(typeid(GblTrack), " has no defined state for " + detectorID + " before fitting");
    }
    if(local_fitted_track_points_.count(detectorID) != 1) {
        throw TrackError(typeid(GblTrack), " does not have any entry for detector " + detectorID);
    }
    // The local track position can simply be transformed to global coordinates
    auto p = std::find_if(
        planes_.begin(), planes_.end(), [detectorID](const auto& plane) { return (plane.getName() == detectorID); });

    return (p->getToGlobal() * local_fitted_track_points_.at(detectorID));
}

void GblTrack::set_seed_cluster(const Cluster* cluster) {
    seed_cluster_ = PointerWrapper<Cluster>(cluster);
}

Cluster* GblTrack::get_seed_cluster() const {
    if(seed_cluster_.get() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return seed_cluster_.get();
}

XYZPoint GblTrack::get_position_outside_telescope(double z) const {
    // most up and downstream plane
    auto first_plane = planes_.begin();
    auto last_plane = planes_.end();
    last_plane--; // No direct iterator for last_plane element
    // check if z is up or downstream
    bool upstream = (z < first_plane->getPosition());

    auto outerPlane = (upstream ? first_plane->getName() : last_plane->getName());
    // inner neighbour of plane - simply adjust the iterators
    first_plane++;
    last_plane--;
    auto innerPlane = (upstream ? last_plane->getName() : last_plane->getName());
    // connect the states to get the direction
    XYZVector direction =
        (upstream ? getState(outerPlane) - getState(innerPlane) : getState(innerPlane) - getState(outerPlane));
    // scale to get a slope
    direction /= direction.z();
    // return extrapolated position
    return (getState(outerPlane) + direction * (z - getState(outerPlane).z()));
}

ROOT::Math::XYZVector GblTrack::getDirection(const std::string& detectorID) const {

    // Defining the direction following the particle results in the direction
    // being defined from the requested plane onwards to the next one
    ROOT::Math::XYZPoint point = getState(detectorID);
    LOG(TRACE) << "Requesting direction at: " << detectorID;

    // searching for the next detector layer
    auto plane =
        std::find_if(planes_.begin(), planes_.end(), [&detectorID](const auto& p) { return p.getName() == detectorID; });
    plane++;
    // If we are at the end we have no kink -> take the last two palbes
    if(plane == planes_.end()) {
        plane -= 2;
        ROOT::Math::XYZPoint pointBefore = getState(plane->getName());
        return ((point - pointBefore) / (point.z() - pointBefore.z()));
    }

    ROOT::Math::XYZPoint pointAfter = getState(plane->getName());
    return ((pointAfter - point) / (pointAfter.z() - point.z()));
}

XYZVector GblTrack::getDirection(const double& z) const {
    auto planeUpstream = std::find_if(planes_.begin(), planes_.end(), [&z](const Plane& p) { return p.getPosition() > z; });
    if(planeUpstream != planes_.end()) {
        return getDirection(planeUpstream->getName());
    } else {
        return getDirection(planes_.back().getName());
    }
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << track_clusters_.size() << " and nscatterers = " << planes_.size()
        << ", chi2 = " << chi2_ << ", ndf = " << ndof_;
}
