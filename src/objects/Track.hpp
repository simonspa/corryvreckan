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
        static std::shared_ptr<Track> Factory(std::string trackModel);

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
        static std::type_index getBaseType();

        /**
         * @brief get the track type used
         * @return track type
         */
        std::string getType() const;

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
         * @param detectorID Name of the detector
         * @return Pointer to closest cluster to the Track if set, nullptr otherwise
         */
        Cluster* getClosestCluster(const std::string& detectorID) const;

        /**
         * @brief Check if this track has a closest cluster assigned to it for a given detector
         * @param detectorID Name of the detector
         * @return True if a closest cluster is set for this detector
         */
        bool hasClosestCluster(const std::string& detectorID) const;

        /**
         * @brief Print an ASCII representation of the Track to the given stream
         * @param ostream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief Set the momentum of the particle
         * @param momentum
         */
        void setParticleMomentum(double p);

        /**
         * @brief Get the chi2 of the track fit
         * @return chi2
         */
        double getChi2() const;

        /**
         * @brief Get chi2/ndof of the track fit
         * @return chi2/ndof
         */
        double getChi2ndof() const;

        /**
         * @brief Get the ndof for the track fit
         * @return ndof
         */
        double getNdof() const;

        /**
         * @brief Get the clusters contained in the track fit
         * @return vector of cluster* of track
         */
        std::vector<Cluster*> getClusters() const;

        /**
         * @brief Get the clusters associated to the track
         * @return vector of cluster* associated to the track
         */
        std::vector<Cluster*> getAssociatedClusters(const std::string& detectorID) const;

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
        bool hasDetector(const std::string& detectorID) const;

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
        size_t getNClusters() const { return track_clusters_.size(); }

        // virtual functions to be implemented by derived classes

        /**
         * @brief Track fitting routine
         */
        virtual void fit() = 0;

        /**
         * @brief Get the track position for a certain z position
         * @param z position
         * @return ROOT::Math::XYZPoint at z position
         */
        virtual ROOT::Math::XYZPoint getIntercept(double) const;

        /**
         * @brief Get the track state at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        virtual ROOT::Math::XYZPoint getState(const std::string&) const;

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        virtual ROOT::Math::XYZVector getDirection(const std::string&) const;

        /**
         * @brief Get the track direction at any given z position
         * @param global z position
         * @return ROOT::Math::XYZPoint direction at z
         */
        virtual ROOT::Math::XYZVector getDirection(const double&) const;

        /**
         * @brief check if the fitting routine already has been called. Chi2 etc are not set before
         * @return true if fit has already been performed, false otherwise
         */
        bool isFitted() const { return isFitted_; }

        /**
         * @brief Get the local residual for a given detector layer
         * @param detectorID
         * @return  2D local residual as ROOT::Math::XYPoint
         */
        ROOT::Math::XYPoint getLocalResidual(const std::string& detectorID) const;

        /**
         * @brief Get the global residual for a given detector layer
         * @param detectorID
         * @return  3D global residual as ROOT::Math::XYPoint
         */
        ROOT::Math::XYZPoint getGlobalResidual(const std::string& detectorID) const;

        /**
         * @brief Get the kink at a given detector layer. This is ill defined for last and first layer
         * @param  detectorID Detector ID at which the kink should be evaluated
         * @return  2D kink at given detector
         */
        virtual ROOT::Math::XYPoint getKinkAt(const std::string& detectorID) const = 0;

        /**
         * @brief Get the materialBudget of a detector layer
         * @param detectorID
         * @return Material Budget for given layer
         */
        double getMaterialBudget(const std::string& detectorID) const;

        ROOT::Math::XYZPoint getCorrection(const std::string& detectorID) const;

        virtual void setVolumeScatter(double length) = 0;

        void registerPlane(const std::string& name, double z, double x0, Transform3D g2l);

        class Plane {
        public:
            Plane() = default;
            Plane(std::string name, double z, double x_x0, Transform3D to_local);

            double getPosition() const;
            double getMaterialBudget() const;
            bool hasCluster() const;

            const std::string& getName() const;
            unsigned getGblPointPosition() const;
            Cluster* getCluster() const;
            Transform3D getToLocal() const;
            Transform3D getToGlobal() const;

            // sorting overload
            bool operator<(const Plane& pl) const;
            // set elements that might be unknown at construction
            void setGblPointPosition(unsigned pos);
            void setPosition(double z) { z_ = z; }
            void setCluster(const Cluster* cluster);
            void print(std::ostream& os) const;

        private:
            double z_, x_x0_;
            std::string name_;
            TRef cluster_;
            unsigned gbl_points_pos_{};
            Transform3D to_local_;
        };

    protected:
        Plane* get_plane(std::string detetorID);
        std::vector<TRef> track_clusters_;
        std::vector<TRef> associated_clusters_;
        std::map<std::string, ROOT::Math::XYPoint> residual_local_;
        std::map<std::string, ROOT::Math::XYZPoint> residual_global_;
        std::map<std::string, ROOT::Math::XYZPoint> corrections_{};

        std::vector<Plane> planes_;

        std::map<std::string, TRef> closest_cluster_;
        double chi2_;
        double ndof_;
        double chi2ndof_;
        bool isFitted_{};
        double momentum_{-1};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Track, 9)
    };
    // Vector type declaration
    using TrackVector = std::vector<std::shared_ptr<Track>>;
} // namespace corryvreckan

// include all tracking methods here to have one header to be include everywhere
#include "GblTrack.hpp"
#include "Multiplet.hpp"
#include "StraightLineTrack.hpp"
#endif // CORRYVRECKAN_TRACK_H
