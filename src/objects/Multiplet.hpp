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

#include <Math/DisplacementVector3D.h>
#include "StraightLineTrack.hpp"
#include "Track.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Multiplet object
     *
     * This class is a simple Multiplet class which knows how to fit itself via two straight line and a kink. It holds a
     * collection of clusters, which may or may not be included in the Multiplet fit.
     */

    class Multiplet : public Track {

    public:
        /**
         * @brief Multiplet object constructor
         */
        Multiplet();
        /**
         * @brief Copy a Multiplet object, including used/associated clusters
         * @param Multiplet to be copied from
         */
        Multiplet(const Multiplet& multiplet);

        Multiplet(Track* upstream, Track* downstream);

        virtual Multiplet* clone() const override { return new Multiplet(*this); }

        void print(std::ostream& out) const override;

        /**
         * @brief The fiting routine
         */
        void fit() override;

        /**
         * @brief Get the track position for a certain z position
         * @param z positon
         * @return ROOT::Math::XYZPoint at z position
         */
        ROOT::Math::XYZPoint intercept(double z) const override;

        /**
         * @brief Get the track state at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        ROOT::Math::XYZPoint state(std::string detectorID) const override;

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        ROOT::Math::XYZVector direction(std::string detectorID) const override;

        ROOT::Math::XYVector getOffsetAtScatterer() { return m_offsetAtScatterer; };

        ROOT::Math::XYVector getKinkAtScatterer() { return m_kinkAtScatterer; };

        void setScattererPosition(double scattererPosition) { m_scattererPosition = scattererPosition; };

    private:
        Track* m_upstream;
        Track* m_downstream;

        void calculateChi2();

        double m_scattererPosition;
        ROOT::Math::XYZVector m_positionAtScatterer;
        ROOT::Math::XYVector m_offsetAtScatterer;
        ROOT::Math::XYVector m_kinkAtScatterer;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Multiplet, 1)
    };
    // Vector type declaration
    using MultipletVector = std::vector<Multiplet*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_Multiplet_H
