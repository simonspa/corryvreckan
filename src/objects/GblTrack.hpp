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

#include <GblPoint.h>
#include "Track.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief GblTrack object
     *
     * This class is a general broken line track which knows how to fit itself. It is derived from Track
     */

    class GblTrack : public Track {
        using Jacobian = Eigen::Matrix<double, 6, 6>;

    public:
        void print(std::ostream& out) const override;

        /**
         * @brief The fitting routine
         */
        void fit() override;

        /**
         * @brief Get the track position for a certain z position
         * @param z GLobal z position
         * @return ROOT::Math::XYZPoint at z position
         */
        ROOT::Math::XYZPoint getIntercept(double z) const override;

        /**
         * @brief Get the track state at a detector
         * @param detectorID Name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        ROOT::Math::XYZPoint getState(const std::string& detectorID) const override;

        /**
         * @brief Get the track direction at a detector
         * @param detectorID Name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        ROOT::Math::XYZVector getDirection(const std::string& detectorID) const override;

        /**
         * @brief Get the track direction at any given z position
         * @param z Global z position
         * @return ROOT::Math::XYZPoint direction at z
         */
        ROOT::Math::XYZVector getDirection(const double& z) const override;

        /**
         * @brief Return kink of track at given detector
         * @param  detectorID Detector ID at which the kink should be evaluated
         * @return            Kink at given detector
         */
        ROOT::Math::XYPoint getKinkAt(const std::string& detectorID) const override;

        void setVolumeScatter(double length) override;

    private:
        // static Matrices to convert from proteus like numbering to eigen3 like numbering of space
        static Eigen::Matrix<double, 5, 6> toGbl;
        static Eigen::Matrix<double, 6, 5> toProt;

        /**
         * @brief Set seedcluster used for track fitting
         * @param cluster Pointer to seedcluster of the GblTrack
         */
        void set_seed_cluster(const Cluster* cluster);

        /**
         * @brief Get seedcluster used for track fitting
         * @return Pointer to seedcluster of the GblTrack if set, nullptr otherwise
         */
        Cluster* get_seed_cluster() const;

        /**
         * Transient storage for GblPoints prepared for the trajectory
         */
        std::vector<gbl::GblPoint> gblpoints_; //! transient value

        /**
         * @brief Helper function to populate the point trajectory
         */
        void prepare_gblpoints();

        void add_plane(std::vector<Plane>::iterator& plane,
                       Transform3D& prevToGlobal,
                       Transform3D& prevToLocal,
                       ROOT::Math::XYZPoint& globalTrackPos,
                       double total_material);

        /**
         * @brief get_position_outside_telescope
         * @param z Poisiton of requested point
         * @return XYZ Point of track outside telescope coverage
         */
        ROOT::Math::XYZPoint get_position_outside_telescope(double z) const;

        // Member variables
        PointerWrapper<Cluster> seed_cluster_{nullptr};
        double scattering_length_volume_{};
        bool use_volume_scatter_{};

        std::map<std::string, ROOT::Math::XYPoint> local_track_points_{};
        std::map<std::string, ROOT::Math::XYZPoint> local_fitted_track_points_{};
        std::map<std::string, ROOT::Math::XYPoint> initital_residual_{};
        std::map<std::string, ROOT::Math::XYPoint> kink_{};
        std::map<std::string, unsigned int> plane_to_gblpoint_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(GblTrack, 6);
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_GBLTRACK_H
