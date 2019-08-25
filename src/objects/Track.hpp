#ifndef TRACK_H
#define TRACK_H 1

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

        // Copy constructor (also copies clusters from the original track)
        Track(const Track& track);

        // Add a new cluster to the track
        void addCluster(const Cluster* cluster);
        // Add a new cluster to the track (which will not be in the fit)
        void addAssociatedCluster(const Cluster* cluster);

        // Calculate the 2D distance^2 between the fitted track and a cluster
        double distance2(const Cluster* cluster) const;

        /**
         * @brief Set associated cluster with smallest distance to track
         * @param Pointer to Cluster cluster which has smallest distance to track
         */
        void setClosestCluster(const Cluster* cluster);

        /**
         * @brief Get associated cluster with smallest distance to track
         * @return Pointer to closest cluster to the track if set, nullptr otherwise
         */
        Cluster* getClosestCluster() const;

        /**
         * @brief Check if track has a closest cluster assigned to it
         * @return True if a closest cluster is set
         */
        bool hasClosestCluster() const;

        // Minimisation operator used by Minuit. Minuit passes the current iteration of the parameters and checks if the chi2
        // is better or worse
        double operator()(const double* parameters);

        // Fit the track (linear regression)
        void fit();

        // Print an ASCII representation of the Track to the given stream
        void print(std::ostream& out) const override;

        // Retrieve track parameters
        double chi2() const { return m_chi2; }
        double chi2ndof() const { return m_chi2ndof; }
        double ndof() const { return m_ndof; }
        std::vector<Cluster*> clusters() const;
        std::vector<Cluster*> associatedClusters() const;
        bool isAssociated(Cluster* cluster) const;

        /**
         * @brief Check if this track has a cluster from a given detector
         * @param  detectorID DetectorID of the detector to check
         * @return True if detector has a cluster on this track, false if not.
         */
        bool hasDetector(std::string detectorID) const;

        /**
         * @brief Get a track cluster from a given detector
         * @param  detectorID DetectorID of the desired detector
         * @return Track cluster from the required detector, nullptr if not found
         */
        Cluster* getClusterFromDetector(std::string detectorID) const;

        size_t nClusters() const { return m_trackClusters.size(); }

        ROOT::Math::XYZPoint intercept(double z) const;
        ROOT::Math::XYZPoint state() const { return m_state; }
        ROOT::Math::XYZVector direction() const { return m_direction; }

    private:
        // Calculate the chi2 of the track
        void calculateChi2();

        // Member variables
        std::vector<TRef> m_trackClusters;
        std::vector<TRef> m_associatedClusters;
        TRef closestCluster{nullptr};
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        ROOT::Math::XYZVector m_direction;
        ROOT::Math::XYZPoint m_state;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Track, 6)
    };

    // Vector type declaration
    using TrackVector = std::vector<Track*>;
} // namespace corryvreckan

#endif // TRACK_H
