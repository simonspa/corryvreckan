/**
 * @file
 * @brief Definition of cluster object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_CLUSTER_H
#define CORRYVRECKAN_CLUSTER_H 1

#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <TRef.h>

#include <iostream>

#include "Pixel.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief This class is a simple cluster class which is used as a base class to interface with the track class. Anything
     * which inherits from it can be placed on a track and used for fitting.
     */
    class Cluster : public Object {

    public:
        // Constructors and destructors
        Cluster();

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Cluster); }

        // Functions
        // Add a new pixel to the cluster
        void addPixel(const Pixel* pixel);

        // Retrieve cluster parameters
        double column() const { return m_column; }
        double row() const { return m_row; }
        double charge() const { return m_charge; }
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
        std::vector<const Pixel*> pixels() const;

        // Retrieve the seed pixel of the cluster, defined as the one with the highest charge:
        const Pixel* getSeedPixel() const;

        // Set cluster parameters
        void setColumn(double col) { m_column = col; }
        void setRow(double row) { m_row = row; }
        void setCharge(double charge) { m_charge = charge; }
        void setClusterCentre(ROOT::Math::XYZPoint global) { m_global = global; }
        void setClusterCentreLocal(ROOT::Math::XYZPoint local) { m_local = local; }
        void setErrorX(double error) { m_error.SetX(error); }
        void setErrorY(double error) { m_error.SetY(error); }
        void setError(ROOT::Math::XYVector error) { m_error = error; }

        /**
         * @brief Print an ASCII representation of Cluster to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

    private:
        // Member variables
        std::vector<TRef> m_pixels;
        double m_column;
        double m_row;
        double m_charge;
        ROOT::Math::XYVector m_error;
        double m_columnWidth;
        double m_rowWidth;
        bool m_split;

        ROOT::Math::XYZPoint m_local;
        ROOT::Math::XYZPoint m_global;

        std::map<int, bool> m_rowHits;
        std::map<int, bool> m_columnHits;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Cluster, 12)
    };

    // Vector type declaration
    using ClusterVector = std::vector<Cluster*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_CLUSTER_H
