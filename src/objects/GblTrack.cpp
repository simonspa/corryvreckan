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

#include "GblPoint.h"
#include "GblTrajectory.h"

using namespace corryvreckan;
using namespace gbl;

GblTrack::GblTrack() : Track() {}

ROOT::Math::XYPoint GblTrack::getKinkAt(std::string detectorID) const {
    if(m_kink.count(detectorID) == 1) {
        return m_kink.at(detectorID);
    } else {
        return ROOT::Math::XYPoint(0, 0);
    }
}

void GblTrack::setVolumeScatter(double length) {
    m_scattering_length_volume = length;
    m_use_volume_scatter = true;
}

void GblTrack::fit() {

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
    planes_.clear();
    double total_material = 0;
    for(auto l : material_budget_) {
        total_material += l.second.first;
        Plane current(l.second.second, l.second.first, l.first, clusters.count(l.first));
        if(current.hasCluster()) {
            current.setPosition(clusters.at(current.getName())->global().z());
            current.setCluster(clusters.at(current.getName()));
        }
        planes_.push_back(current);
    }
    std::sort(planes_.begin(), planes_.end());
    // add volume scattering length - we ignore for now the material thickness while considering air
    if(m_use_volume_scatter) {
        total_material +=
            (planes_.back().getPlanePosition() - planes_.front().getPlanePosition()) / m_scattering_length_volume;
    }

    std::vector<GblPoint> points;
    // get the seedcluster for the fit - find the first plane with a cluster to use
    setSeedCluster(
        std::find_if(planes_.begin(), planes_.end(), [](auto plane) { return plane.hasCluster(); })->getCluster());
    double prevPos = planes_.front().getPlanePosition();

    // lambda to calculate the scattering theta
    auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
        return (13.6 / momentum_ * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
    };
    // lambda to add measurement (if existing) and scattering from single plane
    auto addScattertoGblPoint = [&total_material, &scatteringTheta](GblPoint& point, double material) {
        Eigen::Vector2d scatterWidth;
        scatterWidth(0) = 1 / (scatteringTheta(material, total_material) * scatteringTheta(material, total_material));
        scatterWidth(1) = scatterWidth(0);
        point.addScatterer(Eigen::Vector2d::Zero(), scatterWidth);
    };
    auto JacToNext = [](double val) {
        Matrix5d Jac;
        Jac = Matrix5d::Identity();
        Jac(3, 1) = val;
        Jac(4, 2) = Jac(3, 1);
        return Jac;
    };
    auto addMeasurementtoGblPoint = [this](GblPoint& point, std::vector<Plane>::iterator& p) {
        auto cluster = p->getCluster();
        Eigen::Vector2d initialResidual;
        initialResidual(0) = cluster->global().x() - getSeedCluster()->global().x();
        initialResidual(1) = cluster->global().y() - getSeedCluster()->global().y();
        // FIXME uncertainty of single hit - rotations ignored
        Eigen::Matrix2d covv = Eigen::Matrix2d::Identity();
        covv(0, 0) = 1. / cluster->errorX() / cluster->errorX();
        covv(1, 1) = 1. / cluster->errorY() / cluster->errorY();
        point.addMeasurement(initialResidual, covv);
    };
    // lambda to add plane (not the first one) and air scatterers //FIXME: Where to put them?
    auto addPlane = [&JacToNext, &prevPos, &addMeasurementtoGblPoint, &addScattertoGblPoint, &points, this](
                        std::vector<Plane>::iterator& plane) {
        double dist = plane->getPlanePosition() - prevPos;
        double frac1 = 0.21, frac2 = 0.58;
        // Current layout
        // |        |        |       |
        // |  frac1 | frac2  | frac1 |
        // |        |        |       |

        // first air scatterer:
        auto point = GblPoint(JacToNext(frac1 * dist));
        addScattertoGblPoint(point, dist / 2. / m_scattering_length_volume);
        if(m_use_volume_scatter) {
            points.push_back(point);
            point = GblPoint(JacToNext(frac2 * dist));
            addScattertoGblPoint(point, dist / 2. / m_scattering_length_volume);
            points.push_back(point);
        } else {
            frac1 = 1;
        }
        point = GblPoint(JacToNext(frac1 * dist));
        addScattertoGblPoint(point, plane->getMaterialBudget());
        if(plane->hasCluster()) {
            addMeasurementtoGblPoint(point, plane);
        }
        prevPos = plane->getPlanePosition();
        points.push_back(point);
        plane->setGblPointPosition(unsigned(points.size())); // gbl starts counting at 1
    };

    // First GblPoint
    std::vector<Plane>::iterator pl = planes_.begin();
    auto point = GblPoint(Matrix5d::Identity());
    addScattertoGblPoint(point, pl->getMaterialBudget());
    if(pl->hasCluster()) {
        addMeasurementtoGblPoint(point, pl);
    }
    points.push_back(point);
    pl->setGblPointPosition(1);
    pl++;
    // add all other points
    for(; pl != planes_.end(); ++pl) {
        addPlane(pl);
    }

    // Make sure we missed nothing
    if((points.size() != ((material_budget_.size() * 3) - 2) && m_use_volume_scatter) ||
       (points.size() != material_budget_.size() && !m_use_volume_scatter)) {
        throw TrackError(typeid(GblTrack),
                         "Number of planes " + std::to_string(material_budget_.size()) +
                             " doesn't match number of GBL points on trajectory " + std::to_string(points.size()));
    }
    // perform fit
    GblTrajectory traj(points, false); // false = no magnetic field
    double lostWeight = 0;
    int ndf = 0;
    // fit it
    auto fitReturnValue = traj.fit(chi2_, ndf, lostWeight);
    if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
        fitReturnValue = traj.fit(chi2_, ndf, lostWeight);
        if(fitReturnValue != 0) { // Is this a good ieda? should we discard track candidates that fail?
            return;
            //            throw TrackFitError(typeid(GblTrack), "internal GBL Error " + std::to_string(fitReturnValue));
        }
    }

    // copy the results
    ndof_ = double(ndf);
    chi2ndof_ = (ndof_ < 0.0) ? -1 : (chi2_ / ndof_);
    Eigen::VectorXd localPar(5);
    Eigen::MatrixXd localCov(5, 5);
    Eigen::VectorXd gblCorrection(5);
    Eigen::MatrixXd gblCovariance(5, 5);
    Eigen::VectorXd gblResiduals(2);
    Eigen::VectorXd gblErrorsMeasurements(2);
    Eigen::VectorXd gblErrorsResiduals(2);
    Eigen::VectorXd gblDownWeights(2);
    unsigned int numData = 2;
    for(auto plane : planes_) {
        traj.getScatResults(
            plane.getGblPointPosition(), numData, gblResiduals, gblErrorsMeasurements, gblErrorsResiduals, gblDownWeights);
        m_kink[plane.getName()] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));

        traj.getResults(int(plane.getGblPointPosition()), localPar, localCov);
        corrections_[plane.getName()] = ROOT::Math::XYZPoint(localPar(3), localPar(4), 0);
        if(plane.hasCluster()) {
            traj.getMeasResults(plane.getGblPointPosition(),
                                numData,
                                gblResiduals,
                                gblErrorsMeasurements,
                                gblErrorsResiduals,
                                gblDownWeights);
            residual_[plane.getName()] = ROOT::Math::XYPoint(gblResiduals(0), gblResiduals(1));
        }
    }
    isFitted_ = true;
}

ROOT::Math::XYZPoint GblTrack::getIntercept(double z) const {
    // find the detector with largest z-positon <= z, assumes detectors sorted by z position
    std::string layer = "";
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

ROOT::Math::XYZPoint GblTrack::getState(std::string detectorID) const {
    // The track state at any plane is the seed (always first cluster for now) plus the correction for the plane
    // And as rotations are ignored, the z position is simply the detectors z position
    // Let's check first if the data is fitted and all components are there

    if(!isFitted_)
        throw TrackError(typeid(GblTrack), " detector " + detectorID + " state is not defined before fitting");
    if(material_budget_.count(detectorID) != 1) {
        std::string list;
        for(auto l : material_budget_)
            list.append("\n " + l.first + " with <materialBudget, z position>(" + std::to_string(l.second.first) + ", " +
                        std::to_string(l.second.second) + ")");
        throw TrackError(typeid(GblTrack),
                         " detector " + detectorID + " is not appearing in the material budget map" + list);
    }
    if(corrections_.count(detectorID) != 1)
        throw TrackError(typeid(GblTrack), " detector " + detectorID + " is not appearing in the corrections map");

    // Using the global detector position here is of course not correct, it works for small/no rotations
    // For larger rotations it is an issue
    return ROOT::Math::XYZPoint(getSeedCluster()->global().x() + getCorrection(detectorID).x(),
                                getSeedCluster()->global().y() + getCorrection(detectorID).y(),
                                material_budget_.at(detectorID).second);
}

void GblTrack::setSeedCluster(const Cluster* cluster) {
    m_seedCluster = const_cast<Cluster*>(cluster);
}

Cluster* GblTrack::getSeedCluster() const {
    if(!m_seedCluster.IsValid() || m_seedCluster.GetObject() == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(Cluster));
    }
    return dynamic_cast<Cluster*>(m_seedCluster.GetObject());
}

ROOT::Math::XYZVector GblTrack::getDirection(std::string detectorID) const {

    // Defining the direction following the particle results in the direction
    // beeing definded from the requested plane onwards to the next one
    ROOT::Math::XYZPoint point = getState(detectorID);

    // searching for the next detector layer
    bool found = false;
    std::string nextLayer = "";
    for(auto& layer : material_budget_) {
        if(found) {
            nextLayer = layer.first;
            break;
        } else if(layer.first == detectorID) {
            found = true;
        }
    }
    if(nextLayer == "")
        throw TrackError(typeid(GblTrack), "Direction after the last telescope plane not defined");
    ROOT::Math::XYZPoint pointAfter = getState(nextLayer);
    return ((pointAfter - point) / (pointAfter.z() - point.z()));
}

void GblTrack::print(std::ostream& out) const {
    out << "GblTrack with nhits = " << track_clusters_.size() << " and nscatterers = " << material_budget_.size()
        << ", chi2 = " << chi2_ << ", ndf = " << ndof_;
}
