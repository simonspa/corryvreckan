/**
 * @file
 * @brief Definition of Multiplet base object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_Multiplet_H
#define CORRYVRECKAN_Multiplet_H 1

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TRef.h>

#include "Cluster.hpp"
#include "StraightLineTrack.hpp"
#include "core/utils/type.h"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Multiplet object
     *
     * This class is a simple Multiplet class which knows how to fit itself via two straight line and a kink. It holds a
     * collection of clusters, which may or may not be included in the Multiplet fit.
     */

    class Multiplet : public Object {

    public:
        /**
         * @brief Multiplet object constructor
         */
        Multiplet();
        /**
         * @brief Copy a Multiplet object, including used/associated clusters
         * @param Multiplet to be copied from
         */
        Multiplet(const Multiplet& Multiplet);

        /**
         * @brief Add a cluster to the tack, which will be used in the fit
         * @param cluster to be added
         */
        void addCluster(const Cluster* cluster);
        /** @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Multiplet); }

    private:
        StraightLineTrack upstream, downstream;
        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Multiplet, 1)
    };
    // Vector type declaration
    using MultipletVector = std::vector<Multiplet*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_Multiplet_H
