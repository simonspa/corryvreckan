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
        Multiplet(std::shared_ptr<Track> upstream, std::shared_ptr<Track> downstream);

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
        ROOT::Math::XYZPoint getState(const std::string& detectorID) const override;

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        ROOT::Math::XYZVector getDirection(const std::string& detectorID) const override;

        /**
         * @brief Get the track direction at any given z position
         * @param global z position
         * @return ROOT::Math::XYZPoint direction at z
         */
        ROOT::Math::XYZVector getDirection(const double& z) const override;

        /**
         * @brief Get the offset between up- & downstream tracklet at the position of the scatterer
         * @return ROOT::Math::XYVector offset at scatterer
         */
        ROOT::Math::XYVector getOffsetAtScatterer() { return m_offsetAtScatterer; };

        /**
         * @brief Set the position of the scatterer along z
         * @param position of the scatterer along z
         */
        void setScattererPosition(double scattererPosition) { m_scattererPosition = scattererPosition; };

        /**
         * @brief Get the track intercept at the position of the scatterer
         * @return ROOT::Math::XYPoint Track position at scatterer
         */
        ROOT::Math::XYZPoint getPositionAtScatterer() { return m_positionAtScatterer; };

        /**
         * @brief Get the kink angle between up- & downstream tracklet at the position of the scatterer
         * @return ROOT::Math::XYVector kink at scatterer
         */
        ROOT::Math::XYVector getKinkAtScatterer() { return m_kinkAtScatterer; };

        std::shared_ptr<Track> getUpstreamTracklet() { return m_upstream; };
        std::shared_ptr<Track> getDownstreamTracklet() { return m_downstream; };

        ROOT::Math::XYPoint getKinkAt(const std::string&) const override;
        void setVolumeScatter(double) override{};

    private:
        std::shared_ptr<Track> m_upstream;
        std::shared_ptr<Track> m_downstream;

        void calculateChi2();
        void calculateResiduals();

        double m_scattererPosition;
        ROOT::Math::XYVector m_offsetAtScatterer;
        ROOT::Math::XYZPoint m_positionAtScatterer;
        ROOT::Math::XYVector m_kinkAtScatterer;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Multiplet, 1)
    };
    // Vector type declaration
    using MultipletVector = std::vector<std::shared_ptr<Multiplet>>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_Multiplet_H
