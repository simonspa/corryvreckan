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
        // Constructors and destructors
        Track();

        // Copy constructor (also copies clusters from the original Track)
        Track(const Track& track);

        // Add a new cluster to the Track
        void addCluster(const Cluster* cluster);

        // Add a new cluster to the Track (which will not be in the fit)
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

        // Print an ASCII representation of the Track to the given stream
        void print(std::ostream& out) const override { out << "Base class - nothing to see here" << std::endl; }

        // set particle momentum
        void setParticleMomentum(double p) { m_momentum = p; }

        // Retrieve Track parameters
        double chi2() const { return m_chi2; }
        double chi2ndof() const { return m_chi2ndof; }
        double ndof() const { return m_ndof; }
        std::vector<Cluster*> clusters() const;
        std::vector<Cluster*> associatedClusters() const;
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

        size_t nClusters() const { return m_trackClusters.size(); }

        // virtual functions to be implemented by derived classes
        // Fit the Track (linear regression)
        virtual void fit(){};
        // Calculate the 2D distance^2 between the fitted Track and a cluster
        virtual double distance2(const Cluster*) const { return 0; }
        virtual ROOT::Math::XYZPoint intercept(double) const { return ROOT::Math::XYZPoint(0.0, 0.0, 0.0); }
        virtual ROOT::Math::XYZPoint state(std::string) const { return ROOT::Math::XYZPoint(0.0, 0.0, 0.0); }
        virtual ROOT::Math::XYZVector direction(std::string) const { return ROOT::Math::XYZVector(0.0, 0.0, 0.0); }

    protected:
        std::vector<TRef> m_trackClusters;
        std::vector<TRef> m_associatedClusters;
        TRef closestCluster{nullptr};
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        double m_momentum;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Track, 7)
    };

    // Vector type declaration
    using TrackVector = std::vector<Track*>;
} // namespace corryvreckan

#endif // TRACK_H
