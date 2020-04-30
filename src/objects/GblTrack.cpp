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

GblTrack::GblTrack(const GblTrack& track) : Track(track) {
    if(track.getType() != this->getType())
        throw TrackModelChanged(typeid(*this), track.getType(), this->getType());
    m_planes = track.m_planes;
    m_kink = track.m_kink;
}

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
    m_planes.clear();
    double total_material = 0;
    for(auto l : m_materialBudget) {
        total_material += l.second.first;
        Plane current(l.second.second, l.second.first, l.first, clusters.count(l.first));
        if(current.hasCluster()) {
            current.setPosition(clusters.at(current.name())->global().z());
            current.setCluster(clusters.at(current.name()));
        }
        m_planes.push_back(current);
    }
    std::sort(m_planes.begin(), m_planes.end());
    // add volume scattering length - we ignore for now the material thickness while considering air
    if(m_use_volume_scatter) {
        total_material += (m_planes.back().position() - m_planes.front().position()) / m_scattering_length_volume;
    }

    std::vector<GblPoint> points;
    // get the seedcluster for the fit - find the first plane with a cluster to use
    setSeedCluster(std::find_if(m_planes.begin(), m_planes.end(), [](auto plane) { return plane.hasCluster(); })->cluster());
    double prevPos = m_planes.front().position();

    // lambda to calculate the scattering theta
    auto scatteringTheta = [this](double mbCurrent, double mbTotal) -> double {
        return (13.6 / m_momentum * sqrt(mbCurrent) * (1 + 0.038 * log(mbTotal)));
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
        auto cluster = p->cluster();
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
        double dist = plane->position() - prevPos;
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
        addScattertoGblPoint(point, plane->materialbudget());
        if(plane->hasCluster()) {
            addMeasurementtoGblPoint(point, plane);
        }
        prevPos = plane->position();
        points.push_back(point);
        plane->setGblPos(unsigned(points.size())); // gbl starts counting at 1
    };

    // First GblPoint
    std::vector<Plane>::iterator pl = m_planes.begin();
    auto point = GblPoint(Matrix5d::Identity());
    addScattertoGblPoint(point, pl->materialbudget());
    if(pl->hasCluster()) {
        addMeasurementtoGblPoint(point, pl);
    }
    points.push_back(point);
    pl->setGblPos(1);
    pl++;
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
        if(l.position() >= z) {
            found = true;
            break;
        }
        layer = l.name();
    }
    // Two cases to not return an intercept and throw an error
    // Most upstream plane has larger z (layer == "") -> asked for intercept in front of telescope
    // We do not find a plane with larger z (found == false) -> ased for intercept behind telescope
    if(!found || layer == "") {
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
    return ROOT::Math::XYZPoint(getSeedCluster()->global().x() + correction(detectorID).x(),
                                getSeedCluster()->global().y() + correction(detectorID).y(),
                                m_materialBudget.at(detectorID).second);
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
