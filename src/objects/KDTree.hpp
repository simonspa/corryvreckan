#ifndef KDTREE__H
#define KDTREE__H 1

#include <map>
#include "Cluster.hpp"
#include "Object.hpp"
#include "TKDTree.h"
#include "core/utils/log.h"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief This class is effectively just a wrapper for the root TKDTree class that handles  clusters and converts them
     * into the format needed by ROOT.
     */
    class KDTree : public Object {

    public:
        // Constructors and destructors
        KDTree() {
            timeKdtree = nullptr;
            positionKdtree = nullptr;
        }
        ~KDTree() {
            delete timeKdtree;
            delete positionKdtree;
        }

        // Build a tree sorted by cluster times
        void buildTimeTree(Clusters inputClusters);

        // Build a tree sorted by cluster xy positions
        void buildSpatialTree(Clusters inputClusters);

        // Function to get back all clusters within a given time period
        Clusters getAllClustersInTimeWindow(Cluster* cluster, double timeWindow);

        // Function to get back all clusters within a given spatial window
        Clusters getAllClustersInWindow(Cluster* cluster, double window);

        // Function to get back the nearest cluster in space
        Cluster* getClosestNeighbour(Cluster* cluster);

    private:
        // Member variables
        double* xpositions; //!
        double* ypositions; //!
        double* times;      //!
        TKDTreeID* positionKdtree;
        TKDTreeID* timeKdtree;
        Clusters clusters;
        std::map<Cluster*, size_t> iteratorNumber;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(KDTree, 5)
    };
} // namespace corryvreckan

#endif // KDTREE__H
