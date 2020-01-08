#ifndef GblTrack_H
#define GblTrack_H 1

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
         * @brief  Get the distance between cluster and track
         * @param Cluster* Pointer to the cluster
         * @return distance between cluster and track
         */
        double distance2(const Cluster* cluster) const override;

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
        /**
         * @brief Calculate the expected variance according to the higland formula
         * @param mbCurrent is the material budget of current scatterer
         * @param mbSum is the material budget of all previous scatterers
         * @return expected scattering variance
         */
        double scatteringTheta(double mbCurrent, double mbSum);

        /**
         * @brief Sort all layers by their z-position and store them
         */
        void createSortedListOfSensors();

        std::vector<std::pair<double, std::string>> m_sorted_budgets{};
        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(GblTrack, 0)
    };

    // Vector type declaration
    using GblTrackVector = std::vector<GblTrack*>;

} // namespace corryvreckan

#endif // GblTrack_H
