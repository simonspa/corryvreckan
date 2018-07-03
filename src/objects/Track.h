#ifndef TRACK_H
#define TRACK_H 1

#include "Cluster.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"

/*

 This class is a simple track class which knows how to fit itself.
 It holds a collection of clusters, which may or may not be included
 in the track fit.

 */

namespace corryvreckan {

    class Track : public Object {

    public:
        // Constructors and destructors
        Track() {
            m_direction.SetZ(1.);
            m_state.SetZ(0.);
        }
        //    virtual ~Track() {}

        // Copy constructor (also copies clusters from the original track)
        Track(Track* track) {
            Clusters trackClusters = track->clusters();
            for(auto& track_cluster : trackClusters) {
                Cluster* cluster = new Cluster(track_cluster);
                m_trackClusters.push_back(cluster);
            }

            Clusters associatedClusters = track->associatedClusters();
            for(auto& assoc_cluster : associatedClusters) {
                Cluster* cluster = new Cluster(assoc_cluster);
                m_associatedClusters.push_back(cluster);
            }
            m_state = track->m_state;
            m_direction = track->m_direction;
        }

        // -----------
        // Functions
        // -----------

        // Add a new cluster to the track
        void addCluster(Cluster* cluster) { m_trackClusters.push_back(cluster); }
        // Add a new cluster to the track (which will not be in the fit)
        void addAssociatedCluster(Cluster* cluster) { m_associatedClusters.push_back(cluster); }

        // Calculate the 2D distance^2 between the fitted track and a cluster
        double distance2(Cluster* cluster) {

            // Get the track X and Y at the cluster z position
            double trackX = m_state.X() + m_direction.X() * cluster->globalZ();
            double trackY = m_state.Y() + m_direction.Y() * cluster->globalZ();

            // Calculate the 1D residuals
            double dx = (trackX - cluster->globalX());
            double dy = (trackY - cluster->globalY());

            // Return the distance^2
            return (dx * dx + dy * dy);
        }

        // Calculate the chi2 of the track
        void calculateChi2() {

            // Get the number of clusters
            m_ndof = static_cast<double>(m_trackClusters.size()) - 2.;
            m_chi2 = 0.;
            m_chi2ndof = 0.;

            // Loop over all clusters
            for(auto& cluster : m_trackClusters) {
                // Get the distance^2 and the error^2
                double error2 = cluster->error() * cluster->error();
                m_chi2 += (this->distance2(cluster) / error2);
            }

            // Store also the chi2/degrees of freedom
            m_chi2ndof = m_chi2 / m_ndof;
        }

        // Minimisation operator used by Minuit. Minuit passes the current iteration
        // of
        // the parameters and checks if the chi2 is better or worse
        double operator()(const double* parameters) {

            // Update the track gradient and intercept
            this->m_direction.SetX(parameters[0]);
            this->m_state.SetX(parameters[1]);
            this->m_direction.SetY(parameters[2]);
            this->m_state.SetY(parameters[3]);

            // Calculate the chi2
            this->calculateChi2();

            // Return this to minuit
            return m_chi2;
        }

        // Fit the track (linear regression)
        void fit() {

            double vecx[2] = {0., 0.};
            double vecy[2] = {0., 0.};
            double matx[2][2] = {{0., 0.}, {0., 0.}};
            double maty[2][2] = {{0., 0.}, {0., 0.}};

            // Loop over all clusters and fill the matrices
            for(auto& cluster : m_trackClusters) {
                // Get the global point details
                double x = cluster->globalX();
                double y = cluster->globalY();
                double z = cluster->globalZ();
                double er2 = cluster->error() * cluster->error();
                // Fill the matrices
                vecx[0] += x / er2;
                vecx[1] += x * z / er2;
                vecy[0] += y / er2;
                vecy[1] += y * z / er2;
                matx[0][0] += 1. / er2;
                matx[1][0] += z / er2;
                matx[1][1] += z * z / er2;
                maty[0][0] += 1. / er2;
                maty[1][0] += z / er2;
                maty[1][1] += z * z / er2;
            }

            // Invert the matrices
            double detx = matx[0][0] * matx[1][1] - matx[1][0] * matx[1][0];
            double dety = maty[0][0] * maty[1][1] - maty[1][0] * maty[1][0];

            // Check for singularities.
            if(detx == 0. || dety == 0.)
                return;

            // Get the track parameters
            double slopex = (vecx[1] * matx[0][0] - vecx[0] * matx[1][0]) / detx;
            double slopey = (vecy[1] * maty[0][0] - vecy[0] * maty[1][0]) / dety;

            double interceptx = (vecx[0] * matx[1][1] - vecx[1] * matx[1][0]) / detx;
            double intercepty = (vecy[0] * maty[1][1] - vecy[1] * maty[1][0]) / dety;

            // Set the track parameters
            m_state.SetX(interceptx);
            m_state.SetY(intercepty);
            m_state.SetZ(0.);

            m_direction.SetX(slopex);
            m_direction.SetY(slopey);
            m_direction.SetZ(1.);

            // Calculate the chi2
            this->calculateChi2();
        }

        // Retrieve track parameters
        double chi2() { return m_chi2; }
        double chi2ndof() { return m_chi2ndof; }
        double ndof() { return m_ndof; }
        Clusters clusters() { return m_trackClusters; }
        Clusters associatedClusters() { return m_associatedClusters; }
        size_t nClusters() { return m_trackClusters.size(); }
        ROOT::Math::XYZPoint intercept(double z) {
            ROOT::Math::XYZPoint point = m_state + m_direction * z;
            return point;
        }

        bool is_within_roi() { return m_region_of_interest; }
        void set_within_roi(bool roi) { m_region_of_interest = roi; }

        // Member variables
        Clusters m_trackClusters;
        Clusters m_associatedClusters;
        double m_chi2;
        double m_ndof;
        double m_chi2ndof;
        ROOT::Math::XYZPoint m_state;
        ROOT::Math::XYZVector m_direction;
        bool m_region_of_interest{false};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Track, 2)
    };

    // Vector type declaration
    typedef std::vector<Track*> Tracks;
} // namespace corryvreckan

#endif // TRACK_H
