#ifndef CLUSTER_H
#define CLUSTER_H 1

#include <iostream>
#include "Pixel.h"

/*

 This class is a simple cluster class which is used as a base class
 to interface with the track class. Anything which inherits from it
 can be placed on a track and used for fitting.

 */

using namespace corryvreckan;

class Cluster : public TestBeamObject {

public:
    // Constructors and destructors
    Cluster() {
        m_columnWidth = 0.;
        m_rowWidth = 0.;
    }
    virtual ~Cluster() {}
    // Copy constructor
    Cluster(Cluster* cluster) {
        m_globalX = cluster->globalX();
        m_globalY = cluster->globalY();
        m_globalZ = cluster->globalZ();
        m_localX = cluster->localX();
        m_localY = cluster->localY();
        m_localZ = cluster->localZ();
        m_error = cluster->error();
        m_detectorID = cluster->detectorID();
        m_timestamp = cluster->timestamp();
        m_columnWidth = cluster->columnWidth();
        m_rowWidth = cluster->rowWidth();
    }

    // Functions
    // Add a new pixel to the cluster
    void addPixel(Pixel* pixel) {
        m_pixels.push_back(pixel);
        if(m_columnHits.count(pixel->m_column) == 0) {
            m_columnWidth++;
        }
        if(m_rowHits.count(pixel->m_row) == 0) {
            m_rowWidth++;
        }
        m_columnHits[pixel->m_column] = true;
        m_rowHits[pixel->m_row] = true;
    }
    // Retrieve cluster parameters
    double row() { return m_row; }
    double column() { return m_column; }
    double tot() { return m_tot; }
    double error() { return m_error; }
    double globalX() { return m_globalX; }
    double globalY() { return m_globalY; }
    double globalZ() { return m_globalZ; }
    double localX() { return m_localX; }
    double localY() { return m_localY; }
    double localZ() { return m_localZ; }
    size_t size() { return m_pixels.size(); }
    double columnWidth() { return m_columnWidth; }
    double rowWidth() { return m_rowWidth; }
    Pixels* pixels() { return (&m_pixels); }

    // Set cluster parameters
    void setRow(double row) { m_row = row; }
    void setColumn(double col) { m_column = col; }
    void setTot(double tot) { m_tot = tot; }
    void setClusterCentre(double x, double y, double z) {
        m_globalX = x;
        m_globalY = y;
        m_globalZ = z;
    }
    void setClusterCentreLocal(double x, double y, double z) {
        m_localX = x;
        m_localY = y;
        m_localZ = z;
    }
    void setError(double error) { m_error = error; }

private:
    // Member variables
    Pixels m_pixels;
    double m_row;
    double m_column;
    double m_tot;
    double m_error;
    double m_columnWidth;
    double m_rowWidth;
    double m_globalX;
    double m_globalY;
    double m_globalZ;
    double m_localX;
    double m_localY;
    double m_localZ;
    std::map<int, bool> m_rowHits;
    std::map<int, bool> m_columnHits;

    // ROOT I/O class definition - update version number when you change this
    // class!
    ClassDef(Cluster, 3)
};

// Vector type declaration
typedef std::vector<Cluster*> Clusters;

#endif // CLUSTER_H
