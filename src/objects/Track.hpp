#ifndef TRACK_H
#define TRACK_H 1

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TRef.h>

#include "Cluster.hpp"

namespace corryvreckan {
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
        Track(std::string model = "baseClass");
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
         * @brief  Get the distance between cluster and track
         * @param Cluster* Pointer to the cluster
         * @return distance between cluster and track
         */
        virtual double distance2(const Cluster*) const { return 0; }

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

        std::string trackModel() const { return m_trackModel; }

        bool isFitted() const { return m_isFitted; }

    protected:
        std::vector<TRef> m_trackClusters;
        std::vector<TRef> m_associatedClusters;
        TRef closestCluster{nullptr};
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        double m_momentum;
        bool m_isFitted{};
        std::string m_trackModel;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Track, 7)
    };

    // Vector type declaration
    using TrackVector = std::vector<Track*>;
} // namespace corryvreckan

// include all tracking methods here to have one header to be include everywhere
#include "StraightLineTrack.hpp"

#endif // TRACK_H
