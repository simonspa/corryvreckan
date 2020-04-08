/**
 * @file
 * @brief Definition of GBL track object
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_GBLTRACK_H
#define CORRYVRECKAN_GBLTRACK_H 1

#include "Track.hpp"
namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief GblTrack object
     *
     * This class is a general broken line track which knows how to fit itself. It is dervied from Track
     */

    class GblTrack : public Track {

    public:
        // Constructors and destructors
        GblTrack();

        // copy constructor
        GblTrack(const GblTrack& track);

        virtual GblTrack* clone() const override { return new GblTrack(*this); }

        void print(std::ostream& out) const override;

        /**
         * @brief The fitting routine
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

    private:
        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(GblTrack, 0);
        std::map<std::string, ROOT::Math::XYPoint> m_localTrackPoints{};
        std::map<std::string, ROOT::Math::XYPoint> m_inititalResidual{};
    };

    // Vector type declaration
    using GblTrackVector = std::vector<GblTrack*>;

} // namespace corryvreckan

#endif // CORRYVRECKAN_GBLTRACK_H
