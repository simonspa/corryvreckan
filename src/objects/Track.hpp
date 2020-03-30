/**
 * @file
 * @brief Definition of track base object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_TRACK_H
#define CORRYVRECKAN_TRACK_H 1

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Transform3D.h>
#include <Math/Vector3D.h>
#include <TRef.h>

#include "Cluster.hpp"
#include "core/utils/type.h"
namespace corryvreckan {
    using namespace ROOT::Math;
    class Plane : public Object {
    public:
        Plane() : Object(){};

        Plane(double z, double x_x0, std::string name, bool has_cluster)
            : Object(), m_z(z), m_x_x0(x_x0), m_name(name), m_has_cluster(has_cluster){};

        Plane(const Plane& p);
        // access elements
        double position() const { return m_z; }
        double materialbudget() const { return m_x_x0; }
        bool hasCluster() const { return m_has_cluster; }
        std::string name() const { return m_name; }
        unsigned gblPos() const { return m_gbl_points_pos; }

        // sorting overload
        // bool operator<(const Plane& pl) const { return m_z < pl.m_z; }
        // set elements that might be unknown at construction
        void setGblPos(unsigned pos) { m_gbl_points_pos = pos; }
        void setPosition(double z) { m_z = z; }
        void setCluster(const Cluster* cluster) {
            m_cluster = const_cast<Cluster*>(cluster);
            m_has_cluster = true;
        }
        Cluster* cluster() const { return m_cluster; }
        void print(std::ostream& os) const override {
            os << "Plane at " << m_z << " with rad. length " << m_x_x0 << " and cluster: " << (m_has_cluster) << std::endl;
        }
        void setToLocal(Transform3D toLocal) { m_toLocal = toLocal; }
        void setToGlobal(Transform3D toGlobal) { m_toGlobal = toGlobal; }
        Transform3D toLocal() const { return m_toLocal; }
        Transform3D toGlobal() const { return m_toGlobal; }

        static std::type_index getBaseType() { return typeid(Plane); }

    private:
        double m_z, m_x_x0;
        std::string m_name;
        bool m_has_cluster = false;
        Cluster* m_cluster = nullptr;
        unsigned m_gbl_points_pos{};
        Transform3D m_toLocal, m_toGlobal;
        ClassDefOverride(Plane, 1)
    };

    /**
     * @ingroup Objects
     * @brief Track object
     *
     * This class is a simple track class which knows how to fit itself. It holds a collection of clusters, which may or may
     * not be included in the track fit.
     */

    class Track : public Object {

    public:
        /**
         * @brief Factory to dynamically create track objects
         * @param The name of the track model which should be used
         * @return By param trackModel assigned track model to be used
         */
        static Track* Factory(std::string trackModel);

        /**
         * @brief Track object constructor
         */
        Track();
        /**
         * @brief Copy a track object, including used/associated clusters
         * @param track to be copied from
         */
        Track(const Track& track);

        /**
         * @brief Add a cluster to the tack, which will be used in the fit
         * @param cluster to be added
         */

        /** @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Track); }

        /**
         * @brief get the track type used
         * @return track type
         */
        std::string getType() const { return corryvreckan::demangle(typeid(*this).name()); }

        /**
         * * @brief Add a cluster to the tack, which will be used in the fit
         * * @param cluster to be added */
        void addCluster(const Cluster* cluster);

        /**
         * @brief Associate a cluster to a track, will not be part of the fit
         * @param cluster to be added
         */
        void addAssociatedCluster(const Cluster* cluster);

        /**
         * @brief Set associated cluster with smallest distance to Track
         * @param Pointer to Cluster cluster which has smallest distance to Track
         */
        void setClosestCluster(const Cluster* cluster);

        /**
         * @brief Get associated cluster with smallest distance to Track
         * @return Pointer to closest cluster to the Track if set, nullptr otherwise
         */
        Cluster* getClosestCluster() const;

        /**
         * @brief Check if Track has a closest cluster assigned to it
         * @return True if a closest cluster is set
         */
        bool hasClosestCluster() const;

        /**
         * @brief Print an ASCII representation of the Track to the given stream
         * @param ostream to print to
         */
        void print(std::ostream& out) const override { out << "Base class - nothing to see here" << std::endl; }

        /**
         * @brief Set the momentum of the particle
         * @param momentum
         */
        void setParticleMomentum(double p) { m_momentum = p; }

        /**
         * @brief Get the chi2 of the track fit
         * @return chi2
         */
        double chi2() const;

        /**
         * @brief Get chi2/ndof of the track fit
         * @return chi2/ndof
         */
        double chi2ndof() const;

        /**
         * @brief Get the ndof for the track fit
         * @return ndof
         */
        double ndof() const;

        /**
         * @brief Get the clusters contained in the track fit
         * @return vector of cluster* of track
         */
        std::vector<Cluster*> clusters() const;

        /**
         * @brief Get the clusters associated to the track
         * @return vector of cluster* assosiated to the track
         */
        std::vector<Cluster*> associatedClusters() const;

        /**
         * @brief Check if cluster is associated
         * @param Pointer to the clusterto be checked
         * @return True if the cluster is associated to the track, false if not.
         */
        bool isAssociated(Cluster* cluster) const;

        /**
         * @brief Check if this Track has a cluster from a given detector
         * @param  detectorID DetectorID of the detector to check
         * @return True if detector has a cluster on this Track, false if not.
         */
        bool hasDetector(std::string detectorID) const;

        /**
         * @brief Get a Track cluster from a given detector
         * @param  detectorID DetectorID of the desired detector
         * @return Track cluster from the required detector, nullptr if not found
         */
        Cluster* getClusterFromDetector(std::string detectorID) const;

        /**
         * @brief Get the number of clusters used for track fit
         * @return Number of clusters in track
         */
        size_t nClusters() const { return m_trackClusters.size(); }

        void useVolumeScatterer(bool use) { m_use_volume_scatter = use; }
        // virtual functions to be implemented by derived classes

        /**
         * @brief The fiting routine
         */
        virtual void fit(){};
        /**
         * @brief Virtual function to copy a class
         * @return pointer to copied object
         */
        virtual Track* clone() const { return new Track(); }

        /**
         * @brief Get the track position for a certain z position
         * @param z positon
         * @return ROOT::Math::XYZPoint at z position
         */
        virtual ROOT::Math::XYZPoint intercept(double) const { return ROOT::Math::XYZPoint(0.0, 0.0, 0.0); }

        /**
         * @brief Get the track state at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        virtual ROOT::Math::XYZPoint state(std::string) const { return ROOT::Math::XYZPoint(0.0, 0.0, 0.0); }

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        virtual ROOT::Math::XYZVector direction(std::string) const { return ROOT::Math::XYZVector(0.0, 0.0, 0.0); }

        /**
         * @brief check if the fitting routine already has been called. Chi2 etc are not set before
         * @return true if fit has already been performed, false otherwise
         */
        bool isFitted() const { return m_isFitted; }

        /**
         * @brief Get the residual for a given detector layer
         * @param detectorID
         * @return  2D residual as ROOT::Math::XYPoint
         */
        ROOT::Math::XYPoint residual(std::string detectorID) const { return m_residual.at(detectorID); }

        /**
         * @brief Get the kink at a given detector layer. This is ill defined for last and first layer
         * @param detectorID
         * @return  2D kink as ROOT::Math::XYPoint
         */
        ROOT::Math::XYPoint kink(std::string detectorID) const;

        /**
         * @brief Get the materialBudget of a detector layer
         * @param detectorID
         * @return Material Budget for given layer
         */
        double materialBudget(std::string detectorID) const { return m_materialBudget.at(detectorID).first; }

        void registerPlane(Plane p) { m_planes.push_back(p); }
        void updatePlane(Plane p);

        void addMaterial(std::string detetcorID, double x_x0, double z);

        ROOT::Math::XYZPoint correction(std::string detectorID) const;

        long unsigned int numScatterers() const { return m_materialBudget.size(); }
        void setVolumeScatter(double length) { m_scattering_length_volume = length; }

        void setLogging(bool on = false) { m_logging = on; }

    protected:
        std::vector<TRef> m_trackClusters;
        std::vector<TRef> m_associatedClusters;
        std::map<std::string, ROOT::Math::XYPoint> m_residual;
        std::map<std::string, std::pair<double, double>> m_materialBudget;
        std::map<std::string, ROOT::Math::XYPoint> m_kink;
        std::map<std::string, ROOT::Math::XYZPoint> m_corrections{};
        std::vector<Plane> m_planes{};
        bool m_logging = false;

        TRef closestCluster{nullptr};
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        double m_momentum;
        double m_scattering_length_volume;
        bool m_isFitted{};
        bool m_use_volume_scatter{};
        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Track, 7)
    };
    // Vector type declaration
    using TrackVector = std::vector<Track*>;
} // namespace corryvreckan

// include all tracking methods here to have one header to be include everywhere
#include "GblTrack.hpp"
#include "StraightLineTrack.hpp"
#endif // CORRYVRECKAN_TRACK_H
