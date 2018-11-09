#ifndef CLUSTER_H
#define CLUSTER_H 1

#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <iostream>

#include "Pixel.hpp"

/*

 This class is a simple cluster class which is used as a base class
 to interface with the track class. Anything which inherits from it
 can be placed on a track and used for fitting.

 */

namespace corryvreckan {

    class Cluster : public Object {

    public:
        // Constructors and destructors
        Cluster();

        // Copy constructor
        Cluster(Cluster* cluster);

        // Functions
        // Add a new pixel to the cluster
        void addPixel(Pixel* pixel);

        // Retrieve cluster parameters
        // FIXME these should be renamed seed_row and seed_column!
        double row() const { return m_row; }
        double column() const { return m_column; }
        double tot() const { return m_tot; }
        double error() const;
        double errorX() const { return m_error.X(); }
        double errorY() const { return m_error.Y(); }

        bool isSplit() const { return m_split; }
        void setSplit(bool split);

        ROOT::Math::XYZPoint global() const { return m_global; }
        ROOT::Math::XYZPoint local() const { return m_local; }

        size_t size() const { return m_pixels.size(); }
        double columnWidth() const { return m_columnWidth; }
        double rowWidth() const { return m_rowWidth; }
        Pixels* pixels() { return (&m_pixels); }

        // Retrieve the seed pixel of the cluster, defined as the one with the highest charge:
        Pixel* getSeedPixel();

        // Set cluster parameters
        void setRow(double row) { m_row = row; }
        void setColumn(double col) { m_column = col; }
        void setTot(double tot) { m_tot = tot; }
        void setClusterCentre(ROOT::Math::XYZPoint global) { m_global = global; }
        void setClusterCentreLocal(ROOT::Math::XYZPoint local) { m_local = local; }
        void setErrorX(double error) { m_error.SetX(error); }
        void setErrorY(double error) { m_error.SetY(error); }
        void setError(ROOT::Math::XYVector error) { m_error = error; }

    private:
        // Member variables
        Pixels m_pixels;
        double m_row;
        double m_column;
        double m_tot;
        ROOT::Math::XYVector m_error;
        double m_columnWidth;
        double m_rowWidth;
        bool m_split;

        ROOT::Math::XYZPoint m_local;
        ROOT::Math::XYZPoint m_global;

        std::map<int, bool> m_rowHits;
        std::map<int, bool> m_columnHits;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Cluster, 9)
    };

    // Vector type declaration
    typedef std::vector<Cluster*> Clusters;
} // namespace corryvreckan

#endif // CLUSTER_H
