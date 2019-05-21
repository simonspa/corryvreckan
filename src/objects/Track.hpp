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
        Track(Track* track);

        // Add a new cluster to the track
        void addCluster(const Cluster* cluster);
        // Add a new cluster to the track (which will not be in the fit)
        void addAssociatedCluster(const Cluster* cluster);

        // Calculate the 2D distance^2 between the fitted track and a cluster
        double distance2(const Cluster* cluster) const;

        // Minimisation operator used by Minuit. Minuit passes the current iteration of the parameters and checks if the chi2
        // is better or worse
        double operator()(const double* parameters);

        // Fit the track (linear regression)
        void fit();

        // Retrieve track parameters
        double chi2() const { return m_chi2; }
        double chi2ndof() const { return m_chi2ndof; }
        double ndof() const { return m_ndof; }
        std::vector<Cluster*> clusters() const;
        std::vector<Cluster*> associatedClusters() const;
        bool isAssociated(Cluster* cluster) const;

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
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        ROOT::Math::XYZVector m_direction;
        ROOT::Math::XYZPoint m_state;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Track, 4)
    };

    // Vector type declaration
    typedef std::vector<Track*> Tracks;
} // namespace corryvreckan

#endif // TRACK_H
