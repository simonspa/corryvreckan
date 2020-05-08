/**
 * @file
 * @brief Definition of StraightLine track object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_STRAIGHTLINETRACK_H
#define CORRYVRECKAN_STRAIGHTLINETRACK_H 1

#include "Track.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief StraightLineTrack object
     *
     * This class is a simple track class which knows how to fit itself. It is dervied from Track
     */

    class StraightLineTrack : public Track {

    public:
        // Minimisation operator used by Minuit. Minuit passes the current iteration of the parameters and checks if the chi2
        // is better or worse
        double operator()(const double* parameters);

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
        ROOT::Math::XYZPoint getIntercept(double z) const override;

        /**
         * @brief Get the track state at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        ROOT::Math::XYZPoint getState(std::string) const override { return m_state; }

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        ROOT::Math::XYZVector getDirection(std::string) const override { return m_direction; }

        ROOT::Math::XYPoint distance(const Cluster* cluster) const;

    private:
        /**
         * @brief calculate the chi2 of the linear regression
         */
        void calculateChi2();

        void calculateResiduals();
        // Member variables
        ROOT::Math::XYZVector m_direction{0, 0, 1.};
        ROOT::Math::XYZPoint m_state{0, 0, 0.};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(StraightLineTrack, 1)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_STRAIGHTLINETRACK_H
