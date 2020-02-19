#ifndef CORRYVRECKAN_KDTREE__H
#define CORRYVRECKAN_KDTREE__H 1

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

        KDTree(const KDTree& kd);

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(KDTree); }

        // Build a tree sorted by cluster times
        void buildTimeTree(ClusterVector inputClusters);

        // Build a tree sorted by cluster xy positions
        void buildSpatialTree(ClusterVector inputClusters);

        // Function to get back all clusters within a given time period
        ClusterVector getAllClustersInTimeWindow(Cluster* cluster, double timeWindow);

        // Function to get back all clusters within a given spatial window
        ClusterVector getAllClustersInWindow(Cluster* cluster, double window);

        // Function to get back the nearest cluster in space
        Cluster* getClosestNeighbour(Cluster* cluster);

    private:
        // Member variables
        double* xpositions; //!
        double* ypositions; //!
        double* times;      //!
        TKDTreeID* positionKdtree;
        TKDTreeID* timeKdtree;
        ClusterVector clusters;
        std::map<Cluster*, size_t> iteratorNumber;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(KDTree, 6)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_KDTREE__H
