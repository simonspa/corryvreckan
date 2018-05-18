#ifndef CLUSTER_H
#define CLUSTER_H 1

#include <iostream>
#include "Math/Point3D.h"
#include "Pixel.h"

/*

 This class is a simple cluster class which is used as a base class
 to interface with the track class. Anything which inherits from it
 can be placed on a track and used for fitting.

 */

namespace corryvreckan {

    class Cluster : public Object {

    public:
        // Constructors and destructors
        Cluster() {
            m_columnWidth = 0.;
            m_rowWidth = 0.;
            m_split = false;
        }
        virtual ~Cluster() {}
        // Copy constructor
        Cluster(Cluster* cluster) {
            m_global = cluster->global();
            m_local = cluster->local();
            m_error_x = cluster->errorX();
            m_error_y = cluster->errorY();
            m_detectorID = cluster->detectorID();
            m_timestamp = cluster->timestamp();
            m_columnWidth = cluster->columnWidth();
            m_rowWidth = cluster->rowWidth();
            m_split = cluster->isSplit();
        }

        // Functions
        // Add a new pixel to the cluster
        void addPixel(Pixel* pixel) {
            m_pixels.push_back(pixel);
            if(m_columnHits.count(pixel->column()) == 0) {
                m_columnWidth++;
            }
            if(m_rowHits.count(pixel->row()) == 0) {
                m_rowWidth++;
            }
            m_columnHits[pixel->column()] = true;
            m_rowHits[pixel->row()] = true;
        }
        // Retrieve cluster parameters
        // FIXME these should be renamed seed_row and seed_column!
        double row() { return m_row; }
        double column() { return m_column; }
        double tot() { return m_tot; }
        double error() { return sqrt(m_error_x * m_error_x + m_error_y * m_error_y); }
        double errorX() { return m_error_x; }
        double errorY() { return m_error_y; }

        bool isSplit() { return m_split; }
        void setSplit(bool split) { m_split = split; }

        double globalX() { return m_global.X(); }
        double globalY() { return m_global.Y(); }
        double globalZ() { return m_global.Z(); }
        ROOT::Math::XYZPoint global() { return m_global; }

        double localX() { return m_local.X(); }
        double localY() { return m_local.Y(); }
        double localZ() { return m_local.Z(); }
        ROOT::Math::XYZPoint local() { return m_local; }

        size_t size() { return m_pixels.size(); }
        double columnWidth() { return m_columnWidth; }
        double rowWidth() { return m_rowWidth; }
        Pixels* pixels() { return (&m_pixels); }

        // Retrieve the seed pixel of the cluster, defined as the one with the highest charge:
        Pixel* getSeedPixel() {
            Pixel* seed;
            double maxcharge = -1;
            for(auto& px : m_pixels) {
                if(px->charge() > maxcharge) {
                    maxcharge = px->charge();
                    seed = px;
                }
            }
            return seed;
        }

        // Set cluster parameters
        void setRow(double row) { m_row = row; }
        void setColumn(double col) { m_column = col; }
        void setTot(double tot) { m_tot = tot; }
        void setClusterCentre(ROOT::Math::XYZPoint global) { m_global = global; }
        void setClusterCentre(double x, double y, double z) {
            m_global.SetX(x);
            m_global.SetY(y);
            m_global.SetZ(z);
        }
        void setClusterCentreLocal(ROOT::Math::XYZPoint local) { m_local = local; }
        void setClusterCentreLocal(double x, double y, double z) {
            m_local.SetX(x);
            m_local.SetY(y);
            m_local.SetZ(z);
        }
        void setErrorX(double error) { m_error_x = error; }
        void setErrorY(double error) { m_error_y = error; }

    private:
        // Member variables
        Pixels m_pixels;
        double m_row;
        double m_column;
        double m_tot;
        double m_error_x;
        double m_error_y;
        double m_columnWidth;
        double m_rowWidth;
        bool m_split;

        ROOT::Math::XYZPoint m_local;
        ROOT::Math::XYZPoint m_global;

        std::map<int, bool> m_rowHits;
        std::map<int, bool> m_columnHits;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Cluster, 6)
    };

    // Vector type declaration
    typedef std::vector<Cluster*> Clusters;
} // namespace corryvreckan

#endif // CLUSTER_H
